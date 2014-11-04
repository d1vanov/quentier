#include "RegisterMetatypes.h"
#include "Notebook.h"
#include "Note.h"
#include "Tag.h"
#include "ResourceWrapper.h"
#include "UserWrapper.h"
#include "LinkedNotebook.h"
#include "SavedSearch.h"
#include <QMetaType>

namespace qute_note {

void registerMetatypes()
{
    qRegisterMetaType<Notebook>("Notebook");
    qRegisterMetaType<Note>("Note");
    qRegisterMetaType<Tag>("Tag");
    qRegisterMetaType<ResourceWrapper>("ResourceWrapper");
    qRegisterMetaType<UserWrapper>("UserWrapper");
    qRegisterMetaType<LinkedNotebook>("LinkedNotebook");
    qRegisterMetaType<SavedSearch>("SavedSearch");

    qRegisterMetaType< QList<Notebook> >("QList<Notebook>");
    qRegisterMetaType< QList<Note> >("QList<Note>");
    qRegisterMetaType< QList<Tag> >("QList<Tag>");
    qRegisterMetaType< QList<ResourceWrapper> >("QList<ResourceWrapper>");
    qRegisterMetaType< QList<UserWrapper> >("QList<UserWrapper>");
    qRegisterMetaType< QList<LinkedNotebook> >("QList<LinkedNotebook>");
    qRegisterMetaType< QList<SavedSearch> >("QList<SavedSearch>");
}

} // namespace qute_note
