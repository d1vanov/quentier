#include "AddHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

AddHyperlinkUndoCommand::AddHyperlinkUndoCommand(const QString & htmlWithHyperlink, NoteEditorPrivate & noteEditor,
                                                 QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_htmlWithHyperlink(htmlWithHyperlink)
{
    setText(QObject::tr("Add hyperlink"));
}

AddHyperlinkUndoCommand::AddHyperlinkUndoCommand(const QString & htmlWithHyperlink, NoteEditorPrivate & noteEditor,
                                                 const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_htmlWithHyperlink(htmlWithHyperlink)
{}

AddHyperlinkUndoCommand::~AddHyperlinkUndoCommand()
{}

void AddHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG("AddHyperlinkUndoCommand::redoImpl");

    m_noteEditorPrivate.switchEditorPage(/* should convert from note = */ false);
    m_noteEditorPrivate.setNoteHtml(m_htmlWithHyperlink);
}

void AddHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG("AddHyperlinkUndoCommand::undoImpl");

    m_noteEditorPrivate.popEditorPage();
}

} // namespace qute_note
