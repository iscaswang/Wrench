/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "ScreenRecord"
//#define LOG_NDEBUG 0
#include <utils/Log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "FrameOutput.h"

using namespace android;

static const bool kShowTiming = false;      // set to "true" for debugging
static const int kGlBytesPerPixel = 4;      // GL_RGBA
static const int kOutBytesPerPixel = 3;     // RGB only

inline void FrameOutput::setValueLE(uint8_t* buf, uint32_t value) {
    // Since we're running on an Android device, we're (almost) guaranteed
    // to be little-endian, and (almost) guaranteed that unaligned 32-bit
    // writes will work without any performance penalty... but do it
    // byte-by-byte anyway.
    buf[0] = (uint8_t) value;
    buf[1] = (uint8_t) (value >> 8);
    buf[2] = (uint8_t) (value >> 16);
    buf[3] = (uint8_t) (value >> 24);
}

status_t FrameOutput::createInputSurface(int width, int height,
        sp<IGraphicBufferProducer>* pBufferProducer) {
    status_t err;

    err = mEglWindow.createPbuffer(width, height);
    if (err != NO_ERROR) {
        fprintf(stderr, "%s:%d: err is %d\n", __FILE__, __LINE__, err);
        return err;
    }
    mEglWindow.makeCurrent();

    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Shader for rendering the external texture.
    err = mExtTexProgram.setup(Program::PROGRAM_EXTERNAL_TEXTURE);
    if (err != NO_ERROR) {
        fprintf(stderr, "%s:%d: err is %d\n", __FILE__, __LINE__, err);
        return err;
    }

    // Input side (buffers from virtual display).
    glGenTextures(1, &mExtTextureName);
    if (mExtTextureName == 0) {
        ALOGE("glGenTextures failed: %#x", glGetError());
        fprintf(stderr, "%s:%d: err is %d\n", __FILE__, __LINE__, UNKNOWN_ERROR);
        return UNKNOWN_ERROR;
    }

    sp<IGraphicBufferProducer> producer;
    sp<IGraphicBufferConsumer> consumer;
    BufferQueue::createBufferQueue(&producer, &consumer);
    mGlConsumer = new GLConsumer(consumer, mExtTextureName,
                GL_TEXTURE_EXTERNAL_OES, true, false);
    mGlConsumer->setName(String8("virtual display"));
    mGlConsumer->setDefaultBufferSize(width, height);
#if PLATFORM_SDK_VERSION >= 24
    producer->setMaxDequeuedBufferCount(4);
#else
    mGlConsumer->setDefaultMaxBufferCount(5);
#endif
    mGlConsumer->setConsumerUsageBits(GRALLOC_USAGE_HW_TEXTURE);

    mGlConsumer->setFrameAvailableListener(this);

    mPixelBuf = new uint8_t[width * height * kGlBytesPerPixel];

    *pBufferProducer = producer;

    ALOGD("FrameOutput::createInputSurface OK");
    return NO_ERROR;
}

unsigned char* FrameOutput::getPixelBuf() {
    return mPixelBuf;
}

status_t FrameOutput::copyFrame(long timeoutUsec) {
    Mutex::Autolock _l(mMutex);
    ALOGV("copyFrame %ld\n", timeoutUsec);

    if (!mFrameAvailable) {
        nsecs_t timeoutNsec = (nsecs_t)timeoutUsec * 1000;
        int cc = mEventCond.waitRelative(mMutex, timeoutNsec);
        if (cc == -ETIMEDOUT) {
            ALOGV("cond wait timed out");
            return ETIMEDOUT;
        } else if (cc != 0) {
            ALOGW("cond wait returned error %d", cc);
            return cc;
        }
    }
    if (!mFrameAvailable) {
        // This happens when Ctrl-C is hit.  Apparently POSIX says that the
        // pthread wait call doesn't return EINTR, treating this instead as
        // an instance of a "spurious wakeup".  We didn't get a frame, so
        // we just treat it as a timeout.
        return ETIMEDOUT;
    }

    // A frame is available.  Clear the flag for the next round.
    mFrameAvailable = false;

    float texMatrix[16];
    mGlConsumer->updateTexImage();
    mGlConsumer->getTransformMatrix(texMatrix);

    // The data is in an external texture, so we need to render it to the
    // pbuffer to get access to RGB pixel data.  We also want to flip it
    // upside-down for easy conversion to a bitmap.
    int width = mEglWindow.getWidth();
    int height = mEglWindow.getHeight();
    status_t err = mExtTexProgram.blit(mExtTextureName, texMatrix, 0, 0,
            width, height, true);
    if (err != NO_ERROR) {
        return err;
    }

    // GLES only guarantees that glReadPixels() will work with GL_RGBA, so we
    // need to get 4 bytes/pixel and reduce it.  Depending on the size of the
    // screen and the device capabilities, this can take a while.
    int64_t startWhenNsec, pixWhenNsec, endWhenNsec;
    if (kShowTiming) {
        startWhenNsec = systemTime(CLOCK_MONOTONIC);
    }
    GLenum glErr;
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, mPixelBuf);
    if ((glErr = glGetError()) != GL_NO_ERROR) {
        ALOGE("glReadPixels failed: %#x", glErr);
        return UNKNOWN_ERROR;
    }
    if (kShowTiming) {
        pixWhenNsec = systemTime(CLOCK_MONOTONIC);
    }
    if (kShowTiming) {
        endWhenNsec = systemTime(CLOCK_MONOTONIC);
        ALOGD("got pixels (get=%.3f ms, reduce=%.3fms)",
                (pixWhenNsec - startWhenNsec) / 1000000.0,
                (endWhenNsec - pixWhenNsec) / 1000000.0);
    }

    size_t rgbDataLen = width * height * kOutBytesPerPixel;

    // Currently using buffered I/O rather than writev().  Not expecting it
    // to make much of a difference, but it might be worth a test for larger
    // frame sizes.
    if (kShowTiming) {
        startWhenNsec = systemTime(CLOCK_MONOTONIC);
    }
    if (kShowTiming) {
        endWhenNsec = systemTime(CLOCK_MONOTONIC);
        ALOGD("wrote pixels (%.3f ms)",
                (endWhenNsec - startWhenNsec) / 1000000.0);
    }
    return NO_ERROR;
}

// Callback; executes on arbitrary thread.
void FrameOutput::onFrameAvailable(const BufferItem& /* item */) {
    Mutex::Autolock _l(mMutex);
    mFrameAvailable = true;
    mEventCond.signal();
}
