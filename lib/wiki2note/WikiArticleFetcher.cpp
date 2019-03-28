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

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/Utility.h>

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QDebug>

#define WALOG_IMPL(message, macro) \
    macro(QStringLiteral("<") << m_wikiArticleUrl << QStringLiteral(">: ") << message)

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

#define WIKI_ARTICLE_FETCHER_TIMEOUT_CHECKER_INTERVAL (1000)

namespace quentier {

WikiArticleFetcher::WikiArticleFetcher(QNetworkAccessManager * pNetworkAccessManager,
                                       const QUrl & articleUrl,
                                       const qint64 timeoutMsec,
                                       QObject * parent) :
    m_pNetworkAccessManager(pNetworkAccessManager),
    m_wikiArticleUrl(articleUrl),
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

WikiArticleFetcher::~WikiArticleFetcher()
{
    clear();
}

QByteArray WikiArticleFetcher::fetchedData() const
{
    if (!m_finished || !m_pNetworkReply) {
        return QByteArray();
    }

    return m_pNetworkReply->readAll();
}

void WikiArticleFetcher::start()
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::start"));

    if (m_started) {
        WADEBUG(QStringLiteral("Already started"));
        return;
    }

    clear();

    m_started = true;

    m_lastNetworkTime = QDateTime::currentMSecsSinceEpoch();

    m_pTimeoutTimer = new QTimer(this);
    QObject::connect(m_pTimeoutTimer, QNSIGNAL(QTimer,timeout),
                     this, QNSLOT(WikiArticleFetcher,checkForTimeout));
    m_pTimeoutTimer->start(WIKI_ARTICLE_FETCHER_TIMEOUT_CHECKER_INTERVAL);

    QNetworkRequest request;
    request.setUrl(m_wikiArticleUrl);
    m_pNetworkReply = m_pNetworkAccessManager->get(request);

    QObject::connect(m_pNetworkReply, QNSIGNAL(QNetworkReply,finished),
                     this, QNSLOT(WikiArticleFetcher,onReplyFinished));

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(m_pNetworkReply,
                     QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error),
                     this,
                     QNSLOT(WikiArticleFetcher,onReplyError,
                            QNetworkReply::NetworkError));
#else
    QObject::connect(m_pNetworkReply, SIGNAL(error(QNetworkReply::NetworkError)),
                     this, SLOT(onReplyError(QNetworkReply::NetworkError)));
#endif

    QObject::connect(m_pNetworkReply,
                     QNSIGNAL(QNetworkReply,sslErrors,QList<QSslError>),
                     this,
                     QNSLOT(WikiArticleFetcher,onReplySslErrors,QList<QSslError>));
    QObject::connect(m_pNetworkReply,
                     QNSIGNAL(QNetworkReply,downloadProgress,qint64,qint64),
                     this,
                     QNSLOT(WikiArticleFetcher,onDownloadProgress,qint64,qint64));
}

void WikiArticleFetcher::onReplyFinished()
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onReplyFinished"));

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
        ErrorString errorDescription(QT_TR_NOOP("Failed to convert HTTP status code to int"));

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

void WikiArticleFetcher::onReplyError(QNetworkReply::NetworkError error)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onReplyError: ") << error);

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

void WikiArticleFetcher::onReplySslErrors(QList<QSslError> errors)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onReplySslErrors"));

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

void WikiArticleFetcher::onDownloadProgress(qint64 bytesFetched, qint64 bytesTotal)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::onDownloadProgress: fetched ")
            << humanReadableSize(bytesFetched) << QStringLiteral(", total ")
            << humanReadableSize(bytesTotal));

    m_lastNetworkTime = QDateTime::currentMSecsSinceEpoch();
    Q_EMIT downloadProgress(bytesFetched, bytesTotal);
}

void WikiArticleFetcher::checkForTimeout()
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::checkForTimeout"));

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if ((now - m_lastNetworkTime) <= WIKI_ARTICLE_FETCHER_TIMEOUT) {
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

void WikiArticleFetcher::clear()
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::clear"));

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

void WikiArticleFetcher::finishWithError(ErrorString errorDescription)
{
    WADEBUG(QStringLiteral("WikiArticleFetcher::finishWithError: ")
            << errorDescription);

    m_started = false;
    m_finished = true;

    Q_EMIT finished(false, QByteArray(), errorDescription);
}

} // namespace quentier
