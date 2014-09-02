#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LISTS_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LISTS_H

#include <QVector>
#include <QSharedPointer>

namespace qute_note {

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

typedef QVector<QSharedPointer<UserWrapper> >           UserList;
typedef QVector<QSharedPointer<Notebook> >              NotebookList;
typedef QVector<QSharedPointer<SharedNotebookWrapper> > SharedNotebookList;
typedef QVector<QSharedPointer<LinkedNotebook> >        LinkedNotebookList;
typedef QVector<QSharedPointer<Note> >                  NoteList;
typedef QVector<QSharedPointer<Tag> >                   TagList;
typedef QVector<QSharedPointer<ResourceWrapper> >       ResourceList;
typedef QVector<QSharedPointer<SavedSearch> >           SavedSearchList;

}

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__LISTS_H
