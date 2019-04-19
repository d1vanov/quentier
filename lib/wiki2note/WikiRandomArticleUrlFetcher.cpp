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
#include <quentier/utility/Utility.h>

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
            << humanReadableSize(bytesFetched) << QStringLiteral(", total " )
            << humanReadableSize(bytesTotal));

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
        Q_EMIT finished(false, QUrl(), errorDescription);
        return;
    }

    // TODO: parse the random article URL out of the reply
    Q_UNUSED(fetchedData);
}

} // namespace quentier
