#include "EditHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't undo/redo hyperlink edit: can't get note editor page"); \
        QNWARNING(error); \
        return; \
    }

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_callback(callback)
{
    setText(QObject::tr("Edit hyperlink"));
}

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(NoteEditorPrivate & noteEditorPrivate, const Callback & callback, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_callback(callback)
{}

EditHyperlinkUndoCommand::~EditHyperlinkUndoCommand()
{}

void EditHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG("EditHyperlinkUndoCommand::redoImpl");

    GET_PAGE()
    page->executeJavaScript("hyperlinkManager.redo();", m_callback);
}

void EditHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG("EditHyperlinkUndoCommand::undoImpl");

    GET_PAGE()
    page->executeJavaScript("hyperlinkManager.undo();", m_callback);
}

} // namespace qute_note
