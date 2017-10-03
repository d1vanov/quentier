/*
 * Copyright 2017 Dmitry Ivanov
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

#ifndef QUENTIER_PARSE_NETWORK_PROXY_SETTINGS_H
#define QUENTIER_PARSE_NETWORK_PROXY_SETTINGS_H

#include <QNetworkProxy>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)

/**
 * @brief parseNetworkProxySettings     Helper function collecting and returning network proxy settings
 *                                      read from application settings
 *
 * @param currentAccount                Current account; if empty, application-wise settings are parsed
 * @param type                          Network proxy type
 * @param host                          Network proxy host
 * @param port                          Network proxy port
 * @param user                          Network proxy username
 * @param password                      Network proxy password
 */
void parseNetworkProxySettings(const Account & currentAccount,
                               QNetworkProxy::ProxyType & type, QString & host, int & port,
                               QString & user, QString & password);

} // namespace quentier

#endif // QUENTIER_PARSE_NETWORK_PROXY_SETTINGS_H
