#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REMOVE_RESOURCE_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REMOVE_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/ResourceWrapper.h>

namespace qute_note {

class RemoveResourceUndoCommand: public INoteEditorUndoCommand
{
public:
    RemoveResourceUndoCommand(const ResourceWrapper & resource, const QString & htmlWithRemovedResource,
                              const int pageXOffset, const int pageYOffset, NoteEditorPrivate & noteEditorPrivate,
                              QUndoCommand * parent = Q_NULLPTR);
    RemoveResourceUndoCommand(const ResourceWrapper & resource, const QString & htmlWithRemovedResource,
                              const int pageXOffset, const int pageYOffset, NoteEditorPrivate & noteEditorPrivate,
                              const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~RemoveResourceUndoCommand();

    virtual void undoImpl() Q_DECL_OVERRIDE;
    virtual void redoImpl() Q_DECL_OVERRIDE;

private:
    ResourceWrapper     m_resource;
    QString             m_htmlWithRemovedResource;
    int                 m_pageXOffset;
    int                 m_pageYOffset;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REMOVE_RESOURCE_UNDO_COMMAND_H
