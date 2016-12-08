/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/exception/ApplicationSettingsInitializationException.h>
#include <QApplication>

namespace quentier {

ApplicationSettings::ApplicationSettings() :
    QSettings(QApplication::organizationName(), QApplication::applicationName())
{
    if (Q_UNLIKELY(QApplication::organizationName().isEmpty())) {
        throw ApplicationSettingsInitializationException("can't create ApplicationSettings instance: organization name is empty");
    }

    if (Q_UNLIKELY(QApplication::applicationName().isEmpty())) {
        throw ApplicationSettingsInitializationException("can't create ApplicationSettings instance: application name is empty");
    }

    QString storagePath = applicationPersistentStoragePath();
    if (Q_UNLIKELY(storagePath.isEmpty())) {
        throw ApplicationSettingsInitializationException("can't create ApplicationSettings instance: no persistent storage path");
    }

    storagePath += QStringLiteral("/settings");

    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, storagePath);
}

ApplicationSettings::~ApplicationSettings()
{}

QTextStream & ApplicationSettings::print(QTextStream & strm) const
{
    QStringList allStoredKeys = QSettings::allKeys();

    for(auto it = allStoredKeys.constBegin(), end = allStoredKeys.constEnd(); it != end; ++it) {
        QVariant value = QSettings::value(*it);
        strm << QStringLiteral("Key: ") << *it << QStringLiteral("; Value: ") << value.toString() << QStringLiteral("\n;");
    }

    return strm;
}

} // namespace quentier
