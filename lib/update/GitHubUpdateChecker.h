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

#ifndef QUENTIER_UPDATE_GITHUB_UPDATE_CHECKER_H
#define QUENTIER_UPDATE_GITHUB_UPDATE_CHECKER_H

#include "IUpdateChecker.h"

#include <QDateTime>
#include <QNetworkAccessManager>

#include <memory>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NetworkReplyFetcher)

class GitHubUpdateChecker: public IUpdateChecker
{
    Q_OBJECT
public:
    explicit GitHubUpdateChecker(QObject * parent = nullptr);

    virtual ~GitHubUpdateChecker() = default;

    QString host() const { return m_host; }
    void setHost(QString host) { m_host = std::move(host); }

    QString scheme() const { return m_scheme; }
    void setScheme(QString scheme) { m_scheme = std::move(scheme); }

public Q_SLOTS:
    virtual void checkForUpdates() override;

private Q_SLOTS:
    void onReleasesListed(
        bool status, QByteArray fetchedData, ErrorString errorDescription);

private:
    struct GitHubReleaseInfo
    {
        QUrl        m_htmlUrl;
        QDateTime   m_publishedAt;
    };

private:
    QString     m_host;
    QString     m_scheme;

    bool        m_inProgress = false;

    QNetworkAccessManager   m_nam;

    NetworkReplyFetcher *   m_pListReleasesReplyFetcher = nullptr;

    std::unique_ptr<GitHubReleaseInfo>  m_pLatestReleaseInfo;
};

} // namespace quentier

#endif // QUENTIER_UPDATE_GITHUB_UPDATE_CHECKER_H
