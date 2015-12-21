#include "ImageResourceRotationUndoCommand.h"
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/DesktopServices.h>

namespace qute_note {

ImageResourceRotationUndoCommand::ImageResourceRotationUndoCommand(const QByteArray & resourceDataBefore, const QString & resourceHashBefore,
                                                                   const ResourceWrapper & resourceAfter, const QString & fileStoragePathBefore,
                                                                   const QString & fileStoragePathAfter,
                                                                   const INoteEditorBackend::Rotation::type rotationDirection,
                                                                   NoteEditorPrivate & noteEditor, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_resourceDataBefore(resourceDataBefore),
    m_resourceHashBefore(resourceHashBefore),
    m_resourceAfter(resourceAfter),
    m_resourceFileStoragePathBefore(fileStoragePathBefore),
    m_resourceFileStoragePathAfter(fileStoragePathAfter),
    m_rotationDirection(rotationDirection)
{
    setText(QObject::tr("Image resource rotation") + (m_rotationDirection == INoteEditorBackend::Rotation::Clockwise
                                                      ? QObject::tr("clockwise")
                                                      : QObject::tr("counterclockwise")));
}

ImageResourceRotationUndoCommand::ImageResourceRotationUndoCommand(const QByteArray & resourceDataBefore, const QString & resourceHashBefore,
                                                                   const ResourceWrapper & resourceAfter, const QString & fileStoragePathBefore,
                                                                   const QString & fileStoragePathAfter,
                                                                   const INoteEditorBackend::Rotation::type rotationDirection,
                                                                   NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_resourceDataBefore(resourceDataBefore),
    m_resourceHashBefore(resourceHashBefore),
    m_resourceAfter(resourceAfter),
    m_resourceFileStoragePathBefore(fileStoragePathBefore),
    m_resourceFileStoragePathAfter(fileStoragePathAfter),
    m_rotationDirection(rotationDirection)
{}

ImageResourceRotationUndoCommand::~ImageResourceRotationUndoCommand()
{}

void ImageResourceRotationUndoCommand::redoImpl()
{
    QNDEBUG("ImageResourceRotationUndoCommand::redoImpl");

    m_noteEditorPrivate.replaceResourceInNote(m_resourceAfter);

    QString resourceDisplayName = m_resourceAfter.displayName();

    QString resourceDisplaySize;
    if (m_resourceAfter.hasDataSize()) {
        resourceDisplaySize = humanReadableSize(m_resourceAfter.dataSize());
    }

    m_noteEditorPrivate.updateResourceInfo(m_resourceAfter.localGuid(), m_resourceHashBefore, m_resourceAfter.dataHash(),
                                           resourceDisplayName, resourceDisplaySize, m_resourceFileStoragePathAfter);

    QString updateHashJs = "updateResourceHash('" + m_resourceHashBefore + "', '" + m_resourceAfter.dataHash() + "');";
    QString updateSrcJs = "updateImageResourceSrc('" + m_resourceAfter.dataHash() + "', '" + m_resourceFileStoragePathAfter + "');";

    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page());
    if (page) {
        page->executeJavaScript(updateHashJs);
        page->executeJavaScript(updateSrcJs);
    }
}

void ImageResourceRotationUndoCommand::undoImpl()
{
    QNDEBUG("ImageResourceRotationUndoCommand::undoImpl");

    ResourceWrapper resource(m_resourceAfter);
    resource.setDataBody(m_resourceDataBefore);
    resource.setDataHash(m_resourceHashBefore.toLocal8Bit());
    resource.setDataSize(m_resourceDataBefore.size());

    m_noteEditorPrivate.replaceResourceInNote(resource);

    QString resourceDisplayName = resource.displayName();
    QString resourceDisplaySize = humanReadableSize(resource.dataSize());

    m_noteEditorPrivate.updateResourceInfo(resource.localGuid(), m_resourceAfter.dataHash(), m_resourceHashBefore,
                                           resourceDisplayName, resourceDisplaySize, m_resourceFileStoragePathBefore);

    QString updateHashJs = "updateResourceHash('" + m_resourceAfter.dataHash() + "', '" + m_resourceHashBefore + "');";
    QString updateSrcJs = "updateImageResourceSrc('" + m_resourceHashBefore + "', '" + m_resourceFileStoragePathBefore + "');";

    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page());
    if (page) {
        page->executeJavaScript(updateHashJs);
        page->executeJavaScript(updateSrcJs);
    }
}

} // namespace qute_note
