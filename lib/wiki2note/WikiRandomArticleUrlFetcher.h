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

#ifndef QUENTIER_LIB_WIKI2NOTE_WIKI_RANDOM_ARTICLE_URL_FETCHER_H
#define QUENTIER_LIB_WIKI2NOTE_WIKI_RANDOM_ARTICLE_URL_FETCHER_H

#include <lib/network/NetworkReplyFetcher.h>

#include <quentier/types/ErrorString.h>

#include <QObject>
#include <QUrl>

#define WIKI_RANDOM_ARTICLE_URL_FETCHER_TIMEOUT (2000)

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)

namespace quentier {

class WikiRandomArticleUrlFetcher final : public QObject
{
    Q_OBJECT
public:
    explicit WikiRandomArticleUrlFetcher(
        const qint64 timeoutMsec = NETWORK_REPLY_FETCHER_DEFAULT_TIMEOUT_MSEC,
        QObject * parent = nullptr);

    virtual ~WikiRandomArticleUrlFetcher() override;

    bool isStarted() const
    {
        return m_started;
    }

    bool isFinished() const
    {
        return m_finished;
    }

    const QUrl & url() const
    {
        return m_url;
    }

Q_SIGNALS:
    void finished(
        bool status, QUrl randomArticleUrl, ErrorString errorDescription);

    void progress(double percentage);

    // private signals
    void startFetching();

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onDownloadProgress(qint64 bytesFetched, qint64 bytesTotal);

    void onDownloadFinished(
        bool status, QByteArray fetchedData, ErrorString errorDescription);

private:
    qint32 parsePageIdFromFetchedData(
        const QByteArray & fetchedData, ErrorString & errorDescription);

    void finishWithError(const ErrorString & errorDescription);

private:
    const qint64 m_networkReplyFetcherTimeout;

    NetworkReplyFetcher * m_pNetworkReplyFetcher = nullptr;

    bool m_started = false;
    bool m_finished = false;

    QUrl m_url;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIKI2NOTE_WIKI_RANDOM_ARTICLE_URL_FETCHER_H
