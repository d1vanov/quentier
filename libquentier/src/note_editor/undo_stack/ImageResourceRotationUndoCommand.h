#ifndef LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_IMAGE_RESOURCE_ROTATION_UNDO_COMMAND_H
#define LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_IMAGE_RESOURCE_ROTATION_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/types/ResourceWrapper.h>

namespace quentier {

class ImageResourceRotationUndoCommand: public INoteEditorUndoCommand
{
public:
    ImageResourceRotationUndoCommand(const QByteArray & resourceDataBefore, const QByteArray & resourceHashBefore,
                                     const QByteArray & resourceRecognitionDataBefore, const QByteArray & resourceRecognitionDataHashBefore,
                                     const ResourceWrapper & resourceAfter, const INoteEditorBackend::Rotation::type rotationDirection,
                                     NoteEditorPrivate & noteEditor, QUndoCommand * parent = Q_NULLPTR);
    ImageResourceRotationUndoCommand(const QByteArray & resourceDataBefore, const QByteArray & resourceHashBefore,
                                     const QByteArray & resourceRecognitionDataBefore, const QByteArray & resourceRecognitionDataHashBefore,
                                     const ResourceWrapper & resourceAfter, const INoteEditorBackend::Rotation::type rotationDirection,
                                     NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~ImageResourceRotationUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    const QByteArray                            m_resourceDataBefore;
    const QByteArray                            m_resourceHashBefore;
    const QByteArray                            m_resourceRecognitionDataBefore;
    const QByteArray                            m_resourceRecognitionDataHashBefore;
    const ResourceWrapper                       m_resourceAfter;
    const INoteEditorBackend::Rotation::type    m_rotationDirection;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_UNDO_STACK_IMAGE_RESOURCE_ROTATION_UNDO_COMMAND_H
