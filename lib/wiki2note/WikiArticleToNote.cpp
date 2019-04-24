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

#include "WikiArticleToNote.h"

#include <lib/network/NetworkReplyFetcher.h>

#include <quentier/enml/ENMLConverter.h>
#include <quentier/logging/QuentierLogger.h>

#include <QXmlStreamReader>

#include <algorithm>

namespace quentier {

WikiArticleToNote::WikiArticleToNote(
        QNetworkAccessManager * pNetworkAccessManager,
        ENMLConverter & enmlConverter,
        QObject * parent) :
    QObject(parent),
    m_pNetworkAccessManager(pNetworkAccessManager),
    m_enmlConverter(enmlConverter),
    m_note(),
    m_started(false),
    m_finished(false),
    m_imageDataFetchersWithProgress(),
    m_progress(0.0)
{}

void WikiArticleToNote::start(QByteArray wikiPageContent)
{
    QNDEBUG(QStringLiteral("WikiArticleToNote::start"));

    if (Q_UNLIKELY(m_started)) {
        QNDEBUG(QStringLiteral("Already started"));
        return;
    }

    m_started = true;
    m_finished = false;

    QString html = fetchedWikiArticleToHtml(wikiPageContent);
    if (Q_UNLIKELY(html.isEmpty())) {
        ErrorString errorDescription(QT_TR_NOOP("Failed to extract wiki page HTML"));
        finishWithError(errorDescription);
        return;
    }

    QString cleanedUpHtml;
    ErrorString errorDescription;
    bool res = m_enmlConverter.cleanupExternalHtml(
        html, cleanedUpHtml, errorDescription);
    if (!res) {
        finishWithError(errorDescription);
        return;
    }

    if (!setupImageDataFetching(cleanedUpHtml, errorDescription)) {
        finishWithError(errorDescription);
        return;
    }

    if (!m_imageDataFetchersWithProgress.isEmpty()) {
        return;
    }

    // TODO: convert HTML to ENML
}

void WikiArticleToNote::onNetworkReplyFetcherFinished(
    bool status, QByteArray fetchedData, ErrorString errorDescription)
{
    NetworkReplyFetcher * pFetcher = qobject_cast<NetworkReplyFetcher*>(sender());
    QNDEBUG(QStringLiteral("WikiArticleToNote::onNetworkReplyFetcherFinished: status = ")
            << (status ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description: ") << errorDescription
            << QStringLiteral("; url: ")
            << (pFetcher ? pFetcher->url().toString() : QStringLiteral("<unidentified>")));

    if (!status) {
        finishWithError(errorDescription);
        return;
    }

    // TODO: check if all img src network replies fetched, if yes, proceed to
    // converting wiki page's HTML to ENML
    Q_UNUSED(fetchedData)
}

void WikiArticleToNote::onNetworkReplyFetcherProgress(qint64 bytesFetched,
                                                      qint64 bytesTotal)
{
    NetworkReplyFetcher * pFetcher = qobject_cast<NetworkReplyFetcher*>(sender());
    QNDEBUG(QStringLiteral("WikiArticleToNote::onNetworkReplyFetcherProgress: ")
            << QStringLiteral("fetched ") << bytesFetched << QStringLiteral(" out of ")
            << bytesTotal << QStringLiteral(" bytes; url = ")
            << (pFetcher ? pFetcher->url().toString() : QStringLiteral("<unidentified>")));

    auto it = m_imageDataFetchersWithProgress.find(pFetcher);
    if (it != m_imageDataFetchersWithProgress.end()) {
        it.value() = static_cast<double>(bytesFetched) / std::max(bytesTotal, qint64(1));
        updateProgress();
    }
}

void WikiArticleToNote::finishWithError(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("WikiArticleToNote::finishWithError: ")
            << errorDescription);

    clear();
    Q_EMIT finished(false, errorDescription, Note());
}

void WikiArticleToNote::clear()
{
    QNDEBUG(QStringLiteral("WikiArticleToNote::clear"));

    m_started = false;
    m_finished = false;

    m_note = Note();

    for(auto it = m_imageDataFetchersWithProgress.constBegin(),
        end = m_imageDataFetchersWithProgress.constEnd(); it != end; ++it)
    {
        NetworkReplyFetcher * pNetworkReplyFetcher = it.key();
        pNetworkReplyFetcher->disconnect(this);
        pNetworkReplyFetcher->deleteLater();
    }
    m_imageDataFetchersWithProgress.clear();

    m_progress = 0.0;
}

void WikiArticleToNote::updateProgress()
{
    QNDEBUG(QStringLiteral("WikiArticleToNote::updateProgress"));

    if (m_imageDataFetchersWithProgress.isEmpty()) {
        return;
    }

    double imageFetchersProgress = 0.0;
    for(auto it = m_imageDataFetchersWithProgress.constBegin(),
        end = m_imageDataFetchersWithProgress.constEnd(); it != end; ++it)
    {
        imageFetchersProgress += it.value();
    }
    imageFetchersProgress /= m_imageDataFetchersWithProgress.size();

    // 10% of progress are reserved for final HTML to ENML conversion
    imageFetchersProgress *= 0.9;

    m_progress = std::max(imageFetchersProgress, 0.9);
    QNTRACE(QStringLiteral("Updated progress to ") << m_progress);

    Q_EMIT progress(m_progress);
}

QString WikiArticleToNote::fetchedWikiArticleToHtml(
    const QByteArray & fetchedData) const
{
    QString html;
    QXmlStreamReader reader(fetchedData);
    bool insideTextElement = false;
    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext())

        if (reader.isStartElement() &&
            reader.name().toString() == QStringLiteral("text"))
        {
            insideTextElement = true;
        }

        if (reader.isCharacters() && insideTextElement) {
            html += reader.text().toString();
        }

        if (reader.isEndElement() && insideTextElement) {
            insideTextElement = false;
        }
    }

    if (!html.isEmpty()) {
        html = QStringLiteral("<html><body>") + html + QStringLiteral("</body></html>");
    }

    return html;
}

bool WikiArticleToNote::setupImageDataFetching(const QString & html,
                                               ErrorString & errorDescription)
{
    QXmlStreamReader reader(html);
    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext())

        if (reader.isStartElement() &&
            (reader.name().toString() == QStringLiteral("img")))
        {
            QXmlStreamAttributes attributes = reader.attributes();
            QString imgSrc = attributes.value(QStringLiteral("src")).toString();
            QUrl imgSrcUrl(imgSrc);

            if (!imgSrcUrl.isValid()) {
                errorDescription.setBase(QT_TR_NOOP("Failed to download image: "
                                                    "cannot convert img src to "
                                                    "a valid URL"));
                QNWARNING(errorDescription << QStringLiteral(": ") << imgSrc);
                return false;
            }

            NetworkReplyFetcher * pFetcher = new NetworkReplyFetcher(
                m_pNetworkAccessManager, imgSrcUrl);
            pFetcher->setParent(this);

            QObject::connect(pFetcher,
                             QNSIGNAL(NetworkReplyFetcher,finished,
                                      bool,QByteArray,ErrorString),
                             this,
                             QNSLOT(WikiArticleToNote,onNetworkReplyFetcherFinished,
                                    bool,QByteArray,ErrorString));
            QObject::connect(pFetcher,
                             QNSIGNAL(NetworkReplyFetcher,downloadProgress,
                                      qint64,qint64),
                             this,
                             QNSLOT(WikiArticleToNote,onNetworkReplyFetcherProgress,
                                    qint64,qint64));

            m_imageDataFetchersWithProgress[pFetcher] = 0.0;
        }
    }

    return true;
}

} // namespace quentier
