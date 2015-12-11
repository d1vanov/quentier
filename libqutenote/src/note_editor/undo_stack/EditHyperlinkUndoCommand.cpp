#include "EditHyperlinkUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(const quint64 hyperlinkId, const QString & linkBefore, const QString & textBefore,
                                                   const QString & linkAfter, const QString & textAfter, NoteEditorPrivate & noteEditorPrivate,
                                                   QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_hyperlinkId(hyperlinkId),
    m_linkBefore(linkBefore),
    m_textBefore(textBefore),
    m_linkAfter(linkAfter),
    m_textAfter(textAfter)
{
    setText(QObject::tr("Edit hyperlink"));
}

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(const quint64 hyperlinkId, const QString & linkBefore, const QString & textBefore,
                                                   const QString & linkAfter, const QString & textAfter, NoteEditorPrivate & noteEditorPrivate,
                                                   const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_hyperlinkId(hyperlinkId),
    m_linkBefore(linkBefore),
    m_textBefore(textBefore),
    m_linkAfter(linkAfter),
    m_textAfter(textAfter)
{}

EditHyperlinkUndoCommand::~EditHyperlinkUndoCommand()
{}

void EditHyperlinkUndoCommand::redoImpl()
{
    QNDEBUG("EditHyperlinkUndoCommand::redoImpl");
    m_noteEditorPrivate.replaceHyperlinkContent(m_hyperlinkId, m_linkAfter, m_textAfter);
}

void EditHyperlinkUndoCommand::undoImpl()
{
    QNDEBUG("EditHyperlinkUndoCommand::undoImpl");
    m_noteEditorPrivate.replaceHyperlinkContent(m_hyperlinkId, m_linkBefore, m_textBefore);
}

} // namespace qute_note
