/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of a Qt Solutions component.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/


#ifndef QSCRIPTCONTEXTINFO_H
#define QSCRIPTCONTEXTINFO_H

#include <QtCore/qobjectdefs.h>


#include <QtCore/qlist.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Script)

class QScriptContext;
#ifndef QT_NO_DATASTREAM
class QDataStream;
#endif

class QScriptContextInfoPrivate;
class Q_SCRIPT_EXPORT QScriptContextInfo
{
public:
#ifndef QT_NO_DATASTREAM
    friend Q_SCRIPT_EXPORT QDataStream &operator<<(QDataStream &, const QScriptContextInfo &);
    friend Q_SCRIPT_EXPORT QDataStream &operator>>(QDataStream &, QScriptContextInfo &);
#endif

    enum FunctionType {
        ScriptFunction,
        QtFunction,
        QtPropertyFunction,
        NativeFunction
    };

    QScriptContextInfo(const QScriptContext *context);
    QScriptContextInfo(const QScriptContextInfo &other);
    QScriptContextInfo();
    ~QScriptContextInfo();

    QScriptContextInfo &operator=(const QScriptContextInfo &other);

    bool isNull() const;

    qint64 scriptId() const;
    QString fileName() const;
    int lineNumber() const;
    int columnNumber() const;

    QString functionName() const;
    FunctionType functionType() const;

    QStringList functionParameterNames() const;

    int functionStartLineNumber() const;
    int functionEndLineNumber() const;

    int functionMetaIndex() const;

    bool operator==(const QScriptContextInfo &other) const;
    bool operator!=(const QScriptContextInfo &other) const;

private:
    QScriptContextInfoPrivate *d_ptr;

    Q_DECLARE_PRIVATE(QScriptContextInfo)
};

typedef QList<QScriptContextInfo> QScriptContextInfoList;

#ifndef QT_NO_DATASTREAM
Q_SCRIPT_EXPORT QDataStream &operator<<(QDataStream &, const QScriptContextInfo &);
Q_SCRIPT_EXPORT QDataStream &operator>>(QDataStream &, QScriptContextInfo &);
#endif

QT_END_NAMESPACE

QT_END_HEADER


#endif
