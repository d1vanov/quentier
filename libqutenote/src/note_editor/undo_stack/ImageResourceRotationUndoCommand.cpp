#include "ImageResourceRotationUndoCommand.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/DesktopServices.h>

namespace qute_note {

ImageResourceRotationUndoCommand::ImageResourceRotationUndoCommand(const QByteArray & resourceDataBefore, const QString & resourceHashBefore,
                                                                   const QByteArray & resourceRecognitionDataBefore, const QByteArray & resourceRecognitionDataHashBefore,
                                                                   const ResourceWrapper & resourceAfter, const INoteEditorBackend::Rotation::type rotationDirection,
                                                                   NoteEditorPrivate & noteEditor, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_resourceDataBefore(resourceDataBefore),
    m_resourceHashBefore(resourceHashBefore),
    m_resourceRecognitionDataBefore(resourceRecognitionDataBefore),
    m_resourceRecognitionDataHashBefore(resourceRecognitionDataHashBefore),
    m_resourceAfter(resourceAfter),
    m_rotationDirection(rotationDirection)
{
    setText(QObject::tr("Image resource rotation") + (m_rotationDirection == INoteEditorBackend::Rotation::Clockwise
                                                      ? QObject::tr("clockwise")
                                                      : QObject::tr("counterclockwise")));
}

ImageResourceRotationUndoCommand::ImageResourceRotationUndoCommand(const QByteArray & resourceDataBefore, const QString & resourceHashBefore,
                                                                   const QByteArray & resourceRecognitionDataBefore, const QByteArray & resourceRecognitionDataHashBefore,
                                                                   const ResourceWrapper & resourceAfter, const INoteEditorBackend::Rotation::type rotationDirection,
                                                                   NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_resourceDataBefore(resourceDataBefore),
    m_resourceHashBefore(resourceHashBefore),
    m_resourceRecognitionDataBefore(resourceRecognitionDataBefore),
    m_resourceRecognitionDataHashBefore(resourceRecognitionDataHashBefore),
    m_resourceAfter(resourceAfter),
    m_rotationDirection(rotationDirection)
{}

ImageResourceRotationUndoCommand::~ImageResourceRotationUndoCommand()
{}

void ImageResourceRotationUndoCommand::redoImpl()
{
    QNDEBUG("ImageResourceRotationUndoCommand::redoImpl");

    QString fileStoragePath = m_noteEditorPrivate.imageResourcesStoragePath();
    fileStoragePath += "/" + m_resourceAfter.localGuid();
    fileStoragePath += ".png";

    m_noteEditorPrivate.updateResource(m_resourceAfter.localGuid(), m_resourceHashBefore, m_resourceAfter, fileStoragePath);
}

void ImageResourceRotationUndoCommand::undoImpl()
{
    QNDEBUG("ImageResourceRotationUndoCommand::undoImpl");

    ResourceWrapper resource(m_resourceAfter);
    resource.setDataBody(m_resourceDataBefore);
    resource.setDataSize(m_resourceDataBefore.size());
    resource.setRecognitionDataBody(m_resourceRecognitionDataBefore);
    resource.setRecognitionDataHash(m_resourceRecognitionDataHashBefore);
    if (!m_resourceRecognitionDataBefore.isEmpty()) {
        resource.setRecognitionDataSize(m_resourceRecognitionDataBefore.size());
    }

    QString fileStoragePath = m_noteEditorPrivate.imageResourcesStoragePath();
    fileStoragePath += "/" + resource.localGuid();
    fileStoragePath += ".png";

    m_noteEditorPrivate.updateResource(resource.localGuid(), m_resourceAfter.dataHash(), resource, fileStoragePath);
}

} // namespace qute_note
