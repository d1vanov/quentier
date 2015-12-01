#include "RemoveHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

RemoveHyperlinkUndoCommand::RemoveHyperlinkUndoCommand(const QString & htmlWithoutHyperlink, NoteEditorPrivate & noteEditor,
                                                       QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_htmlWithoutHyperlink(htmlWithoutHyperlink)
{
    setText(QObject::tr("Remove hyperlink"));
}

RemoveHyperlinkUndoCommand::RemoveHyperlinkUndoCommand(const QString & htmlWithoutHyperlink, NoteEditorPrivate & noteEditor,
                                                       const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_htmlWithoutHyperlink(htmlWithoutHyperlink)
{}

RemoveHyperlinkUndoCommand::~RemoveHyperlinkUndoCommand()
{}

void RemoveHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG("RemoveHyperlinkUndoCommand::redoImpl");

    m_noteEditorPrivate.switchEditorPage(/* should convert from note = */ false);
    m_noteEditorPrivate.setNoteHtml(m_htmlWithoutHyperlink);
}

void RemoveHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG("RemoveHyperlinkUndoCommand::undoImpl");

    m_noteEditorPrivate.popEditorPage();
}

} // namespace qute_note
