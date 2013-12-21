#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H

#include "INoteStore.h"
#include <QScopedPointer>

namespace qute_note {

class NoteStorePrivate;
class NoteStore final : public INoteStore
{
public:
    NoteStore(const QString & authenticationToken, const QString & host,
              const int port, const QString & noteStorePath);
    ~NoteStore();

    virtual Note CreateNote(const QString & title, const QString &content, const Notebook & notebook,
                            const std::vector<Tag> & tags = std::vector<Tag>(),
                            const std::vector<ResourceMetadata> & resourcesMetadata = std::vector<ResourceMetadata>());

private:
    NoteStore() = delete;
    NoteStore(const NoteStore & other) = delete;
    NoteStore & operator=(const NoteStore & other) = delete;

    const QScopedPointer<NoteStorePrivate> d_ptr;
    Q_DECLARE_PRIVATE(NoteStore)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H
