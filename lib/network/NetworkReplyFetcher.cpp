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

#include "NetworkReplyFetcher.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Utility.h>

#include <QDateTime>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QTimer>

#define RFLOG_IMPL(message, macro)                                             \
    macro("network", "<" << m_url << ">: " << message) // RFLOG_IMPL

#define RFTRACE(message)    RFLOG_IMPL(message, QNTRACE)
#define RFDEBUG(message)    RFLOG_IMPL(message, QNDEBUG)
#define RFINFO(message)     RFLOG_IMPL(message, QNINFO)
#define RFWARNING(message)  RFLOG_IMPL(message, QNWARNING)
#define RFCRITICAL(message) RFLOG_IMPL(message, QNCRITICAL)
#define RFFATAL(message)    RFLOG_IMPL(message, QNFATAL)

#define NETWORK_REPLY_FETCHER_TIMEOUT_CHECKER_INTERVAL (1000)

namespace quentier {

NetworkReplyFetcher::NetworkReplyFetcher(
    const QUrl & url, const qint64 timeoutMsec, QObject * parent) :
    QObject(parent),
    m_pNetworkAccessManager(new QNetworkAccessManager(this)), m_url(url),
    m_timeoutMsec(timeoutMsec)
{}

NetworkReplyFetcher::~NetworkReplyFetcher()
{
    RFDEBUG("NetworkReplyFetcher::~NetworkReplyFetcher");
}

QByteArray NetworkReplyFetcher::fetchedData() const
{
    return m_fetchedData;
}

void NetworkReplyFetcher::start()
{
    RFDEBUG("NetworkReplyFetcher::start");

    if (m_started) {
        RFDEBUG("Already started");
        return;
    }

    m_started = true;

    m_lastNetworkTime = QDateTime::currentMSecsSinceEpoch();

    if (m_timeoutMsec > 0) {
        m_pTimeoutTimer = new QTimer(this);

        QObject::connect(
            m_pTimeoutTimer, &QTimer::timeout, this,
            &NetworkReplyFetcher::checkForTimeout);

        m_pTimeoutTimer->start(NETWORK_REPLY_FETCHER_TIMEOUT_CHECKER_INTERVAL);
    }

    QNetworkRequest request;
    request.setUrl(m_url);

    QObject::connect(
        m_pNetworkAccessManager, &QNetworkAccessManager::finished, this,
        &NetworkReplyFetcher::onReplyFinished);

    QObject::connect(
        m_pNetworkAccessManager, &QNetworkAccessManager::sslErrors, this,
        &NetworkReplyFetcher::onReplySslErrors);

    auto * pReply = m_pNetworkAccessManager->get(request);

    QObject::connect(
        pReply, &QNetworkReply::downloadProgress, this,
        &NetworkReplyFetcher::onDownloadProgress);
}

void NetworkReplyFetcher::onReplyFinished(QNetworkReply * pReply)
{
    RFDEBUG("NetworkReplyFetcher::onReplyFinished");

    if (m_finished) {
        RFDEBUG("Already finished, probably due to timeout");
        recycleNetworkReply(pReply);
        return;
    }

    if (m_pTimeoutTimer) {
        m_pTimeoutTimer->stop();
        m_pTimeoutTimer->disconnect(this);
        m_pTimeoutTimer->deleteLater();
        m_pTimeoutTimer = nullptr;
    }

    if (pReply->error()) {
        ErrorString errorDescription(QT_TR_NOOP("network error"));
        errorDescription.details() += QStringLiteral("(");
        errorDescription.details() += QString::number(pReply->error());
        errorDescription.details() += QStringLiteral(") ");
        errorDescription.details() += pReply->errorString();
        finishWithError(errorDescription);

        recycleNetworkReply(pReply);
        return;
    }

    QVariant statusCodeAttribute =
        pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

    bool conversionResult = false;
    m_httpStatusCode = statusCodeAttribute.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString errorDescription(
            QT_TR_NOOP("Failed to convert HTTP status code to int"));

        QString str;
        QDebug dbg(&str);
        dbg << statusCodeAttribute;

        errorDescription.details() += str;
        finishWithError(errorDescription);
        recycleNetworkReply(pReply);
        return;
    }

    m_started = false;
    m_finished = true;

    m_fetchedData = pReply->readAll();

    Q_EMIT finished(true, m_fetchedData, ErrorString());
    recycleNetworkReply(pReply);
}

void NetworkReplyFetcher::onReplySslErrors(
    QNetworkReply * pReply, QList<QSslError> errors)
{
    RFDEBUG("NetworkReplyFetcher::onReplySslErrors");

    if (m_finished) {
        RFDEBUG("Already finished, probably due to timeout");
        recycleNetworkReply(pReply);
        return;
    }

    ErrorString errorDescription(QT_TR_NOOP("SSL errors"));

    for (const auto & error: qAsConst(errors)) {
        errorDescription.details() += QStringLiteral("(");
        errorDescription.details() += error.error();
        errorDescription.details() += QStringLiteral(") ");
        errorDescription.details() += error.errorString();
        errorDescription.details() += QStringLiteral("; ");
    }

    finishWithError(errorDescription);
    recycleNetworkReply(pReply);
}

void NetworkReplyFetcher::onDownloadProgress(
    qint64 bytesFetched, qint64 bytesTotal)
{
    RFDEBUG(
        "NetworkReplyFetcher::onDownloadProgress: fetched "
        << bytesFetched << " bytes, total " << bytesTotal << " bytes");

    if (m_finished) {
        RFDEBUG("Already finished, probably due to timeout");
        return;
    }

    m_lastNetworkTime = QDateTime::currentMSecsSinceEpoch();
    Q_EMIT downloadProgress(bytesFetched, bytesTotal);
}

void NetworkReplyFetcher::checkForTimeout()
{
    RFDEBUG("NetworkReplyFetcher::checkForTimeout");

    if (m_finished || !m_started) {
        if (m_pTimeoutTimer) {
            m_pTimeoutTimer->stop();
            m_pTimeoutTimer->deleteLater();
            m_pTimeoutTimer = nullptr;
        }

        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if ((now - m_lastNetworkTime) <= m_timeoutMsec) {
        return;
    }

    ErrorString errorDescription(QT_TR_NOOP("connection timeout"));
    finishWithError(errorDescription);
}

void NetworkReplyFetcher::finishWithError(ErrorString errorDescription)
{
    RFDEBUG("NetworkReplyFetcher::finishWithError: " << errorDescription);

    m_started = false;
    m_finished = true;

    Q_EMIT finished(false, QByteArray(), errorDescription);
}

void NetworkReplyFetcher::recycleNetworkReply(QNetworkReply * pReply)
{
    // NOTE: this is what Qt does since 5.14 when
    // QNetworkAccessManager::setAutoDeleteReplies(true) is called
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QMetaObject::invokeMethod(
        pReply, [pReply] { pReply->deleteLater(); }, Qt::QueuedConnection);
#else
    pReply->deleteLater();
#endif
}

} // namespace quentier
