#include "ImageResourceRotationUndoCommand.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ImageResourceRotationUndoCommand::ImageResourceRotationUndoCommand(const QString & resourceHashBefore, const QString & resourceHashAfter,
                                                                   const INoteEditorBackend::Rotation::type rotationDirection,
                                                                   NoteEditorPrivate & noteEditor, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_resourceHashBefore(resourceHashBefore),
    m_resourceHashAfter(resourceHashAfter),
    m_rotationDirection(rotationDirection)
{
    setText(QObject::tr("Image resource rotation") + (rotationDirection == INoteEditorBackend::Rotation::Clockwise
                                                      ? QObject::tr("clockwise")
                                                      : QObject::tr("counterclockwise")));
}

ImageResourceRotationUndoCommand::ImageResourceRotationUndoCommand(const QString & resourceHashBefore, const QString & resourceHashAfter,
                                                                   const INoteEditorBackend::Rotation::type rotationDirection,
                                                                   NoteEditorPrivate & noteEditor, const QString & text,
                                                                   QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_resourceHashBefore(resourceHashBefore),
    m_resourceHashAfter(resourceHashAfter),
    m_rotationDirection(rotationDirection)
{}

ImageResourceRotationUndoCommand::~ImageResourceRotationUndoCommand()
{}

void ImageResourceRotationUndoCommand::redoImpl()
{
    QNDEBUG("ImageResourceRotationUndoCommand::redoImpl");

    QByteArray dummy;
    Q_UNUSED(m_noteEditorPrivate.doRotateImageAttachment(m_resourceHashBefore, m_rotationDirection, dummy));
}

void ImageResourceRotationUndoCommand::undoImpl()
{
    QNDEBUG("ImageResourceRotationUndoCommand::undoImpl");

    QByteArray dummy;
    Q_UNUSED(m_noteEditorPrivate.doRotateImageAttachment(m_resourceHashAfter, (m_rotationDirection == INoteEditorBackend::Rotation::Clockwise
                                                                               ? INoteEditorBackend::Rotation::Counterclockwise
                                                                               : INoteEditorBackend::Rotation::Clockwise), dummy));
}

} // namespace qute_note
