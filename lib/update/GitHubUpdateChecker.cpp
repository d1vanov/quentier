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

#include "GitHubUpdateChecker.h"

#include <lib/network/NetworkReplyFetcher.h>

#include <quentier/logging/QuentierLogger.h>

namespace quentier {

GitHubUpdateChecker::GitHubUpdateChecker(QObject * parent) :
    IUpdateChecker(parent)
{}

void GitHubUpdateChecker::checkForUpdates()
{
    QNDEBUG("GitHubUpdateChecker::checkForUpdates");

    if (m_inProgress) {
        QNDEBUG("Checking for updates is already in progress");
        return;
    }

    QUrl url;
    url.setHost(m_host);
    url.setScheme(m_scheme);
    url.setPath(QStringLiteral("repos/d1vanov/quentier/releases"));

    auto * pListReleasesReplyFetcher = new NetworkReplyFetcher(
        &m_nam,
        url,
        NETWORK_REPLY_FETCHER_DEFAULT_TIMEOUT_MSEC,
        this);

    QObject::connect(
        pListReleasesReplyFetcher,
        &NetworkReplyFetcher::finished,
        this,
        &GitHubUpdateChecker::onReleasesListed);
    pListReleasesReplyFetcher->start();
}

void GitHubUpdateChecker::onReleasesListed(
    bool status, QByteArray fetchedData, ErrorString errorDescription)
{
    // TODO: implement
    Q_UNUSED(status)
    Q_UNUSED(fetchedData)
    Q_UNUSED(errorDescription)
}

} // namespace quentier
