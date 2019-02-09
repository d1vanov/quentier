/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_NETWORK_PROXY_SETTINGS_HELPERS_H
#define QUENTIER_NETWORK_PROXY_SETTINGS_HELPERS_H

#include <QNetworkProxy>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)

/**
 * @brief parseNetworkProxySettings     Helper function collecting and returning
 *                                      network proxy settings read from
 *                                      application settings
 *
 * @param currentAccount                Current account; if empty,
 *                                      application-wise settings are parsed
 * @param type                          Network proxy type
 * @param host                          Network proxy host
 * @param port                          Network proxy port
 * @param user                          Network proxy username
 * @param password                      Network proxy password
 */
void parseNetworkProxySettings(const Account & currentAccount,
                               QNetworkProxy::ProxyType & type,
                               QString & host, int & port,
                               QString & user, QString & password);
/**
 * @brief persistNetworkProxySettingsForAccount     Helper function persisting
 *                                                  the settings of provided
 *                                                  proxy for the given account
 *
 * @param account                                   Account for which the settings
 *                                                  are persisted; if empty,
 *                                                  the application-wise settings
 *                                                  are persisted
 * @param proxy                                     Network proxy which setttings
 *                                                  are to be persisted
 */
void persistNetworkProxySettingsForAccount(const Account & account,
                                           const QNetworkProxy & proxy);

/**
 * @brief restoreNetworkProxySettingsForAccount     Helper function setting
 *                                                  the application network proxy
 *                                                  built from persisted settings
 *                                                  for the given account
 *                                                  or application-wise settings
 *                                                  if the passed in account is
 *                                                  empty
 * @param account                                   Account for which the network
 *                                                  proxy settings are to be
 *                                                  restored
 */
void restoreNetworkProxySettingsForAccount(const Account & account);

} // namespace quentier

#endif // QUENTIER_NETWORK_PROXY_SETTINGS_HELPERS_H
