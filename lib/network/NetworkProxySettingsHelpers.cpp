/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include "NetworkProxySettingsHelpers.h"

#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/Synchronization.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Account.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QUrl>

#include <algorithm>
#include <memory>

namespace quentier {

void parseNetworkProxySettings(
    const Account & currentAccount, QNetworkProxy::ProxyType & type,
    QString & host, int & port, QString & user, QString & password)
{
    // Initialize with "empty" values
    type = QNetworkProxy::DefaultProxy;
    host.resize(0);
    port = -1;
    user.resize(0);
    password.resize(0);

    std::unique_ptr<ApplicationSettings> syncSettings;
    if (currentAccount.isEmpty()) {
        QNDEBUG(
            "network",
            "parseNetworkProxySettings: using application-wise "
                << "settings");

        syncSettings.reset(new ApplicationSettings);
    }
    else {
        QNDEBUG(
            "network",
            "parseNetworkProxySettings: using account-specific "
                << "settings");

        syncSettings.reset(new ApplicationSettings(
            currentAccount, preferences::keys::files::synchronization));
    }

    syncSettings->beginGroup(preferences::keys::syncNetworkProxyGroup);
    ApplicationSettings::GroupCloser groupCloser{*syncSettings};

    // 1) Parse network proxy type

    if (syncSettings->contains(preferences::keys::syncNetworkProxyType)) {
        const auto data =
            syncSettings->value(preferences::keys::syncNetworkProxyType);
        bool convertedToInt = false;
        const int proxyType = data.toInt(&convertedToInt);
        if (convertedToInt) {
            // NOTE: it is unsafe to just cast int to QNetworkProxy::ProxyType,
            // it can be out of range; hence, checking for each available proxy
            // type manually
            switch (proxyType) {
            case QNetworkProxy::NoProxy:
                type = QNetworkProxy::NoProxy;
                break;
            case QNetworkProxy::DefaultProxy:
                type = QNetworkProxy::DefaultProxy;
                break;
            case QNetworkProxy::Socks5Proxy:
                type = QNetworkProxy::Socks5Proxy;
                break;
            case QNetworkProxy::HttpProxy:
                type = QNetworkProxy::HttpProxy;
                break;
            case QNetworkProxy::HttpCachingProxy:
                type = QNetworkProxy::HttpCachingProxy;
                break;
            case QNetworkProxy::FtpCachingProxy:
                type = QNetworkProxy::FtpCachingProxy;
                break;
            default:
            {
                QNWARNING(
                    "network",
                    "Unrecognized network proxy type: "
                        << proxyType << ", fallback to the default proxy type");
                type = QNetworkProxy::DefaultProxy;
                break;
            }
            }
        }
        else {
            QNWARNING(
                "network",
                "Failed to convert the network proxy type to int: " << data
                    << ", fallback to the default proxy type");
            type = QNetworkProxy::DefaultProxy;
        }
    }
    else {
        QNDEBUG(
            "network",
            "No network proxy type was found within the settings");
    }

    // 2) Parse network proxy host

    if (syncSettings->contains(preferences::keys::syncNetworkProxyHost)) {
        const QString data =
            syncSettings->value(preferences::keys::syncNetworkProxyHost)
                .toString();

        if (!data.isEmpty()) {
            QUrl url{data};
            if (Q_UNLIKELY(!url.isValid())) {
                QNWARNING(
                    "network",
                    "Network proxy host read from app settings does not appear "
                        << "to be the valid URL: " << data);
            }
            else {
                host = data;
            }
        }
    }

    if (host.isEmpty()) {
        QNDEBUG("network", "No host is specified within the settings");
    }

    // 3) Parse network proxy port

    if (syncSettings->contains(preferences::keys::syncNetworkProxyPort)) {
        const auto data =
            syncSettings->value(preferences::keys::syncNetworkProxyPort);
        bool convertedToInt = false;
        const int proxyPort = data.toInt(&convertedToInt);
        if (convertedToInt) {
            port = proxyPort;
        }
        else {
            QNWARNING(
                "network",
                "Failed to convert the network proxy port to int: " << data);
        }
    }
    else {
        QNDEBUG(
            "network",
            "No network proxy port was found within the settings");
    }

    // 4) Parse network proxy username

    if (syncSettings->contains(preferences::keys::syncNetworkProxyUser)) {
        user = syncSettings->value(preferences::keys::syncNetworkProxyUser)
                   .toString();
    }
    else {
        QNDEBUG(
            "network",
            "No network proxy username was found within the settings");
    }

    // 5) Parse network proxy password

    if (syncSettings->contains(preferences::keys::syncNetworkProxyPassword)) {
        password =
            syncSettings->value(preferences::keys::syncNetworkProxyPassword)
                .toString();
    }
    else {
        QNDEBUG(
            "network",
            "No network proxy password was found within the settings");
    }

    QNDEBUG(
        "network",
        "Result: network proxy type = "
            << type << ", host = " << host << ", port = " << port
            << ", username = " << user << ", password: "
            << (password.isEmpty() ? "<empty>" : "not empty"));
}

void persistNetworkProxySettingsForAccount(
    const Account & account, const QNetworkProxy & proxy)
{
    QNDEBUG(
        "network",
        "persistNetworkProxySettingsForAccount: account = "
            << account.name() << "\nProxy type = " << proxy.type()
            << ", proxy host = " << proxy.hostName() << ", proxy port = "
            << proxy.port() << ", proxy user = " << proxy.user());

    std::unique_ptr<ApplicationSettings> syncSettings;
    if (account.isEmpty()) {
        QNDEBUG("network", "Persisting application-wise proxy settings");
        syncSettings.reset(new ApplicationSettings);
    }
    else {
        QNDEBUG("network", "Persisting account-specific settings");

        syncSettings.reset(new ApplicationSettings(
            account, preferences::keys::files::synchronization));
    }

    syncSettings->beginGroup(preferences::keys::syncNetworkProxyGroup);
    ApplicationSettings::GroupCloser groupCloser{*syncSettings};

    syncSettings->setValue(
        preferences::keys::syncNetworkProxyType, proxy.type());

    syncSettings->setValue(
        preferences::keys::syncNetworkProxyHost, proxy.hostName());

    syncSettings->setValue(
        preferences::keys::syncNetworkProxyPort, proxy.port());

    syncSettings->setValue(
        preferences::keys::syncNetworkProxyUser, proxy.user());

    syncSettings->setValue(
        preferences::keys::syncNetworkProxyPassword, proxy.password());
}

void restoreNetworkProxySettingsForAccount(const Account & account)
{
    QNDEBUG(
        "network",
        "restoreNetworkProxySettingsForAccount: account = " << account.name());

    QNetworkProxy::ProxyType type = QNetworkProxy::NoProxy;
    QString host;
    int port = 0;
    QString user;
    QString password;

    parseNetworkProxySettings(account, type, host, port, user, password);

    QNetworkProxy proxy{type};
    proxy.setHostName(host);
    proxy.setPort(static_cast<quint16>(std::max(port, 0)));
    proxy.setUser(user);
    proxy.setPassword(password);

    QNTRACE(
        "network",
        "Setting the application proxy extracted from app settings");

    QNetworkProxy::setApplicationProxy(proxy);
}

} // namespace quentier
