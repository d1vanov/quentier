#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H

#include <Types_types.h>
#include <NoteStore.h>
#include <QString>
#include <QDataStream>
#include <QByteArray>

namespace qute_note {

struct Note
{
    Note() : isDirty(true), isLocal(true), isDeleted(false), en_note() {}

    bool isDirty;
    bool isLocal;
    bool isDeleted;
    evernote::edam::Note en_note;

    bool CheckParameters(QString & errorDescription) const;
};

struct Notebook
{
    Notebook() : isDirty(true), isLocal(true), isLastUsed(false), en_notebook() {}

    bool isDirty;
    bool isLocal;
    bool isLastUsed;
    evernote::edam::Notebook en_notebook;

    bool CheckParameters(QString & errorDescription) const;
};

struct LinkedNotebook
{
    LinkedNotebook(): isDirty(true), en_linked_notebook() {}

    bool isDirty;
    evernote::edam::LinkedNotebook en_linked_notebook;

    bool CheckParameters(QString & errorDescription) const;
};

struct Tag
{
    Tag() : isDirty(true), isLocal(true), isDeleted(false), en_tag() {}

    bool isDirty;
    bool isLocal;
    bool isDeleted;
    evernote::edam::Tag en_tag;

    bool CheckParameters(QString & errorDescription) const;
};

struct SavedSearch
{
    SavedSearch() : isDirty(true), en_search() {}

    bool isDirty;
    evernote::edam::SavedSearch en_search;

    bool CheckParameters(QString & errorDescription) const;
};

typedef evernote::edam::Timestamp Timestamp;
typedef evernote::edam::UserID UserID;
typedef evernote::edam::Guid Guid;

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__EN_WRAPPERS_H
