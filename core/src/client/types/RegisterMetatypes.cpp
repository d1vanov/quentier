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
}

} // namespace qute_note
