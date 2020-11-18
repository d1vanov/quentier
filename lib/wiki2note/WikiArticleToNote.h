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

#ifndef QUENTIER_LIB_WIKI2NOTE_WIKI_ARTICLE_TO_NOTE_H
#define QUENTIER_LIB_WIKI2NOTE_WIKI_ARTICLE_TO_NOTE_H

#include <lib/network/NetworkReplyFetcher.h>

#include <quentier/types/Note.h>
#include <quentier/types/Resource.h>

#include <QHash>
#include <QObject>
#include <QUrl>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ENMLConverter)

/**
 * The WikiArticleToNote converts the contents of a wiki article to a note
 */
class WikiArticleToNote final : public QObject
{
    Q_OBJECT
public:
    explicit WikiArticleToNote(
        ENMLConverter & enmlConverter,
        const qint64 timeoutMsec = NETWORK_REPLY_FETCHER_DEFAULT_TIMEOUT_MSEC,
        QObject * parent = nullptr);

    virtual ~WikiArticleToNote() override;

    bool isStarted() const
    {
        return m_started;
    }

    bool isFinished() const
    {
        return m_finished;
    }

    const Note & note() const
    {
        return m_note;
    }

    double currentProgress() const
    {
        return m_progress;
    }

Q_SIGNALS:
    void finished(bool status, ErrorString errorDescription, Note note);
    void progress(double progressValue);

public Q_SLOTS:
    void start(QByteArray wikiPageContent);

private Q_SLOTS:
    void onNetworkReplyFetcherFinished(
        bool status, QByteArray fetchedData, ErrorString errorDescription);

    void onNetworkReplyFetcherProgress(qint64 bytesFetched, qint64 bytesTotal);

private:
    void finishWithError(ErrorString errorDescription);
    void clear();

    void updateProgress();

    QString fetchedWikiArticleToHtml(const QByteArray & fetchedData) const;
    bool setupImageDataFetching(ErrorString & errorDescription);

    void createResource(const QByteArray & fetchedData, const QUrl & url);

    void convertHtmlToEnmlAndComposeNote();
    bool preprocessHtmlForConversionToEnml();

private:
    ENMLConverter & m_enmlConverter;
    const qint64 m_networkReplyFetcherTimeout;

    Note m_note;

    bool m_started = false;
    bool m_finished = false;

    QHash<NetworkReplyFetcher *, double> m_imageDataFetchersWithProgress;

    // Resources created from imgs downloaded by fetchers by imgs' urls
    QHash<QUrl, Resource> m_imageResourcesByUrl;

    // Cleaned up wiki article's HTML
    QString m_html;

    double m_progress = 0.0;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIKI2NOTE_WIKI_ARTICLE_TO_NOTE_H
