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

#include "WikiRandomArticleUrlFetcher.h"

#include <lib/network/NetworkReplyFetcher.h>

#include <quentier/logging/QuentierLogger.h>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#endif

#include <algorithm>

namespace quentier {

WikiRandomArticleUrlFetcher::WikiRandomArticleUrlFetcher(
        QNetworkAccessManager * pNetworkAccessManager,
        QObject * parent) :
    QObject(parent),
    m_pNetworkAccessManager(pNetworkAccessManager),
    m_pNetworkReplyFetcher(Q_NULLPTR),
    m_started(false),
    m_finished(false),
    m_url()
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
    QNDEBUG(QStringLiteral("WikiRandomArticleUrlFetcher::start"));

    if (Q_UNLIKELY(m_started)) {
        QNWARNING(QStringLiteral("WikiRandomArticleUrlFetcher is already started"));
        return;
    }

    QUrl query(QStringLiteral("https://en.wikipedia.org/w/api.php"
                              "?action=query"
                              "&format=json"
                              "&list=random"
                              "&rnlimit=1"
                              "&rnnamespace=0"));

    m_pNetworkReplyFetcher = new NetworkReplyFetcher(m_pNetworkAccessManager, query);

    QObject::connect(this, QNSIGNAL(WikiRandomArticleUrlFetcher,startFetching),
                     m_pNetworkReplyFetcher, QNSLOT(NetworkReplyFetcher,start));

    QObject::connect(m_pNetworkReplyFetcher,
                     QNSIGNAL(NetworkReplyFetcher,finished,
                              bool,QByteArray,ErrorString),
                     this,
                     QNSLOT(WikiRandomArticleUrlFetcher,onDownloadFinished,
                            bool,QByteArray,ErrorString));
    QObject::connect(m_pNetworkReplyFetcher,
                     QNSIGNAL(NetworkReplyFetcher,downloadProgress,qint64,qint64),
                     this,
                     QNSLOT(WikiRandomArticleUrlFetcher,onDownloadProgress,
                            qint64,qint64));

    m_started = true;
}

void WikiRandomArticleUrlFetcher::onDownloadProgress(qint64 bytesFetched,
                                                     qint64 bytesTotal)
{
    QNDEBUG(QStringLiteral("WikiRandomArticleUrlFetcher::onDownloadProgress: fetched ")
            << bytesFetched << QStringLiteral(" bytes, total " )
            << bytesTotal << QStringLiteral(" bytes"));

    double percentage = 0;
    if (bytesTotal != 0) {
        percentage = static_cast<double>(bytesFetched) / bytesTotal;
        percentage = std::max(percentage, 1.0);
    }

    percentage *= 100.0;
    Q_EMIT progress(percentage);
}

void WikiRandomArticleUrlFetcher::onDownloadFinished(bool status,
                                                     QByteArray fetchedData,
                                                     ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("WikiRandomArticleUrlFetcher::onDownloadFinished: status = ")
            << (status ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", error description = ") << errorDescription);

    if (!status) {
        QNWARNING(QStringLiteral("Download failed: ") << errorDescription);
        finishWithError(errorDescription);
        return;
    }

    errorDescription.clear();

    qint32 pageId = parsePageIdFromFetchedData(fetchedData, errorDescription);
    if (pageId < 0) {
        finishWithError(errorDescription);
        return;
    }

    m_url = QUrl(QStringLiteral("https://en.wikipedia.org/w/api.php"
                                "?action=parse"
                                "&format=xml"
                                "&section=0"
                                "&prop=text"
                                "&pageid=") + QString::number(pageId));
    if (!m_url.isValid()) {
        errorDescription.setBase(QT_TR_NOOP("Failed to compose valid URL from "
                                            "data fetched from Wiki"));
        QNWARNING(errorDescription << QStringLiteral(", page id = ") << pageId);
        finishWithError(errorDescription);
        return;
    }

    QNDEBUG(QStringLiteral("Successfully composed random article URL: ") << m_url);

    m_started = false;
    m_finished = true;
    Q_EMIT finished(true, m_url, ErrorString());
}

qint32 WikiRandomArticleUrlFetcher::parsePageIdFromFetchedData(
    const QByteArray & fetchedData,
    ErrorString & errorDescription)
{
    qint32 pageId = 0;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(fetchedData, &error);
    if (error.error) {
        errorDescription.setBase(QT_TR_NOOP("Failed to parse response as JSON "
                                            "on attempt to fetch random "
                                            "Wiki article URL"));
        errorDescription.details() = error.errorString() + QStringLiteral(" (") +
                                     QString::number(error.error) + QStringLiteral(")");
        QNWARNING(errorDescription << QStringLiteral(": ") << fetchedData);
        return -1;
    }

    if (!document.isObject()) {
        errorDescription.setBase(QT_TR_NOOP("Unexpected response on attempt "
                                            "to fetch random Wiki article URL"));
        QNWARNING(errorDescription << QStringLiteral(": ") << fetchedData);
        return -1;
    }

    QJsonObject object = document.object();
    QVariantHash objectHash = object.toVariantHash();
    QVariantHash queryHash = objectHash[QStringLiteral("query")].toHash();
    QJsonArray randomArray = queryHash[QStringLiteral("random")].toJsonArray();
    QVariantHash randomObjectHash = randomArray.at(0).toObject().toVariantHash();
    auto it = randomObjectHash.constFind(QStringLiteral("id"));
    if (it == randomObjectHash.constEnd()) {
        errorDescription.setBase(QT_TR_NOOP("Failed to fetch random Wiki "
                                            "article URL: could not find id "
                                            "property within JSON"));
        QNWARNING(errorDescription << QStringLiteral(": ") << fetchedData);
        return -1;
    }

    const QVariant & idValue = it.value();
    bool conversionResult = false;
    pageId = idValue.toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription.setBase(QT_TR_NOOP("Failed to fetch random Wiki "
                                            "article URL: could not convert "
                                            "id property from JSON to int"));
        QNWARNING(errorDescription << QStringLiteral(": ") << idValue);
        return -1;
    }

#else // QT_VERSION_CHECK

    // Qt4 doesn't have JSON parsing machinery built it and it's going to be
    // deprecated soon enough so can just insert a hack here: just find the value
    // within the string representing JSON, in a quick and dirty manner
    QString fetchedDataStr = QString::fromUtf8(fetchedData);
    int idIndex = fetchedDataStr.indexOf(QStringLiteral("\"id\": "));
    if (idIndex < 0) {
        errorDescription.setBase(QT_TR_NOOP("Failed to fetch random Wiki "
                                            "article URL: can't find id "
                                            "property within JSON"));
        QNWARNING(errorDescription << QStringLiteral(": ") << fetchedData);
        return -1;
    }

    int colonIndex = fetchedDataStr.indexOf(QStringLiteral(","), idIndex);
    if (colonIndex < 0) {
        errorDescription.setBase(QT_TR_NOOP("Failed to fetch random Wiki "
                                            "article URL: can't find colon after "
                                            "id property within JSON"));
        QNWARNING(errorDescription << QStringLiteral(": ") << fetchedData);
        return -1;
    }

    idIndex += 6;   // shift to the beginning of the value
    QString pageIdStr = fetchedDataStr.mid(idIndex, (colonIndex - idIndex));

    bool conversionResult = false;
    pageId = pageIdStr.toInt(&conversionResult);
    if (!conversionResult) {
        errorDescription.setBase(QT_TR_NOOP("Failed to fetch random Wiki "
                                            "article URL: could not convert "
                                            "id property from JSON to int"));
        QNWARNING(errorDescription << QStringLiteral(": ") << pageIdStr);
        return -1;
    }

#endif // QT_VERSION_CHECK

    return pageId;
}

} // namespace quentier
