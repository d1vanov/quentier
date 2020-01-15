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
#include <lib/update/UpdateInfo.h>

#include <quentier/logging/QuentierLogger.h>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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
    QNDEBUG("GitHubUpdateChecker::onReleasesListed: status = "
        << (status ? "true" : "false") << ", error description = "
        << errorDescription << ", fetched data size = " << fetchedData.size());

    if (Q_UNLIKELY(!status))
    {
        ErrorString error(QT_TR_NOOP("Failed to list releases from GitHub"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING(error);
        Q_EMIT failure(error);
        return;
    }

    QDateTime currentBuildCreationTime = QDateTime::fromString(
        QString::fromUtf8(QUENTIER_BUILD_TIMESTAMP),
        Qt::ISODate);
    if (Q_UNLIKELY(!currentBuildCreationTime.isValid()))
    {
        ErrorString error(
            QT_TR_NOOP("Failed to parse current build creation time from string"));
        error.details() = QString::fromUtf8(QUENTIER_BUILD_TIMESTAMP);
        QNWARNING(error);
        Q_EMIT failure(error);
    }

    QJsonParseError jsonParseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(
        fetchedData,
        &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError)
    {
        ErrorString error(
            QT_TR_NOOP("Failed to parse list releases response from GitHub to "
                       "json"));
        QNWARNING(error);
        Q_EMIT failure(error);
        return;
    }

    QJsonArray releases = jsonDoc.array();
    for(const auto & release: releases)
    {
        if (Q_UNLIKELY(!release.isObject())) {
            QNINFO("Skipping json field which is not an object although it "
                << "should be a GitHub release: " <<  release);
            continue;
        }

        auto releaseObject = release.toObject();

        auto prereleaseValue = releaseObject.value(QStringLiteral("prerelease"));
        if (Q_UNLIKELY(prereleaseValue == QJsonValue::Undefined)) {
            QNWARNING("GitHub release field has no prerelease field: "
                << release);
            continue;
        }

        auto nameValue = releaseObject.value(QStringLiteral("name"));
        if (Q_UNLIKELY(nameValue == QJsonValue::Undefined)) {
            QNWARNING("GitHub release field has no name field: " << release);
            continue;
        }

        if (prereleaseValue.toBool() &&
            nameValue.toString().contains(QStringLiteral("continuous-")) &&
            !m_useContinuousUpdateChannel)
        {
            QNDEBUG("Skipping release " << nameValue.toString()
                << " as checking for continuous releases is switched off");
            continue;
        }

        auto createdAtValue = releaseObject.value(QStringLiteral("created_at"));
        if (Q_UNLIKELY(createdAtValue == QJsonValue::Undefined))
        {
            QNWARNING("GitHub release field has no created_at field: "
                << release);
            continue;
        }

        QDateTime createdAt = QDateTime::fromString(
            createdAtValue.toString(),
            Qt::ISODate);
        if (!createdAt.isValid())
        {
            QNWARNING("Failed to parse datetime from created_at field of "
                << "GitHub release: " << createdAtValue.toString());
            continue;
        }

        // TODO: impement further
    }

    // TODO: implement further
}

} // namespace quentier
