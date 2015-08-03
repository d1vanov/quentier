#ifndef __LIB_QUTE_NOTE__SYNCHRONIZATION__NOTE_STORE_H
#define __LIB_QUTE_NOTE__SYNCHRONIZATION__NOTE_STORE_H

#include <QEverCloud.h>
#include <QSharedPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(SavedSearch)

/**
 * @brief The NoteStore class in qute_note namespace is a wrapper under NoteStore from QEverCloud.
 * The main difference from the underlying class is stronger exception safety: most QEverCloud's methods
 * throw exceptions to indicate errors (much like the native Evercloud API for supported languages do).
 * Using exceptions along with Qt is not simple and desireable. Therefore, this class' methods
 * simply redirect the requests to methods of QEverCloud's NoteStore but catch the "expected" exceptions,
 * "parse" their internal error flags and return the textual representation of the error.
 *
 * QuteNote at the moment uses only several methods from those available in QEverCloud's NoteStore
 * so only the small subset of original NoteStore's API is wrapped here at the moment.
 */
class NoteStore
{
public:
    NoteStore(QSharedPointer<qevercloud::NoteStore> pQecNoteStore);

    QSharedPointer<qevercloud::NoteStore> getQecNoteStore();

    void setNoteStoreUrl(const QString & noteStoreUrl);
    void setAuthenticationToken(const QString & authToken);

    qint32 createNotebook(Notebook & notebook, QString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());
    qint32 updateNotebook(Notebook & notebook, QString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());

    qint32 createNote(Note & note, QString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());
    qint32 updateNote(Note & note, QString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());

    qint32 createTag(Tag & tag, QString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());
    qint32 updateTag(Tag & tag, QString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken = QString());

    qint32 createSavedSearch(SavedSearch & savedSearch, QString & errorDescription, qint32 & rateLimitSeconds);
    qint32 updateSavedSearch(SavedSearch & savedSearch, QString & errorDescription, qint32 & rateLimitSeconds);

    qint32 getSyncState(qevercloud::SyncState & syncState, QString & errorDescription, qint32 & rateLimitSeconds);

    qint32 getSyncChunk(const qint32 afterUSN, const qint32 maxEntries,
                        const qevercloud::SyncChunkFilter & filter,
                        qevercloud::SyncChunk & syncChunk, QString & errorDescription,
                        qint32 & rateLimitSeconds);

    qint32 getLinkedNotebookSyncState(const qevercloud::LinkedNotebook & linkedNotebook,
                                      const QString & authToken, qevercloud::SyncState & syncState,
                                      QString & errorDescription, qint32 & rateLimitSeconds);

    qint32 getLinkedNotebookSyncChunk(const qevercloud::LinkedNotebook & linkedNotebook,
                                      const qint32 afterUSN, const qint32 maxEntries,
                                      const QString & linkedNotebookAuthToken, const bool fullSyncOnly,
                                      qevercloud::SyncChunk & syncChunk, QString & errorDescription,
                                      qint32 & rateLimitSeconds);

    qint32 getNote(const bool withContent, const bool withResourcesData,
                   const bool withResourcesRecognition, const bool withResourceAlternateData,
                   Note & note, QString & errorDescription, qint32 & rateLimitSeconds);

    qint32 authenticateToSharedNotebook(const QString & shareKey, qevercloud::AuthenticationResult & authResult,
                                        QString & errorDescription, qint32 & rateLimitSeconds);
private:

    struct UserExceptionSource
    {
        enum type {
            Creation = 0,
            Update
        };
    };

    qint32 processEdamUserExceptionForNotebook(const Notebook & notebook, const qevercloud::EDAMUserException & userException,
                                               const UserExceptionSource::type & source, QString & errorDescription) const;

    qint32 processEdamUserExceptionForNote(const Note & note, const qevercloud::EDAMUserException & userException,
                                           const UserExceptionSource::type & source, QString & errorDescription) const;

    qint32 processEdamUserExceptionForTag(const Tag & tag, const qevercloud::EDAMUserException & userException,
                                          const UserExceptionSource::type & source, QString & errorDescription) const;

    qint32 processEdamUserExceptionForSavedSearch(const SavedSearch & search, const qevercloud::EDAMUserException & userException,
                                                  const UserExceptionSource::type & source, QString & errorDescription) const;

    qint32 processEdamUserExceptionForGetSyncChunk(const qevercloud::EDAMUserException & userException,
                                                   const qint32 afterUSN, const qint32 maxEntries,
                                                   QString & errorDescription) const;

    qint32 processEdamUserExceptionForGetNote(const Note & note, const qevercloud::EDAMUserException & userException,
                                              QString & errorDescription) const;

    qint32 processUnexpectedEdamUserException(const QString & typeName, const qevercloud::EDAMUserException & userException,
                                              const UserExceptionSource::type & source, QString & errorDescription) const;

    qint32 processEdamSystemException(const qevercloud::EDAMSystemException & systemException,
                                      QString & errorDescription, qint32 & rateLimitSeconds) const;

    void processEdamNotFoundException(const qevercloud::EDAMNotFoundException & notFoundException,
                                      QString & errorDescription) const;

private:
    NoteStore() Q_DECL_DELETE;
    NoteStore(const NoteStore & other) Q_DECL_DELETE;
    NoteStore & operator=(const NoteStore & other) Q_DECL_DELETE;

private:
    QSharedPointer<qevercloud::NoteStore> m_pQecNoteStore;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__SYNCHRONIZATION__NOTE_STORE_H
