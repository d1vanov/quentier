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

#ifndef LIB_QUENTIER_SYNCHRONIZATION_NOTE_STORE_H
#define LIB_QUENTIER_SYNCHRONIZATION_NOTE_STORE_H

#include <quentier/utility/QNLocalizedString.h>
#include <QSharedPointer>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(SavedSearch)

/**
 * @brief The NoteStore class in quentier namespace is a wrapper under NoteStore from QEverCloud.
 * The main difference from the underlying class is stronger exception safety: most QEverCloud's methods
 * throw exceptions to indicate errors (much like the native Evercloud API for supported languages do).
 * Using exceptions along with Qt is not simple and desireable. Therefore, this class' methods
 * simply redirect the requests to methods of QEverCloud's NoteStore but catch the "expected" exceptions,
 * "parse" their internal error flags and return the textual representation of the error.
 *
 * Quentier at the moment uses only several methods from those available in QEverCloud's NoteStore
 * so only the small subset of original NoteStore's API is wrapped here at the moment.
 */
class NoteStore
{
public:
    NoteStore(QSharedPointer<qevercloud::NoteStore> pQecNoteStore);

    QSharedPointer<qevercloud::NoteStore> getQecNoteStore();

    void setNoteStoreUrl(const QString & noteStoreUrl);
    void setAuthenticationToken(const QString & authToken);

    qint32 createNotebook(Notebook & notebook, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());
    qint32 updateNotebook(Notebook & notebook, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());

    qint32 createNote(Note & note, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());
    qint32 updateNote(Note & note, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());

    qint32 createTag(Tag & tag, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());
    qint32 updateTag(Tag & tag, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());

    qint32 createSavedSearch(SavedSearch & savedSearch, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds);
    qint32 updateSavedSearch(SavedSearch & savedSearch, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds);

    qint32 getSyncState(qevercloud::SyncState & syncState, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds);

    qint32 getSyncChunk(const qint32 afterUSN, const qint32 maxEntries,
                        const qevercloud::SyncChunkFilter & filter,
                        qevercloud::SyncChunk & syncChunk, QNLocalizedString & errorDescription,
                        qint32 & rateLimitSeconds);

    qint32 getLinkedNotebookSyncState(const qevercloud::LinkedNotebook & linkedNotebook,
                                      const QString & authToken, qevercloud::SyncState & syncState,
                                      QNLocalizedString & errorDescription, qint32 & rateLimitSeconds);

    qint32 getLinkedNotebookSyncChunk(const qevercloud::LinkedNotebook & linkedNotebook,
                                      const qint32 afterUSN, const qint32 maxEntries,
                                      const QString & linkedNotebookAuthToken, const bool fullSyncOnly,
                                      qevercloud::SyncChunk & syncChunk, QNLocalizedString & errorDescription,
                                      qint32 & rateLimitSeconds);

    qint32 getNote(const bool withContent, const bool withResourcesData,
                   const bool withResourcesRecognition, const bool withResourceAlternateData,
                   Note & note, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds);

    qint32 authenticateToSharedNotebook(const QString & shareKey, qevercloud::AuthenticationResult & authResult,
                                        QNLocalizedString & errorDescription, qint32 & rateLimitSeconds);
private:

    struct UserExceptionSource
    {
        enum type {
            Creation = 0,
            Update
        };
    };

    qint32 processEdamUserExceptionForNotebook(const Notebook & notebook, const qevercloud::EDAMUserException & userException,
                                               const UserExceptionSource::type & source, QNLocalizedString & errorDescription) const;

    qint32 processEdamUserExceptionForNote(const Note & note, const qevercloud::EDAMUserException & userException,
                                           const UserExceptionSource::type & source, QNLocalizedString & errorDescription) const;

    qint32 processEdamUserExceptionForTag(const Tag & tag, const qevercloud::EDAMUserException & userException,
                                          const UserExceptionSource::type & source, QNLocalizedString & errorDescription) const;

    qint32 processEdamUserExceptionForSavedSearch(const SavedSearch & search, const qevercloud::EDAMUserException & userException,
                                                  const UserExceptionSource::type & source, QNLocalizedString & errorDescription) const;

    qint32 processEdamUserExceptionForGetSyncChunk(const qevercloud::EDAMUserException & userException,
                                                   const qint32 afterUSN, const qint32 maxEntries,
                                                   QNLocalizedString & errorDescription) const;

    qint32 processEdamUserExceptionForGetNote(const Note & note, const qevercloud::EDAMUserException & userException,
                                              QNLocalizedString & errorDescription) const;

    qint32 processUnexpectedEdamUserException(const QString & typeName, const qevercloud::EDAMUserException & userException,
                                              const UserExceptionSource::type & source, QNLocalizedString & errorDescription) const;

    qint32 processEdamSystemException(const qevercloud::EDAMSystemException & systemException,
                                      QNLocalizedString & errorDescription, qint32 & rateLimitSeconds) const;

    void processEdamNotFoundException(const qevercloud::EDAMNotFoundException & notFoundException,
                                      QNLocalizedString & errorDescription) const;

private:
    NoteStore() Q_DECL_EQ_DELETE;
    NoteStore(const NoteStore & other) Q_DECL_EQ_DELETE;
    NoteStore & operator=(const NoteStore & other) Q_DECL_EQ_DELETE;

private:
    QSharedPointer<qevercloud::NoteStore> m_pQecNoteStore;
};

} // namespace quentier

#endif // LIB_QUENTIER_SYNCHRONIZATION_NOTE_STORE_H
