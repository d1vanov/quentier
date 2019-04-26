/*
 * Copyright 2019 Dmitry Ivanov
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

#include "WikiArticleFetcher.h"

#include <lib/network/NetworkReplyFetcher.h>

#include <quentier/logging/QuentierLogger.h>

#define WALOG_IMPL(message, macro) \
    macro(QStringLiteral("<") << m_url << QStringLiteral(">: ") << message)

#define WATRACE(message) \
    WALOG_IMPL(message, QNTRACE)

#define WADEBUG(message) \
    WALOG_IMPL(message, QNDEBUG)

#define WAINFO(message) \
    WALOG_IMPL(message, QNINFO)

#define WAWARNING(message) \
    WALOG_IMPL(message, QNWARNING)

#define WACRITICAL(message) \
    WALOG_IMPL(message, QNCRITICAL)

#define WAFATAL(message) \
    WALOG_IMPL(message, QNFATAL)

namespace quentier {

WikiArticleFetcher::WikiArticleFetcher(
        QNetworkAccessManager * pNetworkAccessManager,
        const QUrl & url,
        QObject * parent) :
    QObject(parent),
    m_pNetworkAccessManager(pNetworkAccessManager),
    m_url(url),
    m_started(false),
    m_finished(false),
    m_note(),
    m_pApiUrlFetcher(Q_NULLPTR),
    m_enmlConverter(),
    m_pWikiArticleToNote(Q_NULLPTR)
{}

WikiArticleFetcher::~WikiArticleFetcher()
{
    if (m_pApiUrlFetcher) {
        m_pApiUrlFetcher->disconnect(this);
        m_pApiUrlFetcher->deleteLater();
        m_pApiUrlFetcher = Q_NULLPTR;
    }

    if (m_pWikiArticleToNote) {
        m_pWikiArticleToNote->disconnect(this);
        m_pWikiArticleToNote->deleteLater();
        m_pWikiArticleToNote = Q_NULLPTR;
    }
}

void WikiArticleFetcher::start()
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::start"));

    if (m_started) {
        WADEBUG(QStringLiteral("Already started"));
        return;
    }

    m_started = true;
    m_finished = false;

    m_pApiUrlFetcher = new NetworkReplyFetcher(m_pNetworkAccessManager, m_url);

    QObject::connect(m_pApiUrlFetcher,
                     QNSIGNAL(NetworkReplyFetcher,finished,
                              bool,QByteArray,ErrorString),
                     this,
                     QNSLOT(WikiArticleFetcher,onPageIdFetchingFinished,
                            bool,QByteArray,ErrorString));

    QObject::connect(m_pApiUrlFetcher,
                     QNSIGNAL(NetworkReplyFetcher,downloadProgress,qint64,qint64),
                     this,
                     QNSLOT(WikiArticleFetcher,onPageIdFetchingProgress,
                            qint64,qint64));
}

void WikiArticleFetcher::onPageIdFetchingProgress(qint64 bytesFetched,
                                                  qint64 bytesTotal)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onPageIdFetchingProgress: ")
            << QStringLiteral("fetched ") << bytesFetched
            << QStringLiteral(" out of ") << bytesTotal);

    // TODO: convert to percentage within 10% devoted to page id fetching from
    // total progress
}

void WikiArticleFetcher::onPageIdFetchingFinished(
    bool status, QByteArray fetchedData, ErrorString errorDescription)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onPageIdFetchingFinished: status = ")
            << (status ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription);

    if (!status) {
        finishWithError(errorDescription);
        return;
    }

    // TODO: compose API url and start WikiArticleToNote
}

void WikiArticleFetcher::onWikiArticleToNoteProgress(double progress)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onWikiArticleToNoteProgress: ")
            << progress);

    // TODO: renormalize the progress and emit progress signal
}

void WikiArticleFetcher::onWikiArticleToNoteFinished(
    bool status, ErrorString errorDescription, Note note)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onWikiArticleToNoteFinished: ")
            << QStringLiteral("status = ")
            << (status ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription);

    if (!status) {
        finishWithError(errorDescription);
        return;
    }

    WATRACE(note);
    m_note = note;

    Q_EMIT finished();
}

} // namespace quentier
