#ifndef LIB_QUENTIER_LOCAL_STORAGE_LISTS_H
#define LIB_QUENTIER_LOCAL_STORAGE_LISTS_H

#include <QVector>
#include <QSharedPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(IUser)
QT_FORWARD_DECLARE_CLASS(UserAdapter)
QT_FORWARD_DECLARE_CLASS(UserWrapper)
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

typedef QList<UserWrapper>            UserList;
typedef QList<Notebook>               NotebookList;
typedef QList<SharedNotebookWrapper>  SharedNotebookList;
typedef QList<LinkedNotebook>         LinkedNotebookList;
typedef QList<Note>                   NoteList;
typedef QList<Tag>                    TagList;
typedef QList<ResourceWrapper>        ResourceList;
typedef QList<SavedSearch>            SavedSearchList;

}

#endif // LIB_QUENTIER_LOCAL_STORAGE_LISTS_H
