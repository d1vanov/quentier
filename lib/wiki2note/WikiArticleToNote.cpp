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

#include <quentier/enml/ENMLConverter.h>
#include <quentier/enml/DecryptedTextManager.h>
#include <quentier/logging/QuentierLogger.h>

#include <QBuffer>
#include <QCryptographicHash>
#include <QMimeDatabase>
#include <QMimeType>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

namespace quentier {

WikiArticleToNote::WikiArticleToNote(
        ENMLConverter & enmlConverter,
        const qint64 timeoutMsec,
        QObject * parent) :
    QObject(parent),
    m_enmlConverter(enmlConverter),
    m_networkReplyFetcherTimeout(timeoutMsec),
    m_note(),
    m_started(false),
    m_finished(false),
    m_imageDataFetchersWithProgress(),
    m_imageResourcesByUrl(),
    m_html(),
    m_progress(0.0)
{}

void WikiArticleToNote::start(QByteArray wikiPageContent)
{
    QNDEBUG("WikiArticleToNote::start");

    if (Q_UNLIKELY(m_started)) {
        QNDEBUG("Already started");
        return;
    }

    m_started = true;
    m_finished = false;

    QString html = fetchedWikiArticleToHtml(wikiPageContent);
    if (Q_UNLIKELY(html.isEmpty())) {
        ErrorString errorDescription(
            QT_TR_NOOP("Failed to extract wiki page HTML"));
        finishWithError(errorDescription);
        return;
    }

    ErrorString errorDescription;
    bool res = m_enmlConverter.cleanupExternalHtml(
        html, m_html, errorDescription);
    if (!res) {
        finishWithError(errorDescription);
        return;
    }

    QString supplementedHtml = QStringLiteral("<html><body>");
    supplementedHtml += m_html;
    supplementedHtml += QStringLiteral("</body></html>");
    m_html = supplementedHtml;

    if (!setupImageDataFetching(errorDescription)) {
        finishWithError(errorDescription);
        return;
    }

    if (m_imageDataFetchersWithProgress.isEmpty()) {
        convertHtmlToEnmlAndComposeNote();
    }
    else {
        QNDEBUG("Pending " << m_imageDataFetchersWithProgress.size()
                << " image data downloads");
    }
}

void WikiArticleToNote::onNetworkReplyFetcherFinished(
    bool status, QByteArray fetchedData, ErrorString errorDescription)
{
    NetworkReplyFetcher * pFetcher = qobject_cast<NetworkReplyFetcher*>(sender());
    QNDEBUG("WikiArticleToNote::onNetworkReplyFetcherFinished: status = "
            << (status ? "true" : "false")
            << ", error description: " << errorDescription
            << "; url: "
            << (pFetcher
                ? pFetcher->url().toString()
                : QStringLiteral("<unidentified>")));

    if (!status) {
        finishWithError(errorDescription);
        return;
    }

    createResource(fetchedData, pFetcher->url());

    auto it = m_imageDataFetchersWithProgress.find(pFetcher);
    if (Q_UNLIKELY(it == m_imageDataFetchersWithProgress.end()))
    {
        errorDescription.setBase(
            QT_TR_NOOP("Internal error: detected reply from "
                       "unidentified img data fetcher"));
        finishWithError(errorDescription);
        return;
    }

    it.value() = 1.0;
    updateProgress();

    m_imageDataFetchersWithProgress.erase(it);

    if (m_imageDataFetchersWithProgress.isEmpty()) {
        QNDEBUG("Downloaded all images, converting HTML to note");
        convertHtmlToEnmlAndComposeNote();
    }
}

void WikiArticleToNote::onNetworkReplyFetcherProgress(
    qint64 bytesFetched, qint64 bytesTotal)
{
    NetworkReplyFetcher * pFetcher = qobject_cast<NetworkReplyFetcher*>(sender());
    QNDEBUG("WikiArticleToNote::onNetworkReplyFetcherProgress: "
            << "fetched " << bytesFetched << " out of "
            << bytesTotal << " bytes; url = "
            << (pFetcher
                ? pFetcher->url().toString()
                : QStringLiteral("<unidentified>")));

    if (bytesTotal < 0) {
        // The exact number of bytes to download is not known
        return;
    }

    auto it = m_imageDataFetchersWithProgress.find(pFetcher);
    if (it != m_imageDataFetchersWithProgress.end()) {
        it.value() = static_cast<double>(bytesFetched) / std::max(bytesTotal, qint64(1));
        updateProgress();
    }
}

void WikiArticleToNote::finishWithError(ErrorString errorDescription)
{
    QNDEBUG("WikiArticleToNote::finishWithError: " << errorDescription);

    clear();
    Q_EMIT finished(false, errorDescription, Note());
}

void WikiArticleToNote::clear()
{
    QNDEBUG("WikiArticleToNote::clear");

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

    m_imageResourcesByUrl.clear();

    m_html.clear();
    m_progress = 0.0;
}

void WikiArticleToNote::updateProgress()
{
    QNDEBUG("WikiArticleToNote::updateProgress");

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
    QNTRACE("Updated progress to " << m_progress);

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

    return html;
}

bool WikiArticleToNote::setupImageDataFetching(ErrorString & errorDescription)
{
    QNDEBUG("WikiArticleToNote::setupImageDataFetching");

    QBuffer buf;
    if (!buf.open(QIODevice::WriteOnly)) {
        errorDescription.setBase(
            QT_TR_NOOP("Failed to open buffer for HTML writing"));
        return false;
    }

    QXmlStreamWriter writer(&buf);
    writer.setAutoFormatting(false);
    writer.setCodec("UTF-8");

    QXmlStreamReader reader(m_html);
    bool insideElement = false;
    bool skippingCurrentElement = false;

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext())

        if (reader.isStartDocument()) {
            writer.writeStartDocument();
        }

        if (reader.isStartElement())
        {
            insideElement = true;
            QXmlStreamAttributes attributes = reader.attributes();

            QString elementName = reader.name().toString();
            if (elementName == QStringLiteral("head")) {
                skippingCurrentElement = true;
                continue;
            }

            if (elementName == QStringLiteral("title")) {
                skippingCurrentElement = true;
                continue;
            }

            if (elementName == QStringLiteral("img"))
            {
                QString imgSrc =
                    attributes.value(QStringLiteral("src")).toString();
                if (!imgSrc.startsWith(QStringLiteral("https:")) &&
                    imgSrc.startsWith(QStringLiteral("//")))
                {
                    imgSrc = QStringLiteral("https:") + imgSrc;
                }
                else
                {
                    skippingCurrentElement = true;
                }

                if (!skippingCurrentElement)
                {
                    QUrl imgSrcUrl(imgSrc);

                    if (!imgSrcUrl.isValid())
                    {
                        errorDescription.setBase(
                            QT_TR_NOOP("Failed to download image: cannot "
                                       "convert img src to a valid URL"));
                        QNWARNING(errorDescription << ": " << imgSrc);
                        return false;
                    }

                    QNDEBUG("Starting to download image: " << imgSrcUrl);

                    NetworkReplyFetcher * pFetcher = new NetworkReplyFetcher(
                        imgSrcUrl,
                        m_networkReplyFetcherTimeout);

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
                    pFetcher->start();
                }
            }

            if (!skippingCurrentElement) {
                writer.writeStartElement(elementName);
                writer.writeAttributes(attributes);
            }
        }

        if (reader.isEndElement())
        {
            if (!skippingCurrentElement) {
                writer.writeEndElement();
            }
            else {
                skippingCurrentElement = false;
            }

            insideElement = false;
        }

        if (insideElement)
        {
            if (reader.isCDATA()) {
                writer.writeCDATA(reader.text().toString());
            }
            else {
                writer.writeCharacters(reader.text().toString());
            }
        }

        if (reader.isEndDocument()) {
            writer.writeEndDocument();
        }
    }

    if (reader.hasError()) {
        errorDescription.setBase(QT_TR_NOOP("Failed to parse HTML to extract "
                                            "img tags info"));
        errorDescription.details() = reader.errorString();
        QNWARNING(errorDescription);
        return false;
    }

    m_html = QString::fromUtf8(buf.buffer());
    return true;
}

void WikiArticleToNote::createResource(
    const QByteArray & fetchedData, const QUrl & url)
{
    QNDEBUG("WikiArticleToNote::createResource: url = " << url);

    Resource & resource = m_imageResourcesByUrl[url];

    resource.setDataBody(fetchedData);
    resource.setDataSize(fetchedData.size());
    resource.setDataHash(
        QCryptographicHash::hash(fetchedData, QCryptographicHash::Md5));

    QString urlString = url.toString();
    QString fileName;

    int index = urlString.lastIndexOf(QChar::fromLatin1('/'));
    if (index >= 0) {
        fileName = urlString.mid(index + 1);
    }

    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForData(fetchedData);
    if (!mimeType.isValid())
    {
        // Try to extract the mime type from url
        QList<QMimeType> mimeTypes = mimeDatabase.mimeTypesForFileName(fileName);
        for(auto it = mimeTypes.constBegin(),
            end = mimeTypes.constEnd(); it != end; ++it)
        {
            if (it->isValid()) {
                mimeType = *it;
                break;
            }
        }
    }

    if (mimeType.isValid()) {
        resource.setMime(mimeType.name());
    }
    else {
        // Just a wild guess as a last resort
        resource.setMime(QStringLiteral("image/png"));
    }

    qevercloud::ResourceAttributes & attributes = resource.resourceAttributes();
    attributes.sourceURL = urlString;
    if (!fileName.isEmpty()) {
        attributes.fileName = fileName;
    }
}

void WikiArticleToNote::convertHtmlToEnmlAndComposeNote()
{
    QNDEBUG("WikiArticleToNote::convertHtmlToEnmlAndComposeNote");

    if (!preprocessHtmlForConversionToEnml()) {
        return;
    }

    QString enml;
    DecryptedTextManager decryptedTextManager;
    ErrorString errorDescription;

    bool res = m_enmlConverter.htmlToNoteContent(
        m_html, enml, decryptedTextManager, errorDescription);
    if (!res) {
        finishWithError(errorDescription);
        return;
    }

    m_note.setContent(enml);
    for(auto it = m_imageResourcesByUrl.constBegin(),
        end = m_imageResourcesByUrl.constEnd(); it != end; ++it)
    {
        m_note.addResource(it.value());
    }

    m_progress = 1.0;
    Q_EMIT progress(m_progress);

    m_started = false;
    m_finished = true;

    Q_EMIT finished(true, ErrorString(), m_note);
}

bool WikiArticleToNote::preprocessHtmlForConversionToEnml()
{
    QNDEBUG("WikiArticleToNote::preprocessHtmlForConversionToEnml");

    // This tag seems to be present more than once and confuses ENMLConverter
    m_html.remove(QStringLiteral("<title/>"));

    if (m_imageResourcesByUrl.isEmpty()) {
        return true;
    }

    QString preprocessedHtml;
    QXmlStreamWriter writer(&preprocessedHtml);
    writer.setAutoFormatting(false);
    writer.setCodec("UTF-8");
    writer.writeStartDocument();

    QXmlStreamReader reader(m_html);
    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartDocument()) {
            continue;
        }

        if (reader.isDTD()) {
            writer.writeDTD(reader.text().toString());
            continue;
        }

        if (reader.isEndDocument()) {
            writer.writeEndDocument();
            break;
        }

        if (reader.isStartElement())
        {
            QString name = reader.name().toString();
            writer.writeStartElement(name);

            QXmlStreamAttributes attributes = reader.attributes();
            if (name == QStringLiteral("img"))
            {
                attributes.append(
                    QStringLiteral("en-tag"), QStringLiteral("en-media"));

                QString src = attributes.value(QStringLiteral("src")).toString();
                if (!src.startsWith(QStringLiteral("https:"))) {
                    src = QStringLiteral("https:") + src;
                }

                auto resourceIt = m_imageResourcesByUrl.find(QUrl(src));
                if (Q_UNLIKELY(resourceIt == m_imageResourcesByUrl.end()))
                {
                    ErrorString errorDescription(
                        QT_TR_NOOP("Cannot convert wiki page to ENML: img "
                                   "resource not found"));
                    finishWithError(errorDescription);
                    return false;
                }

                const Resource & resource = resourceIt.value();
                attributes.append(QStringLiteral("hash"),
                                  QString::fromUtf8(resource.dataHash().toHex()));
                attributes.append(QStringLiteral("type"), resource.mime());
            }

            writer.writeAttributes(attributes);
            continue;
        }

        if (reader.isCharacters())
        {
            QString data = reader.text().toString();

            if (reader.isCDATA()) {
                writer.writeCDATA(data);
            }
            else {
                writer.writeCharacters(data);
            }

            continue;
        }

        if (reader.isEndElement()) {
            writer.writeEndElement();
            continue;
        }
    }

    if (Q_UNLIKELY(reader.hasError())) {
        ErrorString errorDescription(QT_TR_NOOP("Cannot convert wiki page to ENML"));
        errorDescription.details() = reader.errorString();
        finishWithError(errorDescription);
        return false;
    }

    m_html = preprocessedHtml;
    return true;
}

} // namespace quentier
