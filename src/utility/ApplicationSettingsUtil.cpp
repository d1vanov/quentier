/*
 * Copyright 2018 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ApplicationSettingsUtil.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

QVariant getApplicationSetting(
    ApplicationSettings & appSettings,
    const QString & settingsGroup,
    const QString & settingsKey,
    const QVariant & defaultValue)
{
    QVariant ret = defaultValue;
    appSettings.beginGroup(settingsGroup);
    if (appSettings.contains(settingsKey)) {
        ret = appSettings.value(settingsKey);
    }
    appSettings.endGroup();
    return ret;
}


QVariant getApplicationSetting(
    const Account & account,
    const QString & settingsName,
    const QString & settingsGroup,
    const QString & settingsKey,
    const QVariant & defaultValue)
{
    QNTRACE(QStringLiteral("getApplicationSetting"));

    ApplicationSettings appSettings(account, settingsName);
    return getApplicationSetting(appSettings, settingsGroup, settingsKey, defaultValue);
}

void setApplicationSetting(
    const Account & account,
    const QString & settingsName,
    const QString & settingsGroup,
    const QString & settingsKey,
    const QVariant & value) {
    ApplicationSettings appSettings(account, settingsName);
    appSettings.beginGroup(settingsGroup);
    appSettings.setValue(settingsKey, value);
    appSettings.endGroup();
}

} // namespace quentier
