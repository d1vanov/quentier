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

#include <QRegExp>
#include <QXmlStreamReader>

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
    clear();
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

    QUrl url;
    ErrorString errorDescription;
    if (!composePageIdFetchingUrl(url, errorDescription)) {
        finishWithError(errorDescription);
        return;
    }

    m_pApiUrlFetcher = new NetworkReplyFetcher(m_pNetworkAccessManager, url);

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

    // Page id fetching has 10% of total progress reserved for it so scaling
    // the numbers
    double progressValue = 0.1 * static_cast<double>(bytesFetched) /
        std::max(bytesTotal, qint64(0));

    Q_EMIT progress(progressValue);
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

bool WikiArticleFetcher::composePageIdFetchingUrl(
    QUrl & url, ErrorString & errorDescription) const
{
    QString urlString = m_url.toString();

    QRegExp regex(QStringLiteral("^https?:\\/\\/\\w{2,}\\.wikipedia\\.org\\/wiki\\/(\\w+)$"));
    int pos = regex.indexIn(urlString);
    if (Q_UNLIKELY(pos < 0)) {
        errorDescription.setBase(QT_TR_NOOP("Can't parse the input URL"));
        errorDescription.details() = urlString;
        WAWARNING(errorDescription);
        return false;
    }

    QStringList capturedTexts = regex.capturedTexts();
    if (capturedTexts.size() < 3) {
        errorDescription.setBase(QT_TR_NOOP("Can't parse the input URL"));
        errorDescription.details() = urlString;
        WAWARNING(errorDescription);
        return false;
    }

    QString languageCode = capturedTexts[1];
    QString articleName = capturedTexts[2];

    QString apiUrl = QStringLiteral("https://");
    apiUrl += languageCode;
    apiUrl += QStringLiteral(".wikipedia.org/w/api.php?action=query&format=xml&titles=");
    apiUrl += articleName;

    url = QUrl(apiUrl);
    return true;
}

qint32 WikiArticleFetcher::parsePageIdFromFetchedData(
    const QByteArray & fetchedData, ErrorString & errorDescription)
{
    qint32 pageId = -1;

    QXmlStreamReader reader(fetchedData);
    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext())

        if (reader.isStartElement() &&
            (reader.name().toString() == QStringLiteral("page")))
        {
            QXmlStreamAttributes attributes = reader.attributes();
            QStringRef pageIdValue = attributes.value(QStringLiteral("pageid"));
            bool conversionResult = false;
            pageId = pageIdValue.toInt(&conversionResult);
            if (!conversionResult) {
                errorDescription.setBase(QT_TR_NOOP("Failed to fetch page id"));
                WAWARNING(errorDescription << QStringLiteral("; page id value = ")
                          << pageIdValue);
                return -1;
            }

            break;
        }
    }

    if (pageId < 0) {
        errorDescription.setBase(QT_TR_NOOP("Failed to fetch page id"));
        WAWARNING(errorDescription << QStringLiteral("; fetched data: ")
                  << fetchedData);
        return -1;
    }

    return pageId;
}

void WikiArticleFetcher::finishWithError(const ErrorString & errorDescription)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::finishWithError: ")
            << errorDescription);

    clear();
    Q_EMIT failure(errorDescription);
}

void WikiArticleFetcher::clear()
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::clear"));

    m_started = false;
    m_finished = false;
    m_note = Note();

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

} // namespace quentier
