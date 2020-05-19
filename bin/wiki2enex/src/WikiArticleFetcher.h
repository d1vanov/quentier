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

#ifndef WIKI2ENEX_WIKI_ARTICLE_FETCHER_H
#define WIKI2ENEX_WIKI_ARTICLE_FETCHER_H

#include <lib/wiki2note/WikiArticleToNote.h>

#include <quentier/enml/ENMLConverter.h>

namespace quentier {

class WikiArticleFetcher: public QObject
{
    Q_OBJECT
public:
    explicit WikiArticleFetcher(
        ENMLConverter & enmlConverter,
        const QUrl & url,
        QObject * parent = nullptr);

    virtual ~WikiArticleFetcher();

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
    void onPageIdFetchingProgress(qint64 bytesFetched, qint64 bytesTotal);

    void onPageIdFetchingFinished(
        bool status, QByteArray fetchedData, ErrorString errorDescription);

    void onWikiArticleDownloadProgress(qint64 bytesFetched, qint64 bytesTotal);

    void onWikiArticleDownloadFinished(
        bool status, QByteArray fetchedData, ErrorString errorDescription);

    void onWikiArticleToNoteProgress(double progress);

    void onWikiArticleToNoteFinished(
        bool status, ErrorString errorDescription, Note note);

private:
    bool composePageIdFetchingUrl(QUrl & url, ErrorString & errorDescription);

    qint32 parsePageIdFromFetchedData(
        const QByteArray & fetchedData, ErrorString & errorDescription);

    void finishWithError(const ErrorString & errorDescription);
    void clear();

private:
    ENMLConverter &         m_enmlConverter;
    QUrl        m_url;

    bool        m_started;
    bool        m_finished;

    Note        m_note;

    // Raw url given in the constructor needs to be converted to the API url
    // using page id. This network reply fetcher queries wiki for page id
    // in order to compose the proper API url
    NetworkReplyFetcher *   m_pApiUrlFetcher;

    // Language code extracted from the startup URL, i.e. en for English wikipedia
    QString     m_languageCode;

    // Article title extracted from the startup URL
    QString     m_articleTitle;

    NetworkReplyFetcher *   m_pArticleContentsFetcher;

    WikiArticleToNote *     m_pWikiArticleToNote;
};

} // namespace quentier

#endif // WIKI2ENEX_WIKI_ARTICLE_FETCHER_H
