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

    // Wiki article fetching takes 80% of the total progress of getting a new
    // note into the local storage; putting the fetched note into the local
    // storage takes the remaining 20%

    it.value() = 0.8;
    updateProgress();

    const Note & note = pFetcher->note();

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_addNoteRequestIds.insert(requestId))
    Q_EMIT addNote(note, requestId);

    m_wikiRandomArticleFetchersWithProgress.erase(it);
}

void WikiArticlesFetcher::onWikiArticleFetchingFailed(ErrorString errorDescription)
{
    QNDEBUG(QStringLiteral("WikiArticlesFetcher::onWikiArticleFetchingFailed: ")
            << errorDescription);

    // TODO: implement
}

void WikiArticlesFetcher::onWikiArticleFetchingProgress(double percentage)
{
    QNDEBUG(QStringLiteral("WikiArticlesFetcher::onWikiArticleFetchingProgress: ")
            << percentage);

    // TODO: implement
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

    // TODO: increment progress
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
    // TODO: implement
}

} // namespace quentier
