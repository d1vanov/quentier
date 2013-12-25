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

    bool ConvertFromEdamNote(const evernote::edam::Note & edamNote, Note & note,
                             QString & error);

    const QScopedPointer<NoteStorePrivate> d_ptr;
    Q_DECLARE_PRIVATE(NoteStore)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H
