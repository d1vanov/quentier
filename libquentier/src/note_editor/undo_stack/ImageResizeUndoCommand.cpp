#include "ImageResizeUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't undo/redo image resizing: can't get note editor page"); \
        QNWARNING(error); \
        return; \
    }

ImageResizeUndoCommand::ImageResizeUndoCommand(NoteEditorPrivate & noteEditor, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent)
{}

ImageResizeUndoCommand::ImageResizeUndoCommand(NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent)
{}

ImageResizeUndoCommand::~ImageResizeUndoCommand()
{}

void ImageResizeUndoCommand::redoImpl()
{
    QNDEBUG("ImageResizeUndoCommand::redoImpl");

    GET_PAGE()
    page->executeJavaScript("resizableImageManager.redo();");
}

void ImageResizeUndoCommand::undoImpl()
{
    QNDEBUG("ImageResizeUndoCommand::undoImpl");

    GET_PAGE()
    page->executeJavaScript("resizableImageManager.undo();");
}

} // namespace quentier
