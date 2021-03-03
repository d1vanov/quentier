/*
 * Copyright 2020-2021 Dmitry Ivanov
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

class QJsonDocument;
class QJsonObject;

namespace quentier {

class GitHubUpdateChecker : public IUpdateChecker
{
    Q_OBJECT
public:
    explicit GitHubUpdateChecker(QObject * parent = nullptr);

    ~GitHubUpdateChecker() override;

    [[nodiscard]] QString host() const
    {
        return m_host;
    }

    void setHost(QString host)
    {
        m_host = std::move(host);
    }

    [[nodiscard]] QString scheme() const
    {
        return m_scheme;
    }

    void setScheme(QString scheme)
    {
        m_scheme = std::move(scheme);
    }

public Q_SLOTS:
    void checkForUpdates() override;

private Q_SLOTS:
    void onReleasesListed(
        bool status, QByteArray fetchedData, ErrorString errorDescription);

private:
    struct GitHubReleaseInfo
    {
        GitHubReleaseInfo(QUrl url, QDateTime createdAt) :
            m_htmlUrl(std::move(url)), m_createdAt(std::move(createdAt))
        {}

        QUrl m_htmlUrl;
        QDateTime m_createdAt;
    };

    void parseListedReleases(const QJsonDocument & jsonDoc);

    [[nodiscard]] bool checkReleaseAssets(
        const QJsonObject & releaseObject) const;

private:
    QString m_host;
    QString m_scheme;

    QDateTime m_currentBuildCreationDateTime;

    bool m_inProgress = false;

    std::unique_ptr<GitHubReleaseInfo> m_pLatestReleaseInfo;
};

} // namespace quentier

#endif // QUENTIER_UPDATE_GITHUB_UPDATE_CHECKER_H
