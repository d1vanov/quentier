#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H

#include "INoteStore.h"
#include <QScopedPointer>

namespace evernote {
namespace edam {
QT_FORWARD_DECLARE_CLASS(Note)
}
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Notebook)

QT_FORWARD_DECLARE_CLASS(NoteStorePrivate)
class NoteStore final : public INoteStore
{
public:
    NoteStore(const QString & authenticationToken, const QString & host,
              const int port, const QString & noteStorePath);
    ~NoteStore();

    virtual Guid CreateNoteGuid(const Note & note, QString & errorDescription) override;

    virtual Guid CreateNotebookGuid(const Notebook & notebook, QString & errorDescription) override;

    virtual Guid CreateTagGuid(const Tag & tag, QString & errorDescription);

private:
    NoteStore() = delete;
    NoteStore(const NoteStore & other) = delete;
    NoteStore & operator=(const NoteStore & other) = delete;

    /**
     * @brief CreateNoteFromEdamNote - attempts to create note based on guid and
     * notebook's guid taken from EDAM note object.
     * @param edamNote - EDAM note which should have at least guid and notebook's guid set
     * @return either valid or empty Note object, in the latter case it would contain
     * error explaining why the NoteStore failed to create Note.
     */
    Note CreateNoteFromEdamNote(const evernote::edam::Note & edamNote) const;

    /**
     * @brief ConvertFromEdamNote - attempts to convert the attributes of EDAM note
     * object into Note object. Would check whether EDAM note has set guid and notebook's guid
     * and whether they match the ones of note into which the converted attributes are written
     * @param edamNote - EDAM note from which the attributes are converted
     * @param note - Note into which the attributes are converted; in case of
     * conversion failure the error is set into this object.
     */
    void ConvertFromEdamNote(const evernote::edam::Note & edamNote, Note & note);

    const QScopedPointer<NoteStorePrivate> d_ptr;
    Q_DECLARE_PRIVATE(NoteStore)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H
