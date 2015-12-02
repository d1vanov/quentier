#include "RemoveHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

RemoveHyperlinkUndoCommand::RemoveHyperlinkUndoCommand(const quint64 removedHyperlinkId, NoteEditorPrivate & noteEditor,
                                                       QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_hyperlinkId(removedHyperlinkId)
{
    setText(QObject::tr("Remove hyperlink"));
}

RemoveHyperlinkUndoCommand::RemoveHyperlinkUndoCommand(const quint64 removedHyperlinkId, NoteEditorPrivate & noteEditor,
                                                       const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_hyperlinkId(removedHyperlinkId)
{}

RemoveHyperlinkUndoCommand::~RemoveHyperlinkUndoCommand()
{}

void RemoveHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG("RemoveHyperlinkUndoCommand::redoImpl");

    m_noteEditorPrivate.doRemoveHyperlink(/* should track delegate = */ false, m_hyperlinkId);
}

void RemoveHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG("RemoveHyperlinkUndoCommand::undoImpl");

    m_noteEditorPrivate.popEditorPage();
}

} // namespace qute_note
