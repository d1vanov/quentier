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

#include "WikiArticleToNote.h"

#include <quentier/enml/DecryptedTextManager.h>
#include <quentier/enml/ENMLConverter.h>
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
    ENMLConverter & enmlConverter, const qint64 timeoutMsec, QObject * parent) :
    QObject(parent),
    m_enmlConverter(enmlConverter), m_networkReplyFetcherTimeout(timeoutMsec)
{}

WikiArticleToNote::~WikiArticleToNote() = default;

void WikiArticleToNote::start(const QByteArray & wikiPageContent)
{
    QNDEBUG("wiki2note", "WikiArticleToNote::start");

    if (Q_UNLIKELY(m_started)) {
        QNDEBUG("wiki2note", "Already started");
        return;
    }

    m_started = true;
    m_finished = false;

    const QString html = fetchedWikiArticleToHtml(wikiPageContent);
    if (Q_UNLIKELY(html.isEmpty())) {
        ErrorString errorDescription(
            QT_TR_NOOP("Failed to extract wiki page HTML"));
        finishWithError(errorDescription);
        return;
    }

    ErrorString errorDescription;
    if (!m_enmlConverter.cleanupExternalHtml(html, m_html, errorDescription)) {
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
        QNDEBUG(
            "wiki2note",
            "Pending " << m_imageDataFetchersWithProgress.size()
                       << " image data downloads");
    }
}

void WikiArticleToNote::onNetworkReplyFetcherFinished(
    bool status, QByteArray fetchedData, // NOLINT
    ErrorString errorDescription)
{
    auto * pFetcher = qobject_cast<NetworkReplyFetcher *>(sender());

    QNDEBUG(
        "wiki2note",
        "WikiArticleToNote::onNetworkReplyFetcherFinished: "
            << "status = " << (status ? "true" : "false")
            << ", error description: " << errorDescription << "; url: "
            << (pFetcher ? pFetcher->url().toString()
                         : QStringLiteral("<unidentified>")));

    if (!status) {
        finishWithError(errorDescription);
        return;
    }

    createResource(fetchedData, pFetcher->url());

    auto it = m_imageDataFetchersWithProgress.find(pFetcher);
    if (Q_UNLIKELY(it == m_imageDataFetchersWithProgress.end())) {
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
        QNDEBUG("wiki2note", "Downloaded all images, converting HTML to note");
        convertHtmlToEnmlAndComposeNote();
    }
}

void WikiArticleToNote::onNetworkReplyFetcherProgress(
    qint64 bytesFetched, qint64 bytesTotal)
{
    auto * pFetcher = qobject_cast<NetworkReplyFetcher *>(sender());

    QNDEBUG(
        "wiki2note",
        "WikiArticleToNote::onNetworkReplyFetcherProgress: "
            << "fetched " << bytesFetched << " out of " << bytesTotal
            << " bytes; url = "
            << (pFetcher ? pFetcher->url().toString()
                         : QStringLiteral("<unidentified>")));

    if (bytesTotal < 0) {
        // The exact number of bytes to download is not known
        return;
    }

    const auto it = m_imageDataFetchersWithProgress.find(pFetcher);
    if (it != m_imageDataFetchersWithProgress.end()) {
        it.value() = static_cast<double>(bytesFetched) /
            static_cast<double>(std::max(bytesTotal, qint64(1)));

        updateProgress();
    }
}

void WikiArticleToNote::finishWithError(
    ErrorString errorDescription) // NOLINT
{
    QNDEBUG(
        "wiki2note",
        "WikiArticleToNote::finishWithError: " << errorDescription);

    clear();
    Q_EMIT finished(false, errorDescription, qevercloud::Note{});
}

void WikiArticleToNote::clear()
{
    QNDEBUG("wiki2note", "WikiArticleToNote::clear");

    m_started = false;
    m_finished = false;

    m_note = qevercloud::Note{};

    for (auto it = m_imageDataFetchersWithProgress.constBegin(),
              end = m_imageDataFetchersWithProgress.constEnd();
         it != end; ++it)
    {
        auto * pNetworkReplyFetcher = it.key();
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
    QNDEBUG("wiki2note", "WikiArticleToNote::updateProgress");

    if (m_imageDataFetchersWithProgress.isEmpty()) {
        return;
    }

    double imageFetchersProgress = 0.0;
    for (auto it = m_imageDataFetchersWithProgress.constBegin(),
              end = m_imageDataFetchersWithProgress.constEnd();
         it != end; ++it)
    {
        imageFetchersProgress += it.value();
    }

    imageFetchersProgress /= m_imageDataFetchersWithProgress.size();

    // 10% of progress are reserved for final HTML to ENML conversion
    imageFetchersProgress *= 0.9;

    m_progress = std::max(imageFetchersProgress, 0.9);
    QNTRACE("wiki2note", "Updated progress to " << m_progress);

    Q_EMIT progress(m_progress);
}

QString WikiArticleToNote::fetchedWikiArticleToHtml(
    const QByteArray & fetchedData) const
{
    QString html;
    QXmlStreamReader reader(fetchedData);
    bool insideTextElement = false;
    while (!reader.atEnd()) {
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
    QNDEBUG("wiki2note", "WikiArticleToNote::setupImageDataFetching");

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

    while (!reader.atEnd()) {
        Q_UNUSED(reader.readNext())

        if (reader.isStartDocument()) {
            writer.writeStartDocument();
        }

        if (reader.isStartElement()) {
            insideElement = true;
            const QXmlStreamAttributes attributes = reader.attributes();

            const QString elementName = reader.name().toString();
            if (elementName == QStringLiteral("head")) {
                skippingCurrentElement = true;
                continue;
            }

            if (elementName == QStringLiteral("title")) {
                skippingCurrentElement = true;
                continue;
            }

            if (elementName == QStringLiteral("img")) {
                QString imgSrc =
                    attributes.value(QStringLiteral("src")).toString();
                if (!imgSrc.startsWith(QStringLiteral("https:")) &&
                    imgSrc.startsWith(QStringLiteral("//")))
                {
                    imgSrc = QStringLiteral("https:") + imgSrc;
                }
                else {
                    skippingCurrentElement = true;
                }

                if (!skippingCurrentElement) {
                    const QUrl imgSrcUrl{imgSrc};

                    if (!imgSrcUrl.isValid()) {
                        errorDescription.setBase(
                            QT_TR_NOOP("Failed to download image: cannot "
                                       "convert img src to a valid URL"));

                        QNWARNING(
                            "wiki2note", errorDescription << ": " << imgSrc);
                        return false;
                    }

                    QNDEBUG(
                        "wiki2note",
                        "Starting to download image: " << imgSrcUrl);

                    auto * pFetcher = new NetworkReplyFetcher(
                        imgSrcUrl, m_networkReplyFetcherTimeout);

                    pFetcher->setParent(this);

                    QObject::connect(
                        pFetcher, &NetworkReplyFetcher::finished, this,
                        &WikiArticleToNote::onNetworkReplyFetcherFinished);

                    QObject::connect(
                        pFetcher, &NetworkReplyFetcher::downloadProgress, this,
                        &WikiArticleToNote::onNetworkReplyFetcherProgress);

                    m_imageDataFetchersWithProgress[pFetcher] = 0.0;
                    pFetcher->start();
                }
            }

            if (!skippingCurrentElement) {
                writer.writeStartElement(elementName);
                writer.writeAttributes(attributes);
            }
        }

        if (reader.isEndElement()) {
            if (!skippingCurrentElement) {
                writer.writeEndElement();
            }
            else {
                skippingCurrentElement = false;
            }

            insideElement = false;
        }

        if (insideElement) {
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
        errorDescription.setBase(
            QT_TR_NOOP("Failed to parse HTML to extract img tags info"));
        errorDescription.details() = reader.errorString();
        QNWARNING("wiki2note", errorDescription);
        return false;
    }

    m_html = QString::fromUtf8(buf.buffer());
    return true;
}

void WikiArticleToNote::createResource(
    const QByteArray & fetchedData, const QUrl & url)
{
    QNDEBUG("wiki2note", "WikiArticleToNote::createResource: url = " << url);

    auto & resource = m_imageResourcesByUrl[url];

    if (!resource.data()) {
        resource.setData(qevercloud::Data{});
    }
    resource.mutableData()->setBody(fetchedData);
    resource.mutableData()->setSize(fetchedData.size());

    resource.mutableData()->setBodyHash(
        QCryptographicHash::hash(fetchedData, QCryptographicHash::Md5));

    QString urlString = url.toString();
    QString fileName;

    const int index = urlString.lastIndexOf(QChar::fromLatin1('/'));
    if (index >= 0) {
        fileName = urlString.mid(index + 1);
    }

    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForData(fetchedData);
    if (!mimeType.isValid()) {
        // Try to extract the mime type from url
        const auto mimeTypes = mimeDatabase.mimeTypesForFileName(fileName);
        for (auto it = mimeTypes.constBegin(), end = mimeTypes.constEnd();
             it != end; ++it)
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

    if (!resource.attributes()) {
        resource.setAttributes(qevercloud::ResourceAttributes{});
    }

    auto & attributes = *resource.mutableAttributes();
    attributes.setSourceURL(std::move(urlString));
    if (!fileName.isEmpty()) {
        attributes.setFileName(std::move(fileName));
    }
}

void WikiArticleToNote::convertHtmlToEnmlAndComposeNote()
{
    QNDEBUG("wiki2note", "WikiArticleToNote::convertHtmlToEnmlAndComposeNote");

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
    for (auto it = m_imageResourcesByUrl.constBegin(),
              end = m_imageResourcesByUrl.constEnd();
         it != end; ++it)
    {
        if (m_note.resources()) {
            m_note.mutableResources()->push_back(it.value());
        }
        else {
            m_note.setResources(QList<qevercloud::Resource>{} << it.value());
        }
    }

    m_progress = 1.0;
    Q_EMIT progress(m_progress);

    m_started = false;
    m_finished = true;

    Q_EMIT finished(true, ErrorString(), m_note);
}

bool WikiArticleToNote::preprocessHtmlForConversionToEnml()
{
    QNDEBUG(
        "wiki2note", "WikiArticleToNote::preprocessHtmlForConversionToEnml");

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
    while (!reader.atEnd()) {
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

        if (reader.isStartElement()) {
            const QString name = reader.name().toString();
            writer.writeStartElement(name);

            auto attributes = reader.attributes();
            if (name == QStringLiteral("img")) {
                attributes.append(
                    QStringLiteral("en-tag"), QStringLiteral("en-media"));

                QString src =
                    attributes.value(QStringLiteral("src")).toString();
                if (!src.startsWith(QStringLiteral("https:"))) {
                    src = QStringLiteral("https:") + src;
                }

                const auto resourceIt = m_imageResourcesByUrl.find(QUrl{src});
                if (Q_UNLIKELY(resourceIt == m_imageResourcesByUrl.end())) {
                    ErrorString errorDescription(
                        QT_TR_NOOP("Cannot convert wiki page to ENML: img "
                                   "resource not found"));
                    finishWithError(errorDescription);
                    return false;
                }

                const auto & resource = resourceIt.value();

                attributes.append(
                    QStringLiteral("hash"),
                    QString::fromUtf8(
                        resource.data().value().bodyHash().value().toHex()));

                attributes.append(
                    QStringLiteral("type"), resource.mime().value());
            }

            writer.writeAttributes(attributes);
            continue;
        }

        if (reader.isCharacters()) {
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
        ErrorString errorDescription(
            QT_TR_NOOP("Cannot convert wiki page to ENML"));
        errorDescription.details() = reader.errorString();
        finishWithError(errorDescription);
        return false;
    }

    m_html = preprocessedHtml;
    return true;
}

} // namespace quentier
