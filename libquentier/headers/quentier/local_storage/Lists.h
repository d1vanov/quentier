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

#ifndef LIB_QUENTIER_LOCAL_STORAGE_LISTS_H
#define LIB_QUENTIER_LOCAL_STORAGE_LISTS_H

#include <QVector>
#include <QSharedPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(User)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(ISharedNotebook)
QT_FORWARD_DECLARE_CLASS(SharedNotebookWrapper)
QT_FORWARD_DECLARE_CLASS(SharedNotebookAdapter)
QT_FORWARD_DECLARE_CLASS(LinkedNotebook)
QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(ResourceWrapper)
QT_FORWARD_DECLARE_CLASS(ResourceAdapter)
QT_FORWARD_DECLARE_CLASS(SavedSearch)

typedef QList<User>                   UserList;
typedef QList<Notebook>               NotebookList;
typedef QList<SharedNotebookWrapper>  SharedNotebookList;
typedef QList<LinkedNotebook>         LinkedNotebookList;
typedef QList<Note>                   NoteList;
typedef QList<Tag>                    TagList;
typedef QList<ResourceWrapper>        ResourceList;
typedef QList<SavedSearch>            SavedSearchList;

}

#endif // LIB_QUENTIER_LOCAL_STORAGE_LISTS_H
