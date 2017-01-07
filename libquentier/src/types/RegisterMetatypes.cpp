/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/types/RegisterMetatypes.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <quentier/types/Resource.h>
#include <quentier/types/User.h>
#include <quentier/types/LinkedNotebook.h>
#include <quentier/types/SavedSearch.h>
#include <quentier/types/SharedNotebook.h>
#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QMetaType>

namespace quentier {

void registerMetatypes()
{
    qRegisterMetaType<Notebook>("Notebook");
    qRegisterMetaType<Note>("Note");
    qRegisterMetaType<Tag>("Tag");
    qRegisterMetaType<Resource>("Resource");
    qRegisterMetaType<User>("User");
    qRegisterMetaType<LinkedNotebook>("LinkedNotebook");
    qRegisterMetaType<SavedSearch>("SavedSearch");

    qRegisterMetaType< QList<Notebook> >("QList<Notebook>");
    qRegisterMetaType< QList<Note> >("QList<Note>");
    qRegisterMetaType< QList<Tag> >("QList<Tag>");
    qRegisterMetaType< QList<Resource> >("QList<Resource>");
    qRegisterMetaType< QList<User> >("QList<User>");
    qRegisterMetaType< QList<LinkedNotebook> >("QList<LinkedNotebook>");
    qRegisterMetaType< QList<SavedSearch> >("QList<SavedSearch>");

    qRegisterMetaType< QList<SharedNotebook> >("QList<SharedNotebook>");

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

    qRegisterMetaType<NoteSearchQuery>("NoteSearchQuery");

    qRegisterMetaType<QNLocalizedString>("QNLocalizedString");
}

} // namespace quentier
