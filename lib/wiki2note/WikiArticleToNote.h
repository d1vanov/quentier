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

#include <lib/network/NetworkReplyFetcher.h>

#include <quentier/enml/Fwd.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Resource.h>

#include <QHash>
#include <QObject>
#include <QUrl>

namespace quentier {

/**
 * The WikiArticleToNote converts the contents of a wiki article to a note
 */
class WikiArticleToNote final : public QObject
{
    Q_OBJECT
public:
    explicit WikiArticleToNote(
        enml::IConverterPtr enmlConverter,
        qint64 timeoutMsec = NetworkReplyFetcher::defaultTimeoutMsec,
        QObject * parent = nullptr);

    ~WikiArticleToNote() override;

    [[nodiscard]] bool isStarted() const noexcept
    {
        return m_started;
    }

    [[nodiscard]] bool isFinished() const noexcept
    {
        return m_finished;
    }

    [[nodiscard]] const qevercloud::Note & note() const noexcept
    {
        return m_note;
    }

    [[nodiscard]] double currentProgress() const noexcept
    {
        return m_progress;
    }

Q_SIGNALS:
    void finished(
        bool status, ErrorString errorDescription, qevercloud::Note note);

    void progress(double progressValue);

public Q_SLOTS:
    void start(QByteArray wikiPageContent);

private Q_SLOTS:
    void onNetworkReplyFetcherFinished(
        bool status, const QByteArray & fetchedData,
        ErrorString errorDescription);

    void onNetworkReplyFetcherProgress(qint64 bytesFetched, qint64 bytesTotal);

private:
    void finishWithError(ErrorString errorDescription);
    void clear();

    void updateProgress();

    [[nodiscard]] QString fetchedWikiArticleToHtml(
        const QByteArray & fetchedData) const;

    [[nodiscard]] bool setupImageDataFetching(ErrorString & errorDescription);

    void createResource(const QByteArray & fetchedData, const QUrl & url);

    void convertHtmlToEnmlAndComposeNote();
    [[nodiscard]] bool preprocessHtmlForConversionToEnml();

private:
    const enml::IConverterPtr m_enmlConverter;
    const qint64 m_networkReplyFetcherTimeout;

    qevercloud::Note m_note;

    bool m_started = false;
    bool m_finished = false;

    QHash<NetworkReplyFetcher *, double> m_imageDataFetchersWithProgress;

    // Resources created from imgs downloaded by fetchers by imgs' urls
    QHash<QUrl, qevercloud::Resource> m_imageResourcesByUrl;

    // Cleaned up wiki article's HTML
    QString m_html;

    double m_progress = 0.0;
};

} // namespace quentier
