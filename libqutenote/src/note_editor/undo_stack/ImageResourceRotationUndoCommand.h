#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__IMAGE_RESOURCE_ROTATION_UNDO_COMMAND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__IMAGE_RESOURCE_ROTATION_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditor_p.h"

namespace qute_note {

class ImageResourceRotationUndoCommand: public INoteEditorUndoCommand
{
public:
    ImageResourceRotationUndoCommand(const QString & resourceHashBefore, const QString & resourceHashAfter,
                                     const INoteEditorBackend::Rotation::type rotationDirection, NoteEditorPrivate & noteEditor,
                                     QUndoCommand * parent = Q_NULLPTR);
    ImageResourceRotationUndoCommand(const QString & resourceHashBefore, const QString & resourceHashAfter,
                                     const INoteEditorBackend::Rotation::type rotationDirection, NoteEditorPrivate & noteEditor,
                                     const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~ImageResourceRotationUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    const QString                               m_resourceHashBefore;
    const QString                               m_resourceHashAfter;
    const INoteEditorBackend::Rotation::type    m_rotationDirection;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__UNDO_STACK__IMAGE_RESOURCE_ROTATION_UNDO_COMMAND_H
