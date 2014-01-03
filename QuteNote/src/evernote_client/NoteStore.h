#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H

#include "../evernote_sdk/src/NoteStore.h"

namespace qute_note {

struct Note {
    bool isDirty;
    bool isLocal;
    bool isDeleted;
    evernote::edam::Note en_note;
};

struct Notebook {
    bool isDirty;
    bool isLocal;
    bool isLastUsed;
    bool isDeleted;
    evernote::edam::Notebook en_notebook;
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

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_STORE_H
