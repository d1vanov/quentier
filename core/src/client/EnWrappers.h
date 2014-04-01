#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H

#include <Types_types.h>
#include <NoteStore.h>
#include <tools/Printable.h>
#include <QString>
#include <QDataStream>
#include <QByteArray>

namespace qute_note {

class Notebook
{
public:
    Notebook() : isDirty(true), isLocal(true), isLastUsed(false), en_notebook() {}

    bool isDirty;
    bool isLocal;
    bool isLastUsed;
    evernote::edam::Notebook en_notebook;

    bool CheckParameters(QString & errorDescription) const;
};

typedef evernote::edam::Timestamp Timestamp;
typedef evernote::edam::UserID UserID;
typedef evernote::edam::Guid Guid;

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H
