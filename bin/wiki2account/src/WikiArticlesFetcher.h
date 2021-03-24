/*
 * Copyright 2019-2021 Dmitry Ivanov
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

#ifndef QUENTIER_WIKI2ACCOUNT_WIKI_ARTICLES_FETCHER_H
#define QUENTIER_WIKI2ACCOUNT_WIKI_ARTICLES_FETCHER_H

#include <quentier/types/ErrorString.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Notebook.h>
#include <qevercloud/types/Tag.h>

#include <QHash>
#include <QObject>
#include <QSet>
#include <QUuid>

namespace quentier {

class LocalStorageManagerAsync;
class WikiRandomArticleFetcher;

class WikiArticlesFetcher final : public QObject
{
    Q_OBJECT
public:
    explicit WikiArticlesFetcher(
        QList<qevercloud::Notebook> notebooks, QList<qevercloud::Tag> tags,
        quint32 minTagsPerNote, quint32 numNotes,
        LocalStorageManagerAsync & localStorageManager,
        QObject * parent = nullptr);

    ~WikiArticlesFetcher() override;

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);
    void progress(double percentage);

    // private signals
    void addNote(qevercloud::Note note, QUuid requestId);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onWikiArticleFetched();
    void onWikiArticleFetchingFailed(ErrorString errorDescription);
    void onWikiArticleFetchingProgress(double percentage);

    void onAddNoteComplete(qevercloud::Note note, QUuid requestId);

    void onAddNoteFailed(
        qevercloud::Note note, ErrorString errorDescription, QUuid requestId);

private:
    void createConnections(LocalStorageManagerAsync & localStorageManager);
    void clear();

    void addTagsToNote(qevercloud::Note & note);

    int nextNotebookIndex();
    void updateProgress();

private:
    QList<qevercloud::Notebook> m_notebooks;
    QList<qevercloud::Tag> m_tags;
    quint32 m_minTagsPerNote;
    quint32 m_numNotes;

    int m_notebookIndex = 0;

    double m_currentProgress = 0.0;

    QHash<WikiRandomArticleFetcher *, double>
        m_wikiRandomArticleFetchersWithProgress;

    QSet<QUuid> m_addNoteRequestIds;
};

} // namespace quentier

#endif // QUENTIER_WIKI2ACCOUNT_WIKI_ARTICLES_FETCHER_H
