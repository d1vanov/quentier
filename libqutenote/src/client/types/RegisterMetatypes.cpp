#include "RegisterMetatypes.h"
#include "Notebook.h"
#include "Note.h"
#include "Tag.h"
#include "ResourceWrapper.h"
#include "UserWrapper.h"
#include "LinkedNotebook.h"
#include "SavedSearch.h"
#include "SharedNotebookWrapper.h"
#include <qute_note/local_storage/LocalStorageManager.h>
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

    qRegisterMetaType< QList<SharedNotebookWrapper> >("QList<SharedNotebookWrapper>");

    qRegisterMetaType<LocalStorageManager::ListObjectsOptions>("LocalStorageManager::ListObjectsOptions");
    qRegisterMetaType<LocalStorageManager::ListNotesOrder::type>("LocalStorageManager::ListNotesOrder::type");
    qRegisterMetaType<LocalStorageManager::ListNotebooksOrder::type>("LocalStorageManager::ListNotebooksOrder::type");
    qRegisterMetaType<LocalStorageManager::ListLinkedNotebooksOrder::type>("LocalStorageManager::ListLinkedNotebooksOrder::type");
    qRegisterMetaType<LocalStorageManager::ListTagsOrder::type>("LocalStorageManager::ListTagsOrder::type");
    qRegisterMetaType<LocalStorageManager::ListSavedSearchesOrder::type>("LocalStorageManager::ListSavedSearchesOrder::type");
    qRegisterMetaType<LocalStorageManager::OrderDirection::type>("LocalStorageManager::OrderDirection::type");
    qRegisterMetaType<size_t>("size_t");

    qRegisterMetaType<QUuid>("QUuid");

    qRegisterMetaType<QList<QPair<QString,QString> > >("QList<QPair<QString,QString> >");
    qRegisterMetaType<QHash<QString,QString> >("QHash<QString,QString>");
    qRegisterMetaType<QHash<QString,qevercloud::Timestamp> >("QHash<QString,qevercloud::Timestamp>");
    qRegisterMetaType<QHash<QString,qint32> >("QHash<QString,qint32>");
}

} // namespace qute_note
