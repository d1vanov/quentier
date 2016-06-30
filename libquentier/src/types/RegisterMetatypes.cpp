#include <quentier/types/RegisterMetatypes.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <quentier/types/ResourceWrapper.h>
#include <quentier/types/UserWrapper.h>
#include <quentier/types/LinkedNotebook.h>
#include <quentier/types/SavedSearch.h>
#include <quentier/types/SharedNotebookWrapper.h>
#include <quentier/local_storage/LocalStorageManager.h>
#include <QMetaType>

namespace quentier {

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

} // namespace quentier
