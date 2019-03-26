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

#ifndef QUENTIER_LIB_WIKI2NOTE_WIKI_ARTICLE_FETCHER_H
#define QUENTIER_LIB_WIKI2NOTE_WIKI_ARTICLE_FETCHER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>

#include <QObject>
#include <QByteArray>
#include <QNetworkReply>
#include <QList>
#include <QSslError>
#include <QUrl>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QTimer)

namespace quentier {

class WikiArticleFetcher: public QObject
{
    Q_OBJECT
public:
    explicit WikiArticleFetcher(QNetworkAccessManager * pNetworkAccessManager,
                                const QUrl & articleUrl,
                                const qint64 timeoutMsec = 2000,
                                QObject * parent = Q_NULLPTR);
    virtual ~WikiArticleFetcher();

    bool isStarted() const { return m_started; }
    bool isFinished() const { return m_finished; }

    qint64 timeoutMsec() const { return m_timeoutMsec; }
    bool timedOut() const { return m_timedOut; }

    bool status() const { return m_status; }
    int statusCode() const { return m_httpStatusCode; }

    const QByteArray & fetchedData() const { return m_fetchedData; }
    qint64 bytesFetched() const { return m_bytesFetched; }
    qint64 bytesTotal() const { return m_bytesTotal; }

Q_SIGNALS:
    void finished(bool status, QByteArray contents, ErrorString errorDescription);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onReplyFinished();
    void onReplyError(QNetworkReply::NetworkError error);
    void onReplySslErrors(QList<QSslError> errors);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void checkForTimeout();

private:
    void finishWithError(ErrorString errorDescription);

private:
    QNetworkAccessManager * m_pNetworkAccessManager;
    QUrl                    m_wikiArticleUrl;
    QNetworkReply *         m_pNetworkReply;

    bool        m_started;
    bool        m_finished;

    qint64      m_timeoutMsec;
    qint64      m_lastNetworkTime;
    QTimer *    m_timeoutTimer;
    bool        m_timedOut;

    QByteArray  m_fetchedData;
    qint64      m_bytesFetched;
    qint64      m_bytesTotal;

    bool        m_status;
    int         m_httpStatusCode;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIKI2NOTE_WIKI_ARTICLE_FETCHER_H
