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

#include "WikiArticlesFetcher.h"
#include "WikiRandomArticleFetcher.h"

#include <quentier/local_storage/LocalStorageManagerAsync.h>
#include <quentier/logging/QuentierLogger.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace quentier {

WikiArticlesFetcher::WikiArticlesFetcher(
        QList<Notebook> notebooks, QList<Tag> tags, quint32 minTagsPerNote,
        quint32 numNotes, LocalStorageManagerAsync & localStorageManager,
        QObject * parent) :
    QObject(parent),
    m_notebooks(notebooks),
    m_tags(tags),
    m_minTagsPerNote(minTagsPerNote),
    m_numNotes(numNotes)
{
    createConnections(localStorageManager);
}

WikiArticlesFetcher::~WikiArticlesFetcher()
{
    clear();
}

void WikiArticlesFetcher::start()
{
    QNDEBUG("wiki2account", "WikiArticlesFetcher::start");

    const qint64 timeoutMsec = -1;
    for(quint32 i = 0; i < m_numNotes; ++i)
    {
        auto * pFetcher = new WikiRandomArticleFetcher(timeoutMsec);
        m_wikiRandomArticleFetchersWithProgress[pFetcher] = 0.0;

        QObject::connect(
            pFetcher,
            &WikiRandomArticleFetcher::finished,
            this,
            &WikiArticlesFetcher::onWikiArticleFetched);

        QObject::connect(
            pFetcher,
            &WikiRandomArticleFetcher::failure,
            this,
            &WikiArticlesFetcher::onWikiArticleFetchingFailed);

        QObject::connect(
            pFetcher,
            &WikiRandomArticleFetcher::progress,
            this,
            &WikiArticlesFetcher::onWikiArticleFetchingProgress);

        pFetcher->start();
    }
}

void WikiArticlesFetcher::onWikiArticleFetched()
{
    QNDEBUG("wiki2account", "WikiArticlesFetcher::onWikiArticleFetched");

    auto * pFetcher = qobject_cast<WikiRandomArticleFetcher*>(sender());
    auto it = m_wikiRandomArticleFetchersWithProgress.find(pFetcher);
    if (it == m_wikiRandomArticleFetchersWithProgress.end()) {
        QNWARNING("wiki2account", "Received wiki article fetched signal from "
            << "unrecognized WikiRandomArticleFetcher");
        return;
    }

    auto note = pFetcher->note();

    int notebookIndex = nextNotebookIndex();
    auto & notebook = m_notebooks[notebookIndex];
    note.setNotebookLocalUid(notebook.localUid());

    addTagsToNote(note);

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_addNoteRequestIds.insert(requestId))
    Q_EMIT addNote(note, requestId);

    m_wikiRandomArticleFetchersWithProgress.erase(it);
    updateProgress();
}

void WikiArticlesFetcher::onWikiArticleFetchingFailed(
    ErrorString errorDescription)
{
    QNWARNING(
        "wiki2account",
        "WikiArticlesFetcher::onWikiArticleFetchingFailed: "
              << errorDescription);

    clear();
    Q_EMIT failure(errorDescription);
}

void WikiArticlesFetcher::onWikiArticleFetchingProgress(double percentage)
{
    QNDEBUG(
        "wiki2account",
        "WikiArticlesFetcher::onWikiArticleFetchingProgress: "
            << percentage);

    auto * pFetcher = qobject_cast<WikiRandomArticleFetcher*>(sender());
    auto it = m_wikiRandomArticleFetchersWithProgress.find(pFetcher);
    if (it == m_wikiRandomArticleFetchersWithProgress.end()) {
        QNWARNING("wiki2account", "Received wiki article fetching progress "
            << "signal from unrecognized WikiRandomArticleFetcher");
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

    QNDEBUG("wiki2account", "WikiArticlesFetcher::onAddNoteComplete: "
        << "request id = " << requestId);

    QNTRACE("wiki2account", note);

    m_addNoteRequestIds.erase(it);
    updateProgress();

    if (m_wikiRandomArticleFetchersWithProgress.isEmpty() &&
        m_addNoteRequestIds.isEmpty())
    {
        Q_EMIT finished();
    }
}

void WikiArticlesFetcher::onAddNoteFailed(
    Note note, ErrorString errorDescription, QUuid requestId)
{
    auto it = m_addNoteRequestIds.find(requestId);
    if (it == m_addNoteRequestIds.end()) {
        return;
    }

    QNWARNING("wiki2account", "WikiArticlesFetcher::onAddNoteFailed: "
        << "request id = " << requestId << ", error description: "
        << errorDescription << ", note: " << note);

    m_addNoteRequestIds.erase(it);

    clear();
    Q_EMIT failure(errorDescription);
}

void WikiArticlesFetcher::createConnections(
    LocalStorageManagerAsync & localStorageManager)
{
    QObject::connect(
        this,
        &WikiArticlesFetcher::addNote,
        &localStorageManager,
        &LocalStorageManagerAsync::onAddNoteRequest);

    QObject::connect(
        &localStorageManager,
        &LocalStorageManagerAsync::addNoteComplete,
        this,
        &WikiArticlesFetcher::onAddNoteComplete);

    QObject::connect(
        &localStorageManager,
        &LocalStorageManagerAsync::addNoteFailed,
        this,
        &WikiArticlesFetcher::onAddNoteFailed);
}

void WikiArticlesFetcher::clear()
{
    for(auto it = m_wikiRandomArticleFetchersWithProgress.begin(),
        end = m_wikiRandomArticleFetchersWithProgress.end(); it != end; ++it)
    {
        auto * pFetcher = it.key();
        pFetcher->disconnect(this);
        pFetcher->deleteLater();
    }

    m_wikiRandomArticleFetchersWithProgress.clear();

    m_addNoteRequestIds.clear();
}

void WikiArticlesFetcher::addTagsToNote(Note & note)
{
    QNDEBUG("wiki2account", "WikiArticlesFetcher::addTagsToNote");

    if (m_tags.isEmpty()) {
        QNDEBUG("wiki2account", "No tags to assign to note");
        return;
    }

    int lowest = static_cast<int>(m_minTagsPerNote);
    int highest = m_tags.size();

    // Protect from the case in which lowest > highest
    lowest = std::min(lowest, highest);

    int range = (highest - lowest) + 1;
    int randomValue = std::rand();
    int numTags =
        lowest +
        static_cast<int>(
            std::floor(static_cast<double>(range) *
                       static_cast<double>(randomValue) / (RAND_MAX + 1.0)));

    QNTRACE("wiki2account", "Adding " << numTags << " tags to note");
    for(int i = 0; i < numTags; ++i) {
        note.addTagLocalUid(m_tags[i].localUid());
    }
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
    QNDEBUG("wiki2account", "WikiArticlesFetcher::updateProgress");

    double percentage = 0.0;

    // Fetching the random wiki article's contents and converting it to note
    // takes 80% of the total progress - the remaining 20% is for adding the
    // note to the local storage
    for(auto it = m_wikiRandomArticleFetchersWithProgress.constBegin(),
        end = m_wikiRandomArticleFetchersWithProgress.constEnd();
        it != end; ++it)
    {
        percentage += 0.8 * it.value();
    }

    // Add note to local storage requests mean there are 80% fetched notes
    percentage += 0.8 * m_addNoteRequestIds.size();

    // Totally finished notes are those already fetched (with fetcher deleted)
    // and added to the local storage
    quint32 numFetchedNotes = m_numNotes;

    numFetchedNotes -= quint32(
        std::max(m_wikiRandomArticleFetchersWithProgress.size(), 0));

    numFetchedNotes -= quint32(std::max(m_addNoteRequestIds.size(), 0));

    percentage += std::max(numFetchedNotes, quint32(0));

    // Divide the accumulated number by the number of notes meant to fetch
    percentage /= m_numNotes;

    // Just in case ensure the progress doesn't exceed 1.0
    percentage = std::min(percentage, 1.0);

    QNTRACE("wiki2account", "Progress: " << percentage);
    Q_EMIT progress(percentage);
}

} // namespace quentier
