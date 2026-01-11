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

#include "WikiArticlesFetcher.h"
#include "WikiRandomArticleFetcher.h"

#include <lib/exception/Utils.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/utility/cancelers/ManualCanceler.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace quentier {

WikiArticlesFetcher::WikiArticlesFetcher(
    QList<qevercloud::Notebook> notebooks, QList<qevercloud::Tag> tags,
    const quint32 minTagsPerNote, const quint32 numNotes,
    local_storage::ILocalStoragePtr localStorage, QObject * parent) :
    QObject{parent}, m_localStorage{std::move(localStorage)},
    m_minTagsPerNote{minTagsPerNote}, m_numNotes{numNotes},
    m_notebooks{std::move(notebooks)}, m_tags{std::move(tags)}
{}

WikiArticlesFetcher::~WikiArticlesFetcher()
{
    clear();
}

void WikiArticlesFetcher::start()
{
    QNINFO("wiki2account::WikiArticlesFetcher", "WikiArticlesFetcher::start");
    startFetchersBatch();
}

void WikiArticlesFetcher::onWikiArticleFetched()
{
    QNDEBUG(
        "wiki2account::WikiArticlesFetcher",
        "WikiArticlesFetcher::onWikiArticleFetched");

    auto * fetcher = qobject_cast<WikiRandomArticleFetcher *>(sender());
    const auto it = m_wikiRandomArticleFetchersWithProgress.find(fetcher);
    if (it == m_wikiRandomArticleFetchersWithProgress.end()) {
        QNWARNING(
            "wiki2account::WikiArticlesFetcher",
            "Received wiki article fetched signal from unrecognized "
            "WikiRandomArticleFetcher instance");
        return;
    }

    ++m_finishedFetchersCount;
    startFetchersBatch();

    auto note = fetcher->note();

    m_wikiRandomArticleFetchersWithProgress.erase(it);
    updateProgress();

    const int notebookIndex = nextNotebookIndex();
    const auto & notebook = m_notebooks[notebookIndex];
    note.setNotebookLocalId(notebook.localId());

    addTagsToNote(note);

    auto noteLocalId = note.localId();
    m_noteLocalIdsPendingPutNoteToLocalStorage.insert(noteLocalId);

    auto canceler = setupCanceler();
    Q_ASSERT(canceler);

    QNDEBUG(
        "wiki2account::WikiArticlesFetcher",
        "Trying to put note to local storage: note local id = " << noteLocalId);

    auto putNoteFuture = m_localStorage->putNote(std::move(note));
    auto putNoteThenFuture = threading::then(
        std::move(putNoteFuture), this, [this, noteLocalId, canceler] {
            if (canceler->isCanceled()) {
                return;
            }

            QNDEBUG(
                "wiki2account::WikiArticlesFetcher",
                "Successfully put note to local storage, note local id = "
                    << noteLocalId);

            m_noteLocalIdsPendingPutNoteToLocalStorage.remove(noteLocalId);
            updateProgress();

            if (checkFinish()) {
                QNINFO(
                    "wiki2account::WikiArticlesFetcher",
                    "Finished fetching notes");

                Q_EMIT finished();
                return;
            }

            QNDEBUG(
                "wiki2account::WikiArticlesFetcher",
                "Still have "
                    << m_wikiRandomArticleFetchersWithProgress.size()
                    << " active fetchers and pending "
                    << m_noteLocalIdsPendingPutNoteToLocalStorage.size()
                    << " notes being put to local storage");
        });

    threading::onFailed(
        std::move(putNoteThenFuture), this,
        [this, noteLocalId = std::move(noteLocalId),
         canceler = std::move(canceler)](const QException & e) {
            auto message = exceptionMessage(e);

            ErrorString error{
                QT_TR_NOOP("Failed to put note to local storage")};
            error.appendBase(message.base());
            error.appendBase(message.additionalBases());
            error.details() = message.details();
            QNWARNING(
                "wiki2account::WikiArticlesFetcher",
                error << ", note local id = " << noteLocalId);

            clear();
            Q_EMIT failure(std::move(error));
        });
}

void WikiArticlesFetcher::onWikiArticleFetchingFailed(
    ErrorString errorDescription)
{
    QNWARNING(
        "wiki2account::WikiArticlesFetcher",
        "WikiArticlesFetcher::onWikiArticleFetchingFailed: "
            << errorDescription);

    clear();
    Q_EMIT failure(std::move(errorDescription));
}

void WikiArticlesFetcher::onWikiArticleFetchingProgress(const double percentage)
{
    QNDEBUG(
        "wiki2account::WikiArticlesFetcher",
        "WikiArticlesFetcher::onWikiArticleFetchingProgress: " << percentage);

    auto * fetcher = qobject_cast<WikiRandomArticleFetcher *>(sender());
    auto it = m_wikiRandomArticleFetchersWithProgress.find(fetcher);
    if (it == m_wikiRandomArticleFetchersWithProgress.end()) {
        QNWARNING(
            "wiki2account::WikiArticlesFetcher",
            "Received wiki article fetching progress "
                << "signal from unrecognized WikiRandomArticleFetcher");
        return;
    }

    it.value() = percentage;
    updateProgress();
}

void WikiArticlesFetcher::clear()
{
    for (auto it = m_wikiRandomArticleFetchersWithProgress.begin(),
              end = m_wikiRandomArticleFetchersWithProgress.end();
         it != end; ++it)
    {
        auto * fetcher = it.key();
        fetcher->disconnect(this);
        fetcher->deleteLater();
    }

    if (m_canceler) {
        m_canceler->cancel();
        m_canceler.reset();
    }

    m_pendingNotesCount = 0;
    m_finishedFetchersCount = 0;

    m_wikiRandomArticleFetchersWithProgress.clear();
    m_noteLocalIdsPendingPutNoteToLocalStorage.clear();
}

bool WikiArticlesFetcher::checkFinish()
{
    return m_wikiRandomArticleFetchersWithProgress.isEmpty() &&
        m_noteLocalIdsPendingPutNoteToLocalStorage.isEmpty() &&
        m_pendingNotesCount == 0 && m_finishedFetchersCount == m_numNotes;
}

void WikiArticlesFetcher::startFetchersBatch()
{
    QNDEBUG(
        "wiki2account::WikiArticlesFetcher",
        "WikiArticlesFetcher::startFetchersBatch: "
            << "max running fetchers count = " << m_maxRunningFetchersCount
            << ", running fetchers count = "
            << m_wikiRandomArticleFetchersWithProgress.size()
            << ", pending notes count = " << m_pendingNotesCount
            << ", finished fetchers count = " << m_finishedFetchersCount);

    int startedFetchers = 0;
    quint32 runningFetchersCount = static_cast<quint32>(
        std::max<decltype(m_wikiRandomArticleFetchersWithProgress.size())>(
            m_wikiRandomArticleFetchersWithProgress.size(), 0));
    while (runningFetchersCount < m_maxRunningFetchersCount &&
           runningFetchersCount + m_finishedFetchersCount < m_numNotes)
    {
        auto * fetcher = new WikiRandomArticleFetcher;
        m_wikiRandomArticleFetchersWithProgress[fetcher] = 0.0;

        QObject::connect(
            fetcher, &WikiRandomArticleFetcher::finished, this,
            &WikiArticlesFetcher::onWikiArticleFetched, Qt::QueuedConnection);

        QObject::connect(
            fetcher, &WikiRandomArticleFetcher::failure, this,
            &WikiArticlesFetcher::onWikiArticleFetchingFailed,
            Qt::QueuedConnection);

        QObject::connect(
            fetcher, &WikiRandomArticleFetcher::progress, this,
            &WikiArticlesFetcher::onWikiArticleFetchingProgress,
            Qt::QueuedConnection);

        fetcher->start();

        ++startedFetchers;

        runningFetchersCount = static_cast<quint32>(
            std::max<decltype(m_wikiRandomArticleFetchersWithProgress.size())>(
                m_wikiRandomArticleFetchersWithProgress.size(), 0));
    }

    QNDEBUG(
        "wiki2account::WikiArticlesFetcher",
        "WikiArticlesFetcher::startFetchersBatch: started " << startedFetchers
                                                            << " new fetchers");
}

void WikiArticlesFetcher::addTagsToNote(qevercloud::Note & note)
{
    QNDEBUG(
        "wiki2account::WikiArticlesFetcher",
        "WikiArticlesFetcher::addTagsToNote");

    if (m_tags.isEmpty()) {
        QNDEBUG(
            "wiki2account::WikiArticlesFetcher", "No tags to assign to note");
        return;
    }

    int lowest = static_cast<int>(m_minTagsPerNote);

    Q_ASSERT(m_tags.size() <= std::numeric_limits<int>::max());
    int highest = static_cast<int>(m_tags.size());

    // Protect from the case in which lowest > highest
    lowest = std::min(lowest, highest);

    int range = (highest - lowest) + 1;
    int randomValue = std::rand();
    int numTags = lowest +
        static_cast<int>(std::floor(
            static_cast<double>(range) * static_cast<double>(randomValue) /
            (RAND_MAX + 1.0)));

    QNTRACE(
        "wiki2account::WikiArticlesFetcher",
        "Adding " << numTags << " tags to note");
    for (int i = 0; i < numTags; ++i) {
        note.mutableTagLocalIds().append(m_tags[i].localId());
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
    QNDEBUG(
        "wiki2account::WikiArticlesFetcher",
        "WikiArticlesFetcher::updateProgress");

    double percentage = 0.0;

    // Fetching random wiki article's contents and converting it to note
    // takes 80% of the total progress - the remaining 20% is for adding the
    // note to the local storage
    for (auto it = m_wikiRandomArticleFetchersWithProgress.constBegin(),
              end = m_wikiRandomArticleFetchersWithProgress.constEnd();
         it != end; ++it)
    {
        percentage += 0.8 * it.value();
    }

    // Add note to local storage requests mean there are 80% fetched notes
    percentage += 0.8 *
        static_cast<double>(m_noteLocalIdsPendingPutNoteToLocalStorage.size());

    // Totally finished notes are those already fetched (with fetcher deleted)
    // and added to the local storage
    percentage += m_finishedFetchersCount;

    // Divide the accumulated number by the number of notes meant to fetch
    percentage /= m_numNotes;

    // Just in case ensure the progress doesn't exceed 1.0
    percentage = std::min(percentage, 1.0);

    QNTRACE("wiki2account::WikiArticlesFetcher", "Progress: " << percentage);
    Q_EMIT progress(percentage);
}

utility::cancelers::ICancelerPtr WikiArticlesFetcher::setupCanceler()
{
    if (!m_canceler) {
        m_canceler = std::make_shared<utility::cancelers::ManualCanceler>();
    }

    return m_canceler;
}

} // namespace quentier
