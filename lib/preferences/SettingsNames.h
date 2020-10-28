/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H
#define QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H

////////////////////////////////////////////////////////////////////////////////

// Account-related settings keys
#define ACCOUNT_SETTINGS_GROUP QStringLiteral("AccountSettings")

#define LAST_USED_ACCOUNT_NAME QStringLiteral("LastUsedAccountName")
#define LAST_USED_ACCOUNT_TYPE QStringLiteral("LastUsedAccountType")
#define LAST_USED_ACCOUNT_ID   QStringLiteral("LastUsedAccountId")

#define LAST_USED_ACCOUNT_EVERNOTE_ACCOUNT_TYPE                                \
    QStringLiteral("LastUsedAccountEvernoteAccountType")

#define LAST_USED_ACCOUNT_EVERNOTE_HOST                                        \
    QStringLiteral("LastUsedAccountEvernoteHost")

#define ONCE_DISPLAYED_GREETER_SCREEN                                          \
    QStringLiteral("OnceDisplayedGreeterScreen")

// Environment variables that can be used to specify the account to use on
// startup
#define ACCOUNT_NAME_ENV_VAR "QUENTIER_ACCOUNT_NAME"
#define ACCOUNT_TYPE_ENV_VAR "QUENTIER_ACCOUNT_TYPE"
#define ACCOUNT_ID_ENV_VAR   "QUENTIER_ACCOUNT_ID"

#define ACCOUNT_EVERNOTE_ACCOUNT_TYPE_ENV_VAR                                  \
    "QUENTIER_ACCOUNT_EVERNOTE_ACCOUNT_TYPE"

#define ACCOUNT_EVERNOTE_HOST_ENV_VAR "QUENTIER_ACCOUNT_EVERNOTE_HOST"

////////////////////////////////////////////////////////////////////////////////

// Synchronization settings
#define SYNCHRONIZATION_SETTINGS_GROUP_NAME                                    \
    QStringLiteral("SynchronizationSettings")

#define SYNCHRONIZATION_DOWNLOAD_NOTE_THUMBNAILS                               \
    QStringLiteral("DownloadNoteThumbnails")

#define SYNCHRONIZATION_DOWNLOAD_INK_NOTE_IMAGES                               \
    QStringLiteral("DownloadInkNoteImages")

#define SYNCHRONIZATION_RUN_SYNC_EACH_NUM_MINUTES                              \
    QStringLiteral("RunSyncEachNumMinutes")

#define SYNCHRONIZATION_NETWORK_PROXY_SETTINGS                                 \
    QStringLiteral("SynchronizationNetworkProxySettings")

#define SYNCHRONIZATION_NETWORK_PROXY_TYPE                                     \
    QStringLiteral("SynchronizationNetworkProxyType")

#define SYNCHRONIZATION_NETWORK_PROXY_HOST                                     \
    QStringLiteral("SynchronizationNetworkProxyHost")

#define SYNCHRONIZATION_NETWORK_PROXY_PORT                                     \
    QStringLiteral("SynchronizationNetworkProxyPort")

#define SYNCHRONIZATION_NETWORK_PROXY_USER                                     \
    QStringLiteral("SynchronizationNetworkProxyUser")

#define SYNCHRONIZATION_NETWORK_PROXY_PASSWORD                                 \
    QStringLiteral("SynchronizationNetworkProxyPassword")

#endif // QUENTIER_LIB_PREFERENCES_SETTINGS_NAMES_H
