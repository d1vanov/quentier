/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/types/ErrorString.h>

#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QSslError>
#include <QUrl>

class QNetworkAccessManager;
class QTimer;

namespace quentier {

class NetworkReplyFetcher final : public QObject
{
    Q_OBJECT
public:
    static constexpr qint64 defaultTimeoutMsec = 30000;

public:
    explicit NetworkReplyFetcher(
        QUrl url, qint64 timeoutMsec = defaultTimeoutMsec,
        QObject * parent = nullptr);

    ~NetworkReplyFetcher() override;

    [[nodiscard]] const QUrl & url() const noexcept
    {
        return m_url;
    }

    [[nodiscard]] bool isStarted() const noexcept
    {
        return m_started;
    }

    [[nodiscard]] bool isFinished() const noexcept
    {
        return m_finished;
    }

    [[nodiscard]] qint64 timeoutMsec() const noexcept
    {
        return m_timeoutMsec;
    }

    [[nodiscard]] bool timedOut() const noexcept
    {
        return m_timedOut;
    }

    [[nodiscard]] bool status() const noexcept
    {
        return m_status;
    }

    [[nodiscard]] int statusCode() const noexcept
    {
        return m_httpStatusCode;
    }

    [[nodiscard]] QByteArray fetchedData() const;

    [[nodiscard]] qint64 bytesFetched() const noexcept
    {
        return m_bytesFetched;
    }

    [[nodiscard]] qint64 bytesTotal() const noexcept
    {
        return m_bytesTotal;
    }

Q_SIGNALS:
    void finished(
        bool status, QByteArray fetchedData, ErrorString errorDescription);

    void downloadProgress(qint64 bytesFetched, qint64 bytesTotal);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onReplyFinished(QNetworkReply * reply);
    void onReplySslErrors(QNetworkReply * reply, QList<QSslError> errors);
    void onDownloadProgress(qint64 bytesFetched, qint64 bytesTotal);
    void checkForTimeout();

private:
    void finishWithError(ErrorString errorDescription);
    void recycleNetworkReply(QNetworkReply * reply);

private:
    Q_DISABLE_COPY_MOVE(NetworkReplyFetcher)

private:
    const QUrl m_url;
    const qint64 m_timeoutMsec = defaultTimeoutMsec;
    QNetworkAccessManager * m_networkAccessManager;

    QByteArray m_fetchedData;

    bool m_started = false;
    bool m_finished = false;

    qint64 m_lastNetworkTime = 0;
    QTimer * m_timeoutTimer = nullptr;
    bool m_timedOut = false;

    qint64 m_bytesFetched = 0;
    qint64 m_bytesTotal = 0;

    bool m_status = false;
    int m_httpStatusCode = 0;
};

} // namespace quentier
