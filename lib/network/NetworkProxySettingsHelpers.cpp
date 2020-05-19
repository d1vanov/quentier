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

#include "NetworkProxySettingsHelpers.h"

#include <lib/preferences/SettingsNames.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/Account.h>
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/utility/Macros.h>

#include <QUrl>

#include <algorithm>
#include <memory>

namespace quentier {

void parseNetworkProxySettings(
    const Account & currentAccount,
    QNetworkProxy::ProxyType & type, QString & host,
    int & port, QString & user, QString & password)
{
    // Initialize with "empty" values
    type = QNetworkProxy::DefaultProxy;
    host.resize(0);
    port = -1;
    user.resize(0);
    password.resize(0);

    std::unique_ptr<ApplicationSettings> pSyncSettings;
    if (currentAccount.isEmpty()) {
        QNDEBUG("parseNetworkProxySettings: using application-wise settings");
        pSyncSettings.reset(new ApplicationSettings);
    }
    else {
        QNDEBUG("parseNetworkProxySettings: using account-specific settings");
        pSyncSettings.reset(
            new ApplicationSettings(currentAccount, QUENTIER_SYNC_SETTINGS));
    }

    pSyncSettings->beginGroup(SYNCHRONIZATION_NETWORK_PROXY_SETTINGS);

    // 1) Parse network proxy type

    if (pSyncSettings->contains(SYNCHRONIZATION_NETWORK_PROXY_TYPE))
    {
        QVariant data = pSyncSettings->value(SYNCHRONIZATION_NETWORK_PROXY_TYPE);
        bool convertedToInt = false;
        int proxyType = data.toInt(&convertedToInt);
        if (convertedToInt)
        {
            // NOTE: it is unsafe to just cast int to QNetworkProxy::ProxyType,
            // it can be out of range; hence, checking for each available proxy
            // type manually
            switch(proxyType)
            {
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
                    QNWARNING("Unrecognized network proxy type: " << proxyType
                        << ", fallback to the default proxy type");
                    type = QNetworkProxy::DefaultProxy;
                    break;
                }
            }
        }
        else
        {
            QNWARNING("Failed to convert the network proxy type to int: "
                << data << ", fallback to the default proxy type");
            type = QNetworkProxy::DefaultProxy;
        }
    }
    else
    {
        QNDEBUG("No network proxy type was found within the settings");
    }

    // 2) Parse network proxy host

    if (pSyncSettings->contains(SYNCHRONIZATION_NETWORK_PROXY_HOST))
    {
        QString data = pSyncSettings->value(
            SYNCHRONIZATION_NETWORK_PROXY_HOST).toString();
        if (!data.isEmpty())
        {
            QUrl url(data);
            if (Q_UNLIKELY(!url.isValid())) {
                QNWARNING("Network proxy host read from app settings does not "
                    << "appear to be the valid URL: " << data);
            }
            else {
                host = data;
            }
        }
    }

    if (host.isEmpty()) {
        QNDEBUG("No host is specified within the settings");
    }

    // 3) Parse network proxy port

    if (pSyncSettings->contains(SYNCHRONIZATION_NETWORK_PROXY_PORT))
    {
        QVariant data = pSyncSettings->value(SYNCHRONIZATION_NETWORK_PROXY_PORT);
        bool convertedToInt = false;
        int proxyPort = data.toInt(&convertedToInt);
        if (convertedToInt) {
            port = proxyPort;
        }
        else {
            QNWARNING("Failed to convert the network proxy port to int: "
                << data);
        }
    }
    else
    {
        QNDEBUG("No network proxy port was found within the settings");
    }

    // 4) Parse network proxy username

    if (pSyncSettings->contains(SYNCHRONIZATION_NETWORK_PROXY_USER)) {
        user = pSyncSettings->value(
            SYNCHRONIZATION_NETWORK_PROXY_USER).toString();
    }
    else {
        QNDEBUG("No network proxy username was found within the settings");
    }

    // 5) Parse network proxy password

    if (pSyncSettings->contains(SYNCHRONIZATION_NETWORK_PROXY_PASSWORD)) {
        password = pSyncSettings->value(
            SYNCHRONIZATION_NETWORK_PROXY_PASSWORD).toString();
    }
    else {
        QNDEBUG("No network proxy password was found within the settings");
    }

    pSyncSettings->endGroup();

    QNDEBUG("Result: network proxy type = " << type << ", host = " << host
        << ", port = " << port << ", username = " << user
        << ", password: " << (password.isEmpty() ? "<empty>" : "not empty"));
}

void persistNetworkProxySettingsForAccount(
    const Account & account, const QNetworkProxy & proxy)
{
    QNDEBUG("persistNetworkProxySettingsForAccount: account = "
        << account.name() << "\nProxy type = "
        << proxy.type() << ", proxy host = "
        << proxy.hostName() << ", proxy port = "
        << proxy.port() << ", proxy user = " << proxy.user());

    std::unique_ptr<ApplicationSettings> pSyncSettings;
    if (account.isEmpty()) {
        QNDEBUG("Persisting application-wise proxy settings");
        pSyncSettings.reset(new ApplicationSettings);
    }
    else {
        QNDEBUG("Persisting account-specific settings");
        pSyncSettings.reset(
            new ApplicationSettings(account, QUENTIER_SYNC_SETTINGS));
    }

    pSyncSettings->beginGroup(SYNCHRONIZATION_NETWORK_PROXY_SETTINGS);

    pSyncSettings->setValue(SYNCHRONIZATION_NETWORK_PROXY_TYPE, proxy.type());

    pSyncSettings->setValue(
        SYNCHRONIZATION_NETWORK_PROXY_HOST,
        proxy.hostName());

    pSyncSettings->setValue(SYNCHRONIZATION_NETWORK_PROXY_PORT, proxy.port());
    pSyncSettings->setValue(SYNCHRONIZATION_NETWORK_PROXY_USER, proxy.user());

    pSyncSettings->setValue(
        SYNCHRONIZATION_NETWORK_PROXY_PASSWORD,
        proxy.password());

    pSyncSettings->endGroup();
}

void restoreNetworkProxySettingsForAccount(const Account & account)
{
    QNDEBUG("restoreNetworkProxySettingsForAccount: account = "
        << account.name());

    QNetworkProxy::ProxyType type = QNetworkProxy::NoProxy;
    QString host;
    int port = 0;
    QString user;
    QString password;

    parseNetworkProxySettings(account, type, host, port, user, password);

    QNetworkProxy proxy(type);
    proxy.setHostName(host);
    proxy.setPort(static_cast<quint16>(std::max(port, 0)));
    proxy.setUser(user);
    proxy.setPassword(password);

    QNTRACE("Setting the application proxy extracted from app settings");
    QNetworkProxy::setApplicationProxy(proxy);
}

} // namespace quentier
