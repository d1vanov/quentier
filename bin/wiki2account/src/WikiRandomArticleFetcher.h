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

#ifndef QUENTIER_WIKI2ACCOUNT_WIKI_RANDOM_ARTICLE_FETCHER_H
#define QUENTIER_WIKI2ACCOUNT_WIKI_RANDOM_ARTICLE_FETCHER_H

#include <lib/wiki2note/WikiArticleToNote.h>
#include <lib/wiki2note/WikiRandomArticleUrlFetcher.h>

#include <quentier/enml/ENMLConverter.h>

namespace quentier {

class WikiRandomArticleFetcher: public QObject
{
    Q_OBJECT
public:
    explicit WikiRandomArticleFetcher(
        const qint64 timeoutMsec = NETWORK_REPLY_FETCHER_DEFAULT_TIMEOUT_MSEC,
        QObject * parent = nullptr);

    virtual ~WikiRandomArticleFetcher();

    bool isStarted() const { return m_started; }
    bool isFinished() const { return m_finished; }

    const QUrl & url() const { return m_url; }

    const Note & note() const { return m_note; }

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
        bool status, ErrorString errorDescription, Note note);

private:
    void clear();

private:
    ENMLConverter   m_enmlConverter;
    const qint64    m_networkReplyFetcherTimeout;

    bool    m_started;
    bool    m_finished;

    WikiRandomArticleUrlFetcher *   m_pWikiArticleUrlFetcher;
    QUrl    m_url;

    NetworkReplyFetcher *   m_pWikiArticleContentsFetcher;
    WikiArticleToNote *     m_pWikiArticleToNote;

    Note    m_note;
};

} // namespace quentier

#endif // QUENTIER_WIKI2ACCOUNT_WIKI_RANDOM_ARTICLE_FETCHER_H
