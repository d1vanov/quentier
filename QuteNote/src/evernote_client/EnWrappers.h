#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H

#include "../evernote_sdk/src/NoteStore.h"
#include <QString>
#include <QDataStream>
#include <QByteArray>

namespace qute_note {

// TODO: create test for these both operators working right with each other
QDataStream & operator<<(QDataStream & out, const evernote::edam::NoteAttributes & noteAttributes);
QDataStream & operator>>(QDataStream & in, evernote::edam::NoteAttributes & noteAttributes);

const QByteArray GetSerializedNoteAttributes(const evernote::edam::NoteAttributes & noteAttributes);
const evernote::edam::NoteAttributes GetDeserializedNoteAttributes(const QByteArray & data);

struct Note {
    bool isDirty;
    bool isLocal;
    bool isDeleted;
    evernote::edam::Note en_note;

    bool CheckParameters(QString & errorDescription) const;
};

struct Notebook {
    bool isDirty;
    bool isLocal;
    bool isLastUsed;
    evernote::edam::Notebook en_notebook;

    bool CheckParameters(QString & errorDescription) const;
};

struct Resource {
    bool isDirty;
    bool isLocal;
    evernote::edam::Resource en_resource;
};

struct Tag {
    bool isDirty;
    bool isLocal;
    evernote::edam::Tag en_tag;
};

typedef evernote::edam::Timestamp Timestamp;
typedef evernote::edam::UserID UserID;

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H
