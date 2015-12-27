#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__RENAME_RESOURCE_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__RENAME_RESOURCE_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include <qute_note/types/ResourceWrapper.h>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(GenericResourceImageWriter)

class RenameResourceUndoCommand: public INoteEditorUndoCommand
{
public:
    RenameResourceUndoCommand(const ResourceWrapper & resource, const QString & previousResourceName,
                              NoteEditorPrivate & noteEditor, GenericResourceImageWriter * pGenericResourceImageWriter,
                              QUndoCommand * parent = Q_NULLPTR);
    RenameResourceUndoCommand(const ResourceWrapper & resource, const QString & previousResourceName,
                              NoteEditorPrivate & noteEditor, GenericResourceImageWriter * pGenericResourceImageWriter,
                              const QString & text,  QUndoCommand * parent = Q_NULLPTR);
    virtual ~RenameResourceUndoCommand();

    virtual void undoImpl() Q_DECL_OVERRIDE;
    virtual void redoImpl() Q_DECL_OVERRIDE;

private:
    ResourceWrapper                 m_resource;
    QString                         m_previousResourceName;
    QString                         m_newResourceName;
    GenericResourceImageWriter *    m_pGenericResourceImageWriter;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__RENAME_RESOURCE_UNDO_COMMAND_H
