/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_TESTS_LOCAL_STORAGE_CACHE_ASYNC_TESTER_H
#define LIB_QUENTIER_TESTS_LOCAL_STORAGE_CACHE_ASYNC_TESTER_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <quentier/types/LinkedNotebook.h>
#include <quentier/types/SavedSearch.h>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(LocalStorageManagerThreadWorker)
QT_FORWARD_DECLARE_CLASS(LocalStorageCacheManager)

namespace test {

class LocalStorageCacheAsyncTester: public QObject
{
    Q_OBJECT
public:
    explicit LocalStorageCacheAsyncTester(QObject * parent = Q_NULLPTR);
    virtual ~LocalStorageCacheAsyncTester();

public Q_SLOTS:
    void onInitTestCase();

Q_SIGNALS:
    void success();
    void failure(QString errorDescription);

// private signals
    void addNotebookRequest(Notebook notebook, QUuid requestId = QUuid());
    void updateNotebookRequest(Notebook notebook, QUuid requestId = QUuid());

    void addNoteRequest(Note note, QUuid requestId = QUuid());
    void updateNoteRequest(Note note, bool updateResources, bool updateTags, QUuid requestId = QUuid());

    void addTagRequest(Tag tag, QUuid requestId = QUuid());
    void updateTagRequest(Tag tag, QUuid requestId = QUuid());

    void addLinkedNotebookRequest(LinkedNotebook linkedNotebook, QUuid requestId = QUuid());
    void updateLinkedNotebookRequest(LinkedNotebook linkedNotebook, QUuid requestId = QUuid());

    void addSavedSearchRequest(SavedSearch search, QUuid requestId = QUuid());
    void updateSavedSearchRequest(SavedSearch search, QUuid requestId = QUuid());

private Q_SLOTS:
    void onWorkerInitialized();

    void onAddNotebookCompleted(Notebook notebook, QUuid requestId);
    void onAddNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId);

    void onUpdateNotebookCompleted(Notebook notebook, QUuid requestId);
    void onUpdateNotebookFailed(Notebook notebook, QNLocalizedString errorDescription, QUuid requestId);

    void onAddNoteCompleted(Note note, QUuid requestId);
    void onAddNoteFailed(Note note, QNLocalizedString errorDescription, QUuid requestId);

    void onUpdateNoteCompleted(Note note, bool updateResources, bool updateTags, QUuid requestId);
    void onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                            QNLocalizedString errorDescription, QUuid requestId);

    void onAddTagCompleted(Tag tag, QUuid requestId);
    void onAddTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);

    void onUpdateTagCompleted(Tag tag, QUuid requestId);
    void onUpdateTagFailed(Tag tag, QNLocalizedString errorDescription, QUuid requestId);

    void onAddLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId);
    void onAddLinkedNotebookFailed(LinkedNotebook linkedNotebook, QNLocalizedString errorDescription, QUuid requestId);

    void onUpdateLinkedNotebookCompleted(LinkedNotebook linkedNotebook, QUuid requestId);
    void onUpdateLinkedNotebookFailed(LinkedNotebook linkedNotebook, QNLocalizedString errorDescription, QUuid requestId);

    void onAddSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onAddSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);

    void onUpdateSavedSearchCompleted(SavedSearch search, QUuid requestId);
    void onUpdateSavedSearchFailed(SavedSearch search, QNLocalizedString errorDescription, QUuid requestId);

    void onFailure(QNLocalizedString errorDescription);

private:
    void createConnections();

    void addNotebook();
    void updateNotebook();

    void addNote();
    void updateNote();

    void addTag();
    void updateTag();

    void addLinkedNotebook();
    void updateLinkedNotebook();

    void addSavedSearch();
    void updateSavedSearch();

    enum State
    {
        STATE_UNINITIALIZED,
        STATE_SENT_NOTEBOOK_ADD_REQUEST,
        STATE_SENT_NOTEBOOK_UPDATE_REQUEST,
        STATE_SENT_NOTE_ADD_REQUEST,
        STATE_SENT_NOTE_UPDATE_REQUEST,
        STATE_SENT_TAG_ADD_REQUEST,
        STATE_SENT_TAG_UPDATE_REQUEST,
        STATE_SENT_LINKED_NOTEBOOK_ADD_REQUEST,
        STATE_SENT_LINKED_NOTEBOOK_UPDATE_REQUEST,
        STATE_SENT_SAVED_SEARCH_ADD_REQUEEST,
        STATE_SENT_SAVED_SEARCH_UPDATE_REQUEST
    };

    State   m_state;
    LocalStorageManagerThreadWorker *   m_pLocalStorageManagerThreadWorker;
    const LocalStorageCacheManager *    m_pLocalStorageCacheManager;
    QThread *                           m_pLocalStorageManagerThread;

    Notebook    m_firstNotebook;
    Notebook    m_secondNotebook;
    Notebook    m_currentNotebook;
    size_t      m_addedNotebooksCount;

    Note        m_firstNote;
    Note        m_secondNote;
    Note        m_currentNote;
    size_t      m_addedNotesCount;

    Tag         m_firstTag;
    Tag         m_secondTag;
    Tag         m_currentTag;
    size_t      m_addedTagsCount;

    LinkedNotebook      m_firstLinkedNotebook;
    LinkedNotebook      m_secondLinkedNotebook;
    LinkedNotebook      m_currentLinkedNotebook;
    size_t              m_addedLinkedNotebooksCount;

    SavedSearch         m_firstSavedSearch;
    SavedSearch         m_secondSavedSearch;
    SavedSearch         m_currentSavedSearch;
    size_t              m_addedSavedSearchesCount;
};

} // namespace test
} // namespace quentier

#endif // LIB_QUENTIER_TESTS_LOCAL_STORAGE_CACHE_ASYNC_TESTER_H
