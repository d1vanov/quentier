/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#include <quentier/enml/Factory.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

WikiRandomArticleFetcher::WikiRandomArticleFetcher(
    const qint64 timeoutMsec, QObject * parent) :
    QObject{parent}, m_enmlConverter{enml::createConverter()},
    m_networkReplyFetcherTimeout{timeoutMsec}
{}

WikiRandomArticleFetcher::~WikiRandomArticleFetcher()
{
    clear();
}

void WikiRandomArticleFetcher::start()
{
    QNDEBUG(
        "wiki2account::WikiRandomArticleFetcher",
        "WikiRandomArticleFetcher::start");

    if (Q_UNLIKELY(m_started)) {
        QNWARNING(
            "wiki2account::WikiRandomArticleFetcher",
            "WikiRandomArticleFetcher is already started");
        return;
    }

    m_wikiArticleUrlFetcher =
        new WikiRandomArticleUrlFetcher{m_networkReplyFetcherTimeout};

    QObject::connect(
        m_wikiArticleUrlFetcher, &WikiRandomArticleUrlFetcher::progress, this,
        &WikiRandomArticleFetcher::onRandomArticleUrlFetchProgress);

    QObject::connect(
        m_wikiArticleUrlFetcher, &WikiRandomArticleUrlFetcher::finished, this,
        &WikiRandomArticleFetcher::onRandomArticleUrlFetchFinished);

    m_wikiArticleUrlFetcher->start();
    m_started = true;
}

void WikiRandomArticleFetcher::onRandomArticleUrlFetchProgress(
    const double percentage)
{
    QNDEBUG(
        "wiki2account::WikiRandomArticleFetcher",
        "WikiRandomArticleFetcher::onRandomArticleUrlFetchProgress: "
            << percentage);

    // Downloading the article's URL is considered only 10% of total progress
    Q_EMIT progress(0.1 * percentage);
}

void WikiRandomArticleFetcher::onRandomArticleUrlFetchFinished(
    const bool status, QUrl randomArticleUrl, ErrorString errorDescription)
{
    QNDEBUG(
        "wiki2account::WikiRandomArticleFetcher",
        "WikiRandomArticleFetcher::onRandomArticleUrlFetchFinished: "
            << (status ? "success" : "failure")
            << ", url = " << randomArticleUrl
            << ", error description = " << errorDescription);

    if (m_wikiArticleUrlFetcher) {
        m_wikiArticleUrlFetcher->disconnect(this);
        m_wikiArticleUrlFetcher->deleteLater();
        m_wikiArticleUrlFetcher = nullptr;
    }

    if (!status) {
        clear();
        QNWARNING(
            "wiki2account::WikiRandomArticleFetcher",
            "Failed to fetch random wiki article's URL: " << errorDescription);
        Q_EMIT failure(std::move(errorDescription));
        return;
    }

    m_url = std::move(randomArticleUrl);

    QNDEBUG(
        "wiki2account::WikiRandomArticleFetcher",
        "Starting to fetch wiki article content: " << m_url);

    m_wikiArticleContentsFetcher =
        new NetworkReplyFetcher{m_url, m_networkReplyFetcherTimeout};

    QObject::connect(
        m_wikiArticleContentsFetcher, &NetworkReplyFetcher::finished, this,
        &WikiRandomArticleFetcher::onWikiArticleDownloadFinished);

    QObject::connect(
        m_wikiArticleContentsFetcher, &NetworkReplyFetcher::downloadProgress,
        this, &WikiRandomArticleFetcher::onWikiArticleDownloadProgress);

    m_wikiArticleContentsFetcher->start();
}

void WikiRandomArticleFetcher::onWikiArticleDownloadProgress(
    qint64 bytesFetched, qint64 bytesTotal)
{
    QNDEBUG(
        "wiki2account::WikiRandomArticleFetcher",
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
            (static_cast<double>(bytesFetched) /
             static_cast<double>(std::max(bytesTotal, qint64(1)))));
}

void WikiRandomArticleFetcher::onWikiArticleDownloadFinished(
    bool status, QByteArray fetchedData, ErrorString errorDescription)
{
    QNDEBUG(
        "wiki2account::WikiRandomArticleFetcher",
        "WikiRandomArticleFetcher::onWikiArticleDownloadFinished: "
            << (status ? "success" : "failure")
            << ", error description = " << errorDescription);

    if (m_wikiArticleContentsFetcher) {
        m_wikiArticleContentsFetcher->disconnect(this);
        m_wikiArticleContentsFetcher->deleteLater();
        m_wikiArticleContentsFetcher = nullptr;
    }

    if (!status) {
        clear();
        QNWARNING(
            "wiki2account::WikiRandomArticleFetcher",
            "Failed to fetch random wiki article's contents: "
                << errorDescription << "; url = " << m_url);
        Q_EMIT failure(errorDescription);
        return;
    }

    m_wikiArticleToNote =
        new WikiArticleToNote{m_enmlConverter, m_networkReplyFetcherTimeout};

    QObject::connect(
        m_wikiArticleToNote, &WikiArticleToNote::progress, this,
        &WikiRandomArticleFetcher::onWikiArticleToNoteProgress);

    QObject::connect(
        m_wikiArticleToNote, &WikiArticleToNote::finished, this,
        &WikiRandomArticleFetcher::onWikiArticleToNoteFinished);

    m_wikiArticleToNote->start(fetchedData);
}

void WikiRandomArticleFetcher::onWikiArticleToNoteProgress(
    const double percentage)
{
    QNDEBUG(
        "wiki2account::WikiRandomArticleFetcher",
        "WikiRandomArticleFetcher::onWikiArticleToNoteProgress: "
            << percentage);

    // Converting the article to note takes the remaining 30% of total progress
    // after downloading random wiki article's URL and after downloading note's
    // contents
    Q_EMIT progress(0.7 + 0.3 * percentage);
}

void WikiRandomArticleFetcher::onWikiArticleToNoteFinished(
    const bool status, ErrorString errorDescription, qevercloud::Note note)
{
    QNDEBUG(
        "wiki2account::WikiRandomArticleFetcher",
        "WikiRandomArticleFetcher::onWikiArticleToNoteFinished: "
            << (status ? "success" : "failure")
            << ", error description = " << errorDescription);

    if (m_wikiArticleToNote) {
        m_wikiArticleToNote->disconnect(this);
        m_wikiArticleToNote->deleteLater();
        m_wikiArticleToNote = nullptr;
    }

    if (!status) {
        clear();
        QNWARNING(
            "wiki2account::WikiRandomArticleFetcher",
            "Failed to convert wiki article's contents "
                << "to note: " << errorDescription);
        Q_EMIT failure(std::move(errorDescription));
        return;
    }

    QNTRACE("wiki2account::WikiRandomArticleFetcher", note);
    m_note = std::move(note);

    m_started = false;
    m_finished = true;
    Q_EMIT finished();
}

void WikiRandomArticleFetcher::clear()
{
    QNDEBUG(
        "wiki2account::WikiRandomArticleFetcher",
        "WikiRandomArticleFetcher::clear");

    m_url = QUrl{};
    m_note = qevercloud::Note{};

    if (m_wikiArticleToNote) {
        m_wikiArticleToNote->disconnect(this);
        m_wikiArticleToNote->deleteLater();
        m_wikiArticleToNote = nullptr;
    }

    if (m_wikiArticleContentsFetcher) {
        m_wikiArticleContentsFetcher->disconnect(this);
        m_wikiArticleContentsFetcher->deleteLater();
        m_wikiArticleContentsFetcher = nullptr;
    }

    if (m_wikiArticleUrlFetcher) {
        m_wikiArticleUrlFetcher->disconnect(this);
        m_wikiArticleUrlFetcher->deleteLater();
        m_wikiArticleUrlFetcher = nullptr;
    }
}

} // namespace quentier
