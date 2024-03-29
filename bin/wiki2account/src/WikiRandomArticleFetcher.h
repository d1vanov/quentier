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

#include <lib/wiki2note/WikiArticleToNote.h>
#include <lib/wiki2note/WikiRandomArticleUrlFetcher.h>

#include <quentier/enml/Fwd.h>

namespace quentier {

class WikiRandomArticleFetcher : public QObject
{
    Q_OBJECT
public:
    explicit WikiRandomArticleFetcher(
        qint64 timeoutMsec = NetworkReplyFetcher::defaultTimeoutMsec,
        QObject * parent = nullptr);

    ~WikiRandomArticleFetcher() override;

    [[nodiscard]] bool isStarted() const noexcept
    {
        return m_started;
    }

    [[nodiscard]] bool isFinished() const noexcept
    {
        return m_finished;
    }

    [[nodiscard]] const QUrl & url() const noexcept
    {
        return m_url;
    }

    [[nodiscard]] const qevercloud::Note & note() const noexcept
    {
        return m_note;
    }

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);
    void progress(double percentage);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onRandomArticleUrlFetchProgress(double percentage);

    void onRandomArticleUrlFetchFinished(
        bool status, QUrl randomArticleUrl, ErrorString errorDescription);

    void onWikiArticleDownloadProgress(qint64 bytesFetched, qint64 bytesTotal);

    void onWikiArticleDownloadFinished(
        bool status, QByteArray fetchedData, ErrorString errorDescription);

    void onWikiArticleToNoteProgress(double percentage);

    void onWikiArticleToNoteFinished(
        bool status, ErrorString errorDescription, qevercloud::Note note);

private:
    void clear();

private:
    const enml::IConverterPtr m_enmlConverter;
    const qint64 m_networkReplyFetcherTimeout;

    bool m_started = false;
    bool m_finished = false;

    WikiRandomArticleUrlFetcher * m_wikiArticleUrlFetcher = nullptr;
    QUrl m_url;

    NetworkReplyFetcher * m_wikiArticleContentsFetcher = nullptr;
    WikiArticleToNote * m_wikiArticleToNote = nullptr;

    qevercloud::Note m_note;
};

} // namespace quentier
