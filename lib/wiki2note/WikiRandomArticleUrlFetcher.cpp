/*
 * Copyright 2019-2021 Dmitry Ivanov
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

#include "WikiRandomArticleUrlFetcher.h"

#include <quentier/logging/QuentierLogger.h>

#include <QXmlStreamReader>

#include <algorithm>

namespace quentier {

WikiRandomArticleUrlFetcher::WikiRandomArticleUrlFetcher(
    const qint64 timeoutMsec, QObject * parent) :
    QObject(parent),
    m_networkReplyFetcherTimeout(timeoutMsec)
{}

WikiRandomArticleUrlFetcher::~WikiRandomArticleUrlFetcher()
{
    if (m_pNetworkReplyFetcher) {
        m_pNetworkReplyFetcher->disconnect(this);
        m_pNetworkReplyFetcher->deleteLater();
    }
}

void WikiRandomArticleUrlFetcher::start()
{
    QNDEBUG("wiki2note", "WikiRandomArticleUrlFetcher::start");

    if (Q_UNLIKELY(m_started)) {
        QNWARNING(
            "wiki2note", "WikiRandomArticleUrlFetcher is already started");
        return;
    }

    QUrl query(
        QStringLiteral("https://en.wikipedia.org/w/api.php"
                       "?action=query"
                       "&format=xml"
                       "&list=random"
                       "&rnlimit=1"
                       "&rnnamespace=0"));

    m_pNetworkReplyFetcher =
        new NetworkReplyFetcher(query, m_networkReplyFetcherTimeout);

    QObject::connect(
        this, &WikiRandomArticleUrlFetcher::startFetching,
        m_pNetworkReplyFetcher, &NetworkReplyFetcher::start);

    QObject::connect(
        m_pNetworkReplyFetcher, &NetworkReplyFetcher::finished, this,
        &WikiRandomArticleUrlFetcher::onDownloadFinished);

    QObject::connect(
        m_pNetworkReplyFetcher, &NetworkReplyFetcher::downloadProgress, this,
        &WikiRandomArticleUrlFetcher::onDownloadProgress);

    m_pNetworkReplyFetcher->start();
    m_started = true;
}

void WikiRandomArticleUrlFetcher::onDownloadProgress(
    qint64 bytesFetched, qint64 bytesTotal)
{
    QNDEBUG(
        "wiki2note",
        "WikiRandomArticleUrlFetcher::onDownloadProgress: "
            << "fetched " << bytesFetched << " bytes, total " << bytesTotal
            << " bytes");

    double percentage = 0;
    if (bytesTotal != 0) {
        percentage =
            static_cast<double>(bytesFetched) / static_cast<double>(bytesTotal);
        percentage = std::max(percentage, 1.0);
    }

    Q_EMIT progress(percentage);
}

void WikiRandomArticleUrlFetcher::onDownloadFinished(
    bool status, QByteArray fetchedData, // NOLINT
    ErrorString errorDescription)
{
    QNDEBUG(
        "wiki2note",
        "WikiRandomArticleUrlFetcher::onDownloadFinished: "
            << "status = " << (status ? "true" : "false")
            << ", error description = " << errorDescription);

    if (!status) {
        QNWARNING("wiki2note", "Download failed: " << errorDescription);
        finishWithError(errorDescription);
        return;
    }

    errorDescription.clear();

    const qint32 pageId =
        parsePageIdFromFetchedData(fetchedData, errorDescription);

    if (pageId < 0) {
        finishWithError(errorDescription);
        return;
    }

    m_url = QUrl{
        QStringLiteral("https://en.wikipedia.org/w/api.php"
                       "?action=parse"
                       "&format=xml"
                       "&section=0"
                       "&prop=text"
                       "&pageid=") +
        QString::number(pageId)};

    if (!m_url.isValid()) {
        errorDescription.setBase(
            QT_TR_NOOP("Failed to compose valid URL from data fetched from "
                       "Wiki"));
        QNWARNING("wiki2note", errorDescription << ", page id = " << pageId);
        finishWithError(errorDescription);
        return;
    }

    QNDEBUG("wiki2note", "Successfully composed random article URL: " << m_url);

    m_started = false;
    m_finished = true;
    Q_EMIT finished(true, m_url, ErrorString());
}

qint32 WikiRandomArticleUrlFetcher::parsePageIdFromFetchedData(
    const QByteArray & fetchedData, ErrorString & errorDescription)
{
    qint32 pageId = 0;
    bool foundPageId = false;

    QXmlStreamReader reader(fetchedData);

    while (!reader.atEnd()) {
        Q_UNUSED(reader.readNext());

        if (reader.isStartElement() &&
            (reader.name().toString() == QStringLiteral("page")))
        {
            const auto attributes = reader.attributes();

            const QString id =
                attributes.value(QStringLiteral("id")).toString();

            bool conversionResult = false;
            pageId = id.toInt(&conversionResult);
            if (!conversionResult) {
                errorDescription.setBase(
                    QT_TR_NOOP("Failed to fetch random Wiki article URL: could "
                               "not convert id property to int"));
                QNWARNING("wiki2note", errorDescription << ": " << id);
                return -1;
            }

            foundPageId = true;
            break;
        }
    }

    if (!foundPageId) {
        errorDescription.setBase(
            QT_TR_NOOP("Failed to fetch random Wiki article URL: could not "
                       "find page id"));
        QNWARNING("wiki2note", errorDescription);
        return -1;
    }

    return pageId;
}

void WikiRandomArticleUrlFetcher::finishWithError(
    const ErrorString & errorDescription)
{
    QNDEBUG(
        "wiki2note",
        "WikiRandomArticleUrlFetcher::finishWithError: " << errorDescription);

    m_started = false;
    m_finished = false;

    if (m_pNetworkReplyFetcher) {
        m_pNetworkReplyFetcher->disconnect(this);
        m_pNetworkReplyFetcher->deleteLater();
        m_pNetworkReplyFetcher = nullptr;
    }

    Q_EMIT finished(false, QUrl(), errorDescription);
}

} // namespace quentier
