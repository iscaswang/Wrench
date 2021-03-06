/*
    SnoreNotify is a Notification Framework based on Qt
    Copyright (C) 2015  Hannah von Reth <vonreth@kde.org>

    SnoreNotify is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SnoreNotify is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with SnoreNotify.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SETTINGSUTILS_H
#define SETTINGSUTILS_H

#include <QSettings>

class SettingsUtils
{
public:
    template<typename Func>
    static const QStringList allSettingsKeysWithPrefix(const QString &prefix, QSettings &settings, Func fun)
    {
        QStringList groups = prefix.split(QLatin1Char('/'));
        QStringList out;

        foreach(const QString & group, groups) {
            settings.beginGroup(group);
        }
        out = fun(settings);

        for (int i = 0; i < groups.size(); ++i) {
            settings.endGroup();
        }
        return out;
    }

    static const QStringList knownApps();

    static QSettings &settings();

};

#endif // SETTINGSUTILS_H