/*
 * Copyright 2020 Dmitry Ivanov
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

#include "UpdateSettings.h"

#include "DefaultSettings.h"
#include "SettingsNames.h"

#include <lib/utility/VersionInfo.h>

#include <quentier/utility/ApplicationSettings.h>

#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>

namespace quentier {

void readPersistentUpdateSettings(
    bool & checkForUpdatesEnabled,
    bool & shouldCheckForUpdatesOnStartup,
    bool & useContinuousUpdateChannel,
    int & checkForUpdatesIntervalOption,
    QString & updateChannel,
    UpdateProvider & updateProvider)
{
    ApplicationSettings appSettings;
    appSettings.beginGroup(CHECK_FOR_UPDATES_SETTINGS_GROUP_NAME);

    checkForUpdatesEnabled = appSettings.value(
        CHECK_FOR_UPDATES_SETTINGS_KEY,
        DEFAULT_CHECK_FOR_UPDATES).toBool();

    shouldCheckForUpdatesOnStartup = appSettings.value(
        CHECK_FOR_UPDATES_ON_STARTUP_SETTINGS_KEY,
        DEFAULT_CHECK_FOR_UPDATES_ON_STARTUP).toBool();

    useContinuousUpdateChannel = appSettings.value(
        USE_CONTINUOUS_UPDATE_CHANNEL_SETTINGS_KEY,
        DEFAULT_USE_CONTINUOUS_UPDATE_CHANNEL).toBool();

    checkForUpdatesIntervalOption = appSettings.value(
        CHECK_FOR_UPDATES_INTERVAL_SETTINGS_KEY,
        DEFAULT_CHECK_FOR_UPDATES_INTERVAL_OPTION_INDEX).toInt();

    updateChannel = appSettings.value(
        CHECK_FOR_UPDATES_CHANNEL_KEY,
        DEFAULT_UPDATE_CHANNEL).toString();

#if !QUENTIER_PACKAGED_AS_APP_IMAGE
    // Only GitHub provider is available
    updateProvider = UpdateProvider::GITHUB;
#else
    int updateProviderIndex = appSettings.value(
        CHECK_FOR_UPDATES_PROVIDER_SETTINGS_KEY,
        static_cast<int>(DEFAULT_UPDATES_PROVIDER)).toInt();
    if (updateProviderIndex >= 0 && updateProviderIndex <= 1) {
        updateProvider = static_cast<UpdateProvider>(updateProviderIndex);
    }
    else {
        updateProvider = DEFAULT_UPDATES_PROVIDER;
    }
#endif

    appSettings.endGroup();
}

qint64 checkForUpdatesIntervalMsecFromOption(
    const CheckForUpdatesInterval option)
{
    switch(option)
    {
    case CheckForUpdatesInterval::FIFTEEN_MINUTES:
        return 15ll * 60 * 1000;
    case CheckForUpdatesInterval::HALF_AN_HOUR:
        return 30ll * 60 * 1000;
    case CheckForUpdatesInterval::HOUR:
        return 60ll * 60 * 1000;
    case CheckForUpdatesInterval::TWO_HOURS:
        return 120ll * 60 * 1000;
    case CheckForUpdatesInterval::FOUR_HOURS:
        return 240ll * 60 * 1000;
    case CheckForUpdatesInterval::DAILY:
        return 24ll * 60 * 60 * 1000;
    case CheckForUpdatesInterval::MONTHLY:
        return 30ll * 24 * 60 * 60 * 1000;
    case CheckForUpdatesInterval::WEEKLY:
    default:
        return 7ll * 24 * 60 * 60 * 1000;
    }
}

QString updateProviderName(const UpdateProvider updateProvider)
{
    switch(updateProvider)
    {
    case UpdateProvider::GITHUB:
        return QCoreApplication::translate("preferences", "GitHub releases");
    case UpdateProvider::APPIMAGE:
        return QCoreApplication::translate("preferences", "AppImage");
    default:
        return QString::fromUtf8("Unknown: ") +
            QString::number(static_cast<int>(updateProvider));
    }
}

namespace {

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void printCheckForUpdatesInterval(T & t, const CheckForUpdatesInterval interval)
{
    switch(interval)
    {
    case CheckForUpdatesInterval::FIFTEEN_MINUTES:
        t << "15 minutes";
        break;
    case CheckForUpdatesInterval::HALF_AN_HOUR:
        t << "30 minutes";
        break;
    case CheckForUpdatesInterval::HOUR:
        t << "1 hour";
        break;
    case CheckForUpdatesInterval::TWO_HOURS:
        t << "2 hours";
        break;
    case CheckForUpdatesInterval::FOUR_HOURS:
        t << "4 hours";
        break;
    case CheckForUpdatesInterval::DAILY:
        t << "1 day";
        break;
    case CheckForUpdatesInterval::MONTHLY:
        t << "1 month";
        break;
    case CheckForUpdatesInterval::WEEKLY:
        t << "1 week";
        break;
    default:
        t << "Unknown: option value = " << static_cast<qint64>(interval);
        break;
    }
}

template <typename T>
void printUpdateProvider(T & t, const UpdateProvider provider)
{
    switch(provider)
    {
    case UpdateProvider::GITHUB:
        t << "GitHub";
        break;
    case UpdateProvider::APPIMAGE:
        t << "AppImage";
        break;
    default:
        t << "Unknown: option value = " << static_cast<qint64>(provider);
        break;
    }
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

QTextStream & operator<<(
    QTextStream & strm, const CheckForUpdatesInterval interval)
{
    printCheckForUpdatesInterval(strm, interval);
    return strm;
}

QDebug & operator<<(QDebug & dbg, const CheckForUpdatesInterval interval)
{
    printCheckForUpdatesInterval(dbg, interval);
    return dbg;
}

QTextStream & operator<<(QTextStream & strm, const UpdateProvider provider)
{
    printUpdateProvider(strm, provider);
    return strm;
}

QDebug & operator<<(QDebug & dbg, const UpdateProvider provider)
{
    printUpdateProvider(dbg, provider);
    return dbg;
}

} // namespace quentier
