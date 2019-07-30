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

#include "NetworkReplyFetcher.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Utility.h>

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QDebug>

#define RFLOG_IMPL(message, macro)                                             \
    macro("<" << m_url << ">: " << message)                                    \
// RFLOG_IMPL

#define RFTRACE(message) RFLOG_IMPL(message, QNTRACE)
#define RFDEBUG(message) RFLOG_IMPL(message, QNDEBUG)
#define RFINFO(message) RFLOG_IMPL(message, QNINFO)
#define RFWARNING(message) RFLOG_IMPL(message, QNWARNING)
#define RFCRITICAL(message) RFLOG_IMPL(message, QNCRITICAL)
#define RFFATAL(message) RFLOG_IMPL(message, QNFATAL)

#define NETWORK_REPLY_FETCHER_TIMEOUT_CHECKER_INTERVAL (1000)

namespace quentier {

NetworkReplyFetcher::NetworkReplyFetcher(
        QNetworkAccessManager * pNetworkAccessManager,
        const QUrl & url,
        const qint64 timeoutMsec,
        QObject * parent) :
    QObject(parent),
    m_pNetworkAccessManager(pNetworkAccessManager),
    m_url(url),
    m_pNetworkReply(Q_NULLPTR),
    m_started(false),
    m_finished(false),
    m_timeoutMsec(timeoutMsec),
    m_lastNetworkTime(0),
    m_pTimeoutTimer(Q_NULLPTR),
    m_timedOut(false),
    m_bytesFetched(0),
    m_bytesTotal(0),
    m_status(false),
    m_httpStatusCode(0)
{}

NetworkReplyFetcher::~NetworkReplyFetcher()
{
    clear();
}

QByteArray NetworkReplyFetcher::fetchedData() const
{
    if (!m_finished || !m_pNetworkReply) {
        return QByteArray();
    }

    return m_pNetworkReply->readAll();
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
        QObject::connect(m_pTimeoutTimer, QNSIGNAL(QTimer,timeout),
                         this, QNSLOT(NetworkReplyFetcher,checkForTimeout));
        m_pTimeoutTimer->start(NETWORK_REPLY_FETCHER_TIMEOUT_CHECKER_INTERVAL);
    }

    QNetworkRequest request;
    request.setUrl(m_url);
    m_pNetworkReply = m_pNetworkAccessManager->get(request);

    QObject::connect(m_pNetworkReply, QNSIGNAL(QNetworkReply,finished),
                     this, QNSLOT(NetworkReplyFetcher,onReplyFinished));

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(m_pNetworkReply,
                     QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                     this,
                     QNSLOT(NetworkReplyFetcher,onReplyError,
                            QNetworkReply::NetworkError));
#else
    QObject::connect(m_pNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)),
                     this, SLOT(onReplyError(QNetworkReply::NetworkError)));
#endif

    QObject::connect(m_pNetworkReply,
                     QNSIGNAL(QNetworkReply,sslErrors,QList<QSslError>),
                     this,
                     QNSLOT(NetworkReplyFetcher,onReplySslErrors,QList<QSslError>));
    QObject::connect(m_pNetworkReply,
                     QNSIGNAL(QNetworkReply,downloadProgress,qint64,qint64),
                     this,
                     QNSLOT(NetworkReplyFetcher,onDownloadProgress,qint64,qint64));
}

void NetworkReplyFetcher::onReplyFinished()
{
    RFDEBUG("NetworkReplyFetcher::onReplyFinished");

    if (m_pTimeoutTimer) {
        m_pTimeoutTimer->stop();
        m_pTimeoutTimer->disconnect(this);
        m_pTimeoutTimer->deleteLater();
        m_pTimeoutTimer = Q_NULLPTR;
    }

    QVariant statusCodeAttribute =
        m_pNetworkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool conversionResult = false;
    m_httpStatusCode = statusCodeAttribute.toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Failed to convert HTTP status code to int"));

        QString str;
        QDebug dbg(&str);
        dbg << statusCodeAttribute;

        errorDescription.details() += str;
        finishWithError(errorDescription);
        return;
    }

    m_started = false;
    m_finished = true;

    QByteArray fetchedData = m_pNetworkReply->readAll();
    Q_EMIT finished(true, fetchedData, ErrorString());
}

void NetworkReplyFetcher::onReplyError(QNetworkReply::NetworkError error)
{
    RFDEBUG("NetworkReplyFetcher::onReplyError: " << error);

    ErrorString errorDescription(QT_TR_NOOP("network error"));
    errorDescription.details() += QStringLiteral("(");
    errorDescription.details() += QString::number(error);
    errorDescription.details() += QStringLiteral(")");

    if (m_pNetworkReply) {
        errorDescription.details() += QStringLiteral(" ");
        errorDescription.details() += m_pNetworkReply->errorString();
    }

    finishWithError(errorDescription);
}

void NetworkReplyFetcher::onReplySslErrors(QList<QSslError> errors)
{
    RFDEBUG("NetworkReplyFetcher::onReplySslErrors");

    ErrorString errorDescription(QT_TR_NOOP("SSL errors"));

    for(auto it = errors.constBegin(), end = errors.constEnd(); it != end; ++it) {
        const QSslError & error = *it;
        errorDescription.details() += QStringLiteral("(");
        errorDescription.details() += error.error();
        errorDescription.details() += QStringLiteral(") ");
        errorDescription.details() += error.errorString();
        errorDescription.details() += QStringLiteral("; ");
    }

    finishWithError(errorDescription);
}

void NetworkReplyFetcher::onDownloadProgress(qint64 bytesFetched, qint64 bytesTotal)
{
    RFDEBUG("NetworkReplyFetcher::onDownloadProgress: fetched "
            << bytesFetched << " bytes, total " << bytesTotal << " bytes");

    m_lastNetworkTime = QDateTime::currentMSecsSinceEpoch();
    Q_EMIT downloadProgress(bytesFetched, bytesTotal);
}

void NetworkReplyFetcher::checkForTimeout()
{
    RFDEBUG("NetworkReplyFetcher::checkForTimeout");

    if (m_finished || !m_started)
    {
        if (m_pTimeoutTimer) {
            m_pTimeoutTimer->stop();
            m_pTimeoutTimer->deleteLater();
            m_pTimeoutTimer = Q_NULLPTR;
        }

        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if ((now - m_lastNetworkTime) <= m_timeoutMsec) {
        return;
    }

    if (m_pNetworkReply) {
        m_pNetworkReply->disconnect(this);
        m_pNetworkReply->deleteLater();
        m_pNetworkReply = Q_NULLPTR;
    }

    ErrorString errorDescription(QT_TR_NOOP("connection timeout"));
    finishWithError(errorDescription);
}

void NetworkReplyFetcher::clear()
{
    RFDEBUG("NetworkReplyFetcher::clear");

    m_started = false;
    m_finished = false;

    if (m_pNetworkReply) {
        m_pNetworkReply->disconnect(this);
        m_pNetworkReply->deleteLater();
        m_pNetworkReply = Q_NULLPTR;
    }

    if (m_pTimeoutTimer) {
        m_pTimeoutTimer->disconnect(this);
        m_pTimeoutTimer->deleteLater();
        m_pTimeoutTimer = Q_NULLPTR;
    }

    m_lastNetworkTime = 0;
    m_timedOut = false;
    m_bytesFetched = 0;
    m_bytesTotal = 0;

    m_status = false;
    m_httpStatusCode = 0;
}

void NetworkReplyFetcher::finishWithError(ErrorString errorDescription)
{
    RFDEBUG("NetworkReplyFetcher::finishWithError: " << errorDescription);

    m_started = false;
    m_finished = true;

    Q_EMIT finished(false, QByteArray(), errorDescription);
}

} // namespace quentier
