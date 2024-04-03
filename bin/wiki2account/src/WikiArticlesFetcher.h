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

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/cancelers/Fwd.h>

#include <qevercloud/types/Note.h>
#include <qevercloud/types/Notebook.h>
#include <qevercloud/types/Tag.h>

#include <QHash>
#include <QObject>
#include <QSet>

namespace quentier {

class WikiRandomArticleFetcher;

class WikiArticlesFetcher : public QObject
{
    Q_OBJECT
public:
    explicit WikiArticlesFetcher(
        QList<qevercloud::Notebook> notebooks, QList<qevercloud::Tag> tags,
        quint32 minTagsPerNote, quint32 numNotes,
        local_storage::ILocalStoragePtr localStorage,
        QObject * parent = nullptr);

    ~WikiArticlesFetcher() override;

Q_SIGNALS:
    void finished();
    void failure(ErrorString errorDescription);
    void progress(double percentage);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onWikiArticleFetched();
    void onWikiArticleFetchingFailed(ErrorString errorDescription);
    void onWikiArticleFetchingProgress(double percentage);

private:
    void clear();

    [[nodiscard]] bool checkFinish();
    void startFetchersBatch();

    void addTagsToNote(qevercloud::Note & note);

    [[nodiscard]] int nextNotebookIndex();
    void updateProgress();

    [[nodiscard]] utility::cancelers::ICancelerPtr setupCanceler();

private:
    const local_storage::ILocalStoragePtr m_localStorage;
    const quint32 m_minTagsPerNote;
    const quint32 m_numNotes;
    const QList<qevercloud::Notebook> m_notebooks;
    const QList<qevercloud::Tag> m_tags;
    const quint32 m_maxRunningFetchersCount = 100;

    int m_notebookIndex = 0;
    double m_currentProgress = 0.0;

    quint32 m_pendingNotesCount = 0;
    quint32 m_finishedFetchersCount = 0;

    QHash<WikiRandomArticleFetcher *, double>
        m_wikiRandomArticleFetchersWithProgress;

    QSet<QString> m_noteLocalIdsPendingPutNoteToLocalStorage;

    utility::cancelers::ManualCancelerPtr m_canceler;
};

} // namespace quentier
