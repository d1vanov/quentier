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

#include "WikiArticlesFetcher.h"
#include "WikiRandomArticleFetcher.h"

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

WikiArticlesFetcher::WikiArticlesFetcher(
        QList<Notebook> notebooks, quint32 numNotes,
        LocalStorageManagerAsync & localStorageManager,
        QObject * parent) :
    QObject(parent),
    m_notebooks(notebooks),
    m_numNotes(numNotes),
    m_notebookIndex(0),
    m_currentProgress(0.0),
    m_networkAccessManager(),
    m_wikiRandomArticleFetchersWithProgress(),
    m_addNoteRequestIds()
{
    createConnections(localStorageManager);
}

WikiArticlesFetcher::~WikiArticlesFetcher()
{
    clear();
}

void WikiArticlesFetcher::start()
{
    QNDEBUG(QStringLiteral("WikiArticlesFetcher::start"));

    for(quint32 i = 0; i < m_numNotes; ++i)
    {
        WikiRandomArticleFetcher * pFetcher =
            new WikiRandomArticleFetcher(&m_networkAccessManager);
        m_wikiRandomArticleFetchersWithProgress[pFetcher] = 0.0;

        QObject::connect(pFetcher,
                         QNSIGNAL(WikiRandomArticleFetcher,finished),
                         this,
                         QNSLOT(WikiArticlesFetcher,onWikiArticleFetched));
        QObject::connect(pFetcher,
                         QNSIGNAL(WikiRandomArticleFetcher,failure,ErrorString),
                         this,
                         QNSLOT(WikiArticlesFetcher,onWikiArticleFetchingFailed,
                                ErrorString));
        QObject::connect(pFetcher,
                         QNSIGNAL(WikiRandomArticleFetcher,progress,double),
                         this,
                         QNSLOT(WikiArticlesFetcher,
                                onWikiArticleFetchingProgress,double));

        pFetcher->start();
    }
}

void WikiArticlesFetcher::onWikiArticleFetched()
{
    QNDEBUG(QStringLiteral("WikiArticlesFetcher::onWikiArticleFetched"));

    WikiRandomArticleFetcher * pFetcher =
        qobject_cast<WikiRandomArticleFetcher*>(sender());

    auto it = m_wikiRandomArticleFetchersWithProgress.find(pFetcher);
    if (it == m_wikiRandomArticleFetchersWithProgress.end()) {
        QNWARNING(QStringLiteral("Received wiki article fetched signal from "
                                 "unrecognized WikiRandomArticleFetcher"));
        return;
    }

    Note note = pFetcher->note();

    int notebookIndex = nextNotebookIndex();
    Notebook & notebook = m_notebooks[notebookIndex];
    note.setNotebookLocalUid(notebook.localUid());

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_addNoteRequestIds.insert(requestId))
    Q_EMIT addNote(note, requestId);

    m_wikiRandomArticleFetchersWithProgress.erase(it);
    updateProgress();
}

void WikiArticlesFetcher::onWikiArticleFetchingFailed(ErrorString errorDescription)
{
    QNWARNING(QStringLiteral("WikiArticlesFetcher::onWikiArticleFetchingFailed: ")
              << errorDescription);

    clear();
    Q_EMIT failure(errorDescription);
}

void WikiArticlesFetcher::onWikiArticleFetchingProgress(double percentage)
{
    QNDEBUG(QStringLiteral("WikiArticlesFetcher::onWikiArticleFetchingProgress: ")
            << percentage);

    WikiRandomArticleFetcher * pFetcher =
        qobject_cast<WikiRandomArticleFetcher*>(sender());

    auto it = m_wikiRandomArticleFetchersWithProgress.find(pFetcher);
    if (it == m_wikiRandomArticleFetchersWithProgress.end()) {
        QNWARNING(QStringLiteral("Received wiki article fetching progress signal "
                                 "from unrecognized WikiRandomArticleFetcher"));
        return;
    }

    it.value() = percentage;
    updateProgress();
}

void WikiArticlesFetcher::onAddNoteComplete(Note note, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("WikiArticlesFetcher::onAddNoteComplete: request id = ")
            << requestId);
    QNTRACE(note);

    m_addNoteRequestIds.erase(it);
    updateProgress();

    if (m_wikiRandomArticleFetchersWithProgress.isEmpty() &&
        m_addNoteRequestIds.isEmpty())
    {
        Q_EMIT finished();
    }
}

void WikiArticlesFetcher::onAddNoteFailed(Note note, ErrorString errorDescription,
                                          QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNWARNING(QStringLiteral("WikiArticlesFetcher::onAddNoteFailed: request id = ")
              << requestId << QStringLiteral(", error description: ")
              << errorDescription << QStringLiteral(", note: ") << note);

    m_addNoteRequestIds.erase(it);

    clear();
    Q_EMIT failure(errorDescription);
}

void WikiArticlesFetcher::createConnections(
    LocalStorageManagerAsync & localStorageManager)
{
    QObject::connect(this,
                     QNSIGNAL(WikiArticlesFetcher,addNote,Note,QUuid),
                     &localStorageManager,
                     QNSLOT(LocalStorageManagerAsync,onAddNoteRequest,Note,QUuid));

    QObject::connect(&localStorageManager,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteComplete,Note,QUuid),
                     this,
                     QNSLOT(WikiArticlesFetcher,onAddNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManager,
                     QNSIGNAL(LocalStorageManagerAsync,addNoteFailed,
                              Note,ErrorString,QUuid),
                     this,
                     QNSLOT(WikiArticlesFetcher,onAddNoteFailed,
                            Note,ErrorString,QUuid));
}

void WikiArticlesFetcher::clear()
{
    for(auto it = m_wikiRandomArticleFetchersWithProgress.begin(),
        end = m_wikiRandomArticleFetchersWithProgress.end(); it != end; ++it)
    {
        WikiRandomArticleFetcher * pFetcher = it.key();
        pFetcher->disconnect(this);
        pFetcher->deleteLater();
    }

    m_wikiRandomArticleFetchersWithProgress.clear();

    m_addNoteRequestIds.clear();
}

int WikiArticlesFetcher::nextNotebookIndex()
{
    ++m_notebookIndex;
    if (m_notebookIndex >= m_notebooks.size()) {
        m_notebookIndex = 0;
    }

    return m_notebookIndex;
}

void WikiArticlesFetcher::updateProgress()
{
    QNDEBUG(QStringLiteral("WikiArticlesFetcher::updateProgress"));

    double percentage = 0.0;

    // Fetching the random wiki article's contents and converting it to note
    // takes 80% of the total progress - the remaining 20% is for adding the
    // note to the local storage
    for(auto it = m_wikiRandomArticleFetchersWithProgress.constBegin(),
        end = m_wikiRandomArticleFetchersWithProgress.constEnd(); it != end; ++it)
    {
        percentage += 0.8 * it.value();
    }

    // Add note to local storage requests mean there are 80% fetched notes
    percentage += 0.8 * m_addNoteRequestIds.size();

    // Totally finished notes are those already fetched (with fetcher deleted)
    // and added to the local storage
    quint32 numFetchedNotes = m_numNotes;
    numFetchedNotes -= quint32(std::max(m_wikiRandomArticleFetchersWithProgress.size(), 0));
    numFetchedNotes -= quint32(std::max(m_addNoteRequestIds.size(), 0));

    percentage += std::max(numFetchedNotes, quint32(0));

    // Divide the accumulated number by the number of notes meant to fetch
    percentage /= m_numNotes;

    // Just in case ensure the progress doesn't exceed 1.0
    percentage = std::min(percentage, 1.0);

    QNTRACE(QStringLiteral("Progress: ") << percentage);
    Q_EMIT progress(percentage);
}

} // namespace quentier
