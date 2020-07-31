/*
 * Copyright 2019-2020 Dmitry Ivanov
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

#include "WikiRandomArticleFetcher.h"

#include <lib/network/NetworkReplyFetcher.h>

#include <quentier/logging/QuentierLogger.h>

namespace quentier {

WikiRandomArticleFetcher::WikiRandomArticleFetcher(
        const qint64 timeoutMsec,
        QObject * parent) :
    QObject(parent),
    m_enmlConverter(),
    m_networkReplyFetcherTimeout(timeoutMsec)
{}

WikiRandomArticleFetcher::~WikiRandomArticleFetcher()
{
    clear();
}

void WikiRandomArticleFetcher::start()
{
    QNDEBUG("wiki2account", "WikiRandomArticleFetcher::start");

    if (Q_UNLIKELY(m_started)) {
        QNWARNING("wiki2account", "WikiRandomArticleFetcher is already "
            << "started");
        return;
    }

    m_pWikiArticleUrlFetcher = new WikiRandomArticleUrlFetcher(
        m_networkReplyFetcherTimeout);

    QObject::connect(
        m_pWikiArticleUrlFetcher,
        &WikiRandomArticleUrlFetcher::progress,
        this,
        &WikiRandomArticleFetcher::onRandomArticleUrlFetchProgress);

    QObject::connect(
        m_pWikiArticleUrlFetcher,
        &WikiRandomArticleUrlFetcher::finished,
        this,
        &WikiRandomArticleFetcher::onRandomArticleUrlFetchFinished);

    m_pWikiArticleUrlFetcher->start();
    m_started = true;
}

void WikiRandomArticleFetcher::onRandomArticleUrlFetchProgress(
    double percentage)
{
    QNDEBUG(
        "wiki2account",
        "WikiRandomArticleFetcher::onRandomArticleUrlFetchProgress: "
            << percentage);

    // Downloading the article's URL is considered only 10% of total progress
    Q_EMIT progress(0.1 * percentage);
}

void WikiRandomArticleFetcher::onRandomArticleUrlFetchFinished(
    bool status, QUrl randomArticleUrl, ErrorString errorDescription)
{
    QNDEBUG(
        "wiki2account",
        "WikiRandomArticleFetcher::onRandomArticleUrlFetchFinished: "
            << (status ? "success" : "failure") << ", url = "
            << randomArticleUrl << ", error description = "
            << errorDescription);

    if (m_pWikiArticleUrlFetcher) {
        m_pWikiArticleUrlFetcher->disconnect(this);
        m_pWikiArticleUrlFetcher->deleteLater();
        m_pWikiArticleUrlFetcher = nullptr;
    }

    if (!status)
    {
        clear();
        QNWARNING("wiki2account", "Failed to fetch random wiki article's URL: "
            << errorDescription);
        Q_EMIT failure(errorDescription);
        return;
    }

    m_url = randomArticleUrl;

    QNDEBUG("wiki2account", "Starting to fetch wiki article content: "
        << m_url);

    m_pWikiArticleContentsFetcher = new NetworkReplyFetcher(
        m_url,
        m_networkReplyFetcherTimeout);

    QObject::connect(
        m_pWikiArticleContentsFetcher,
        &NetworkReplyFetcher::finished,
        this,
        &WikiRandomArticleFetcher::onWikiArticleDownloadFinished);

    QObject::connect(
        m_pWikiArticleContentsFetcher,
        &NetworkReplyFetcher::downloadProgress,
        this,
        &WikiRandomArticleFetcher::onWikiArticleDownloadProgress);

    m_pWikiArticleContentsFetcher->start();
}

void WikiRandomArticleFetcher::onWikiArticleDownloadProgress(
    qint64 bytesFetched, qint64 bytesTotal)
{
    QNDEBUG(
        "wiki2account",
        "WikiRandomArticleFetcher::onWikiArticleDownloadProgress: "
            << bytesFetched << " bytes fetched out of " << bytesTotal);

    if (bytesTotal < 0) {
        // The exact number of bytes to download is not known
        return;
    }

    // Downloading the article's contents is considered 60% of total progress
    // 10% of progress was reserved for fetching random article's URL
    Q_EMIT progress(
        0.1 +
        0.6 *
        (static_cast<double>(bytesFetched) / std::max(bytesTotal, qint64(1))));
}

void WikiRandomArticleFetcher::onWikiArticleDownloadFinished(
    bool status, QByteArray fetchedData, ErrorString errorDescription)
{
    QNDEBUG(
        "wiki2account",
        "WikiRandomArticleFetcher::onWikiArticleDownloadFinished: "
            << (status ? "success" : "failure")
            << ", error description = " << errorDescription);

    if (m_pWikiArticleContentsFetcher) {
        m_pWikiArticleContentsFetcher->disconnect(this);
        m_pWikiArticleContentsFetcher->deleteLater();
        m_pWikiArticleContentsFetcher = nullptr;
    }

    if (!status)
    {
        clear();
        QNWARNING("wiki2account", "Failed to fetch random wiki article's "
            << "contents: " << errorDescription << "; url = " << m_url);
        Q_EMIT failure(errorDescription);
        return;
    }

    m_pWikiArticleToNote = new WikiArticleToNote(
        m_enmlConverter,
        m_networkReplyFetcherTimeout);

    QObject::connect(
        m_pWikiArticleToNote,
        &WikiArticleToNote::progress,
        this,
        &WikiRandomArticleFetcher::onWikiArticleToNoteProgress);

    QObject::connect(
        m_pWikiArticleToNote,
        &WikiArticleToNote::finished,
        this,
        &WikiRandomArticleFetcher::onWikiArticleToNoteFinished);

    m_pWikiArticleToNote->start(fetchedData);
}

void WikiRandomArticleFetcher::onWikiArticleToNoteProgress(double percentage)
{
    QNDEBUG(
        "wiki2account",
        "WikiRandomArticleFetcher::onWikiArticleToNoteProgress: "
            << percentage);

    // Converting the article to note takes the remaining 30% of total progress
    // after downloading random wiki article's URL and after downloading note's
    // contents
    Q_EMIT progress(0.7 + 0.3 * percentage);
}

void WikiRandomArticleFetcher::onWikiArticleToNoteFinished(
    bool status, ErrorString errorDescription, Note note)
{
    QNDEBUG(
        "wiki2account",
        "WikiRandomArticleFetcher::onWikiArticleToNoteFinished: "
            << (status ? "success" : "failure")
            << ", error description = " << errorDescription);

    if (m_pWikiArticleToNote) {
        m_pWikiArticleToNote->disconnect(this);
        m_pWikiArticleToNote->deleteLater();
        m_pWikiArticleToNote = nullptr;
    }

    if (!status)
    {
        clear();
        QNWARNING("wiki2account", "Failed to convert wiki article's contents "
            << "to note: " << errorDescription);
        Q_EMIT failure(errorDescription);
        return;
    }

    QNTRACE("wiki2account", note);
    m_note = note;

    m_started = false;
    m_finished = true;
    Q_EMIT finished();
}

void WikiRandomArticleFetcher::clear()
{
    QNDEBUG("wiki2account", "WikiRandomArticleFetcher::clear");

    m_url = QUrl();
    m_note = Note();

    if (m_pWikiArticleToNote) {
        m_pWikiArticleToNote->disconnect(this);
        m_pWikiArticleToNote->deleteLater();
        m_pWikiArticleToNote = nullptr;
    }

    if (m_pWikiArticleContentsFetcher) {
        m_pWikiArticleContentsFetcher->disconnect(this);
        m_pWikiArticleContentsFetcher->deleteLater();
        m_pWikiArticleContentsFetcher = nullptr;
    }

    if (m_pWikiArticleUrlFetcher) {
        m_pWikiArticleUrlFetcher->disconnect(this);
        m_pWikiArticleUrlFetcher->deleteLater();
        m_pWikiArticleUrlFetcher = nullptr;
    }
}

} // namespace quentier
