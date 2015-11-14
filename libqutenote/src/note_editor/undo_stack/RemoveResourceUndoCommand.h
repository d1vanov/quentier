#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REMOVE_RESOURCE_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REMOVE_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/ResourceWrapper.h>

namespace qute_note {

class RemoveResourceUndoCommand: public INoteEditorUndoCommand
{
public:
    RemoveResourceUndoCommand(const ResourceWrapper & resource, NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent = Q_NULLPTR);
    RemoveResourceUndoCommand(const ResourceWrapper & resource, NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~RemoveResourceUndoCommand();

    void setHtmlBefore(const QString & htmlBefore);
    void setHtmlAfter(const QString & htmlAfter);

    virtual void undo() Q_DECL_OVERRIDE;
    virtual void redo() Q_DECL_OVERRIDE;

private:
    void init();

private:
    ResourceWrapper     m_resource;

    QString             m_htmlBefore;
    QString             m_htmlAfter;

    bool                m_htmlBeforeSet;
    bool                m_htmlAfterSet;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__REMOVE_RESOURCE_UNDO_COMMAND_H
