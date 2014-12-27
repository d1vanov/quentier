#ifndef __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__NOTE_STORE_H
#define __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__NOTE_STORE_H

#include <QEverCloud.h>

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
 * so only the small subset of original NoteStore's API is wrapped now.
 */
class NoteStore
{
public:
    NoteStore();

    void setNoteStoreUrl(const QString & noteStoreUrl);
    void setAuthenticationToken(const QString & authToken);

    bool createNotebook(Notebook & notebook, QString & errorDescription);
    bool updateNotebook(Notebook & notebook, QString & errorDescription);

    bool createNote(Note & note, QString & errorDescription);
    bool updateNote(Note & note, QString & errorDescription);

    bool createTag(Tag & tag, QString & errorDescription);
    bool updateTag(Tag & tag, QString & errorDescription);

    bool createSavedSearch(SavedSearch & savedSearch, QString & errorDescription);
    bool updateSavedSearch(SavedSearch & savedSearch, QString & errorDescription);

    bool getSyncChunk(const qint32 afterUSN, const qint32 maxEntries,
                      const qevercloud::SyncChunkFilter & filter,
                      qevercloud::SyncChunk & syncChunk);

private:
    NoteStore(const NoteStore & other) Q_DECL_DELETE;
    NoteStore & operator=(const NoteStore & other) Q_DECL_DELETE;

private:
    qevercloud::NoteStore   m_qecNoteStore;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__SYNCHRONIZATION__NOTE_STORE_H
