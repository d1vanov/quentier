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
        ENMLConverter & enmlConverter,
        const QUrl & url,
        QObject * parent) :
    QObject(parent),
    m_pNetworkAccessManager(pNetworkAccessManager),
    m_enmlConverter(enmlConverter),
    m_url(url),
    m_started(false),
    m_finished(false),
    m_note(),
    m_pApiUrlFetcher(Q_NULLPTR),
    m_languageCode(),
    m_articleTitle(),
    m_pArticleContentsFetcher(Q_NULLPTR),
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

    m_pApiUrlFetcher->start();
}

void WikiArticleFetcher::onPageIdFetchingProgress(qint64 bytesFetched,
                                                  qint64 bytesTotal)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onPageIdFetchingProgress: ")
            << QStringLiteral("fetched ") << bytesFetched
            << QStringLiteral(" out of ") << bytesTotal);

    if (bytesTotal < 0) {
        // The exact number of bytes to download is not known
        return;
    }

    // Page id fetching has 10% of total progress reserved for it so scaling
    // the numbers
    double progressValue = 0.1 * static_cast<double>(bytesFetched) /
        std::max(bytesTotal, qint64(1));

    WATRACE(QStringLiteral("Progress update: ") << progressValue);
    Q_EMIT progress(progressValue);
}

void WikiArticleFetcher::onPageIdFetchingFinished(
    bool status, QByteArray fetchedData, ErrorString errorDescription)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onPageIdFetchingFinished: status = ")
            << (status ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription);

    if (m_pApiUrlFetcher) {
        m_pApiUrlFetcher->disconnect(this);
        m_pApiUrlFetcher->deleteLater();
        m_pApiUrlFetcher = Q_NULLPTR;
    }

    if (!status) {
        finishWithError(errorDescription);
        return;
    }

    qint32 pageId = parsePageIdFromFetchedData(fetchedData, errorDescription);
    if (pageId < 0) {
        finishWithError(errorDescription);
        return;
    }

    QString articleUrl = QStringLiteral("https://");
    articleUrl += m_languageCode;
    articleUrl += QStringLiteral(".wikipedia.org/w/api.php"
                                 "?action=parse"
                                 "&format=xml"
                                 "&prop=text"
                                 "&pageid=") + QString::number(pageId);

    QUrl url(articleUrl);
    WADEBUG(QStringLiteral("Starting to fetch wiki article content: ") << url);

    m_pArticleContentsFetcher = new NetworkReplyFetcher(m_pNetworkAccessManager, url);

    QObject::connect(m_pArticleContentsFetcher,
                     QNSIGNAL(NetworkReplyFetcher,finished,
                              bool,QByteArray,ErrorString),
                     this,
                     QNSLOT(WikiArticleFetcher,onWikiArticleDownloadFinished,
                            bool,QByteArray,ErrorString));
    QObject::connect(m_pArticleContentsFetcher,
                     QNSIGNAL(NetworkReplyFetcher,downloadProgress,qint64,qint64),
                     this,
                     QNSLOT(WikiArticleFetcher,onWikiArticleDownloadProgress,
                            qint64,qint64));

    m_pArticleContentsFetcher->start();
}

void WikiArticleFetcher::onWikiArticleDownloadProgress(qint64 bytesFetched,
                                                       qint64 bytesTotal)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onWikiArticleDownloadProgress: fetched ")
            << bytesFetched << QStringLiteral(" out of ") << bytesTotal);

    if (bytesTotal < 0) {
        // The exact number of bytes to download is not known
        return;
    }

    // Wiki article fetching has 30% of total progress reserved for it so
    // scaling the numbers + add 10% of finished progress from page id fetching

    double progressValue = 0.1 + 0.3 * static_cast<double>(bytesFetched) /
        std::max(bytesTotal, qint64(1));

    WATRACE(QStringLiteral("Progress update: ") << progressValue);
    Q_EMIT progress(progressValue);
}

void WikiArticleFetcher::onWikiArticleDownloadFinished(
    bool status, QByteArray fetchedData, ErrorString errorDescription)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onWikiArticleDownloadFinished: ")
            << QStringLiteral("status = ")
            << (status ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ")
            << errorDescription);

    if (m_pArticleContentsFetcher) {
        m_pArticleContentsFetcher->disconnect(this);
        m_pArticleContentsFetcher->deleteLater();
        m_pArticleContentsFetcher = Q_NULLPTR;
    }

    if (!status) {
        finishWithError(errorDescription);
        return;
    }

    m_pWikiArticleToNote = new WikiArticleToNote(
        m_pNetworkAccessManager, m_enmlConverter, this);

    QObject::connect(m_pWikiArticleToNote,
                     QNSIGNAL(WikiArticleToNote,progress,double),
                     this,
                     QNSLOT(WikiArticleFetcher,onWikiArticleToNoteProgress,double));
    QObject::connect(m_pWikiArticleToNote,
                     QNSIGNAL(WikiArticleToNote,finished,bool,ErrorString,Note),
                     this,
                     QNSLOT(WikiArticleFetcher,onWikiArticleToNoteFinished,
                            bool,ErrorString,Note));

    m_pWikiArticleToNote->start(fetchedData);
}

void WikiArticleFetcher::onWikiArticleToNoteProgress(double progressValue)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onWikiArticleToNoteProgress: ")
            << progressValue);

    // Wiki article conversion to note has 60% of total progress reserved for it
    // so scaling the numbers + add 10% from page id fetching and 30% from wiki
    // article contents fetching

    progressValue *= 0.6;
    progressValue += 0.4;

    WATRACE(QStringLiteral("Progress update: ") << progressValue);
    Q_EMIT progress(progressValue);
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
    QUrl & url, ErrorString & errorDescription)
{
    QString urlString = m_url.toString();

    QRegExp regex(QStringLiteral("^https?:\\/\\/(\\w{2})\\.wikipedia\\.org\\/wiki\\/(\\w+)$"));
    int pos = regex.indexIn(urlString);
    if (Q_UNLIKELY(pos < 0)) {
        errorDescription.setBase(QT_TR_NOOP("Can't parse the input URL"));
        errorDescription.details() = urlString;
        WAWARNING(errorDescription);
        return false;
    }

    QStringList capturedTexts = regex.capturedTexts();
    if (capturedTexts.size() < 3) {
        errorDescription.setBase(QT_TR_NOOP("Can't parse the input URL: "
                                            "wrong number of captured texts"));
        errorDescription.details() = urlString;
        WAWARNING(errorDescription);
        return false;
    }

    m_languageCode = capturedTexts[1];
    m_articleTitle = capturedTexts[2];

    QString apiUrl = QStringLiteral("https://");
    apiUrl += m_languageCode;
    apiUrl += QStringLiteral(".wikipedia.org/w/api.php?action=query&format=xml&titles=");
    apiUrl += m_articleTitle;

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
            pageId = pageIdValue.toString().toInt(&conversionResult);
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

    if (m_pArticleContentsFetcher) {
        m_pArticleContentsFetcher->disconnect(this);
        m_pArticleContentsFetcher->deleteLater();
        m_pArticleContentsFetcher = Q_NULLPTR;
    }

    if (m_pWikiArticleToNote) {
        m_pWikiArticleToNote->disconnect(this);
        m_pWikiArticleToNote->deleteLater();
        m_pWikiArticleToNote = Q_NULLPTR;
    }
}

} // namespace quentier
