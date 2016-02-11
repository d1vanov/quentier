#include "AddHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't undo/redo adding the hyperlink to the selected text: can't get note editor page"); \
        QNWARNING(error); \
        return; \
    }

AddHyperlinkUndoCommand::AddHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_callback(callback)
{
    setText(QObject::tr("Add hyperlink"));
}

AddHyperlinkUndoCommand::AddHyperlinkUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_callback(callback)
{}

AddHyperlinkUndoCommand::~AddHyperlinkUndoCommand()
{}

void AddHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG("AddHyperlinkUndoCommand::redoImpl");

    GET_PAGE()
    page->executeJavaScript("hyperlinkManager.redo();", m_callback);
}

void AddHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG("AddHyperlinkUndoCommand::undoImpl");

    GET_PAGE()
    page->executeJavaScript("hyperlinkManager.undo();", m_callback);
}

} // namespace qute_note
