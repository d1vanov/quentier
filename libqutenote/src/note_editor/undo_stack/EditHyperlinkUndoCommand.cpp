#include "EditHyperlinkUndoCommand.h"
#include "EditHyperlinkUndoCommandNotReadyException.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_htmlBefore(),
    m_htmlAfter(),
    m_htmlBeforeSet(false),
    m_htmlAfterSet(false)
{
    m_ready = false;
    init();
}

EditHyperlinkUndoCommand::EditHyperlinkUndoCommand(NoteEditorPrivate & noteEditorPrivate, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_htmlBefore(),
    m_htmlAfter(),
    m_htmlBeforeSet(false),
    m_htmlAfterSet(false)
{
    m_ready = false;
    init();
}

EditHyperlinkUndoCommand::~EditHyperlinkUndoCommand()
{}

void EditHyperlinkUndoCommand::setHtmlBefore(const QString & htmlBefore)
{
    m_htmlBefore = htmlBefore;
    m_htmlBeforeSet = true;

    if (m_htmlBeforeSet && m_htmlAfterSet) {
        m_ready = true;
    }
}

void EditHyperlinkUndoCommand::setHtmlAfter(const QString & htmlAfter)
{
    m_htmlAfter = htmlAfter;
    m_htmlAfterSet = true;

    if (m_htmlBeforeSet && m_htmlAfterSet) {
        m_ready = true;
    }
}

void EditHyperlinkUndoCommand::redo()
{
    QNDEBUG("EditHyperlinkUndoCommand::redo");

    if (Q_UNLIKELY(!m_ready)) {
        throw EditHyperlinkUndoCommandNotReadyException();
    }

    m_noteEditorPrivate.setNoteHtml(m_htmlAfter);
}

void EditHyperlinkUndoCommand::undo()
{
    QNDEBUG("EditHyperlinkUndoCommand::undo");

    if (Q_UNLIKELY(!m_ready)) {
        throw EditHyperlinkUndoCommandNotReadyException();
    }

    m_noteEditorPrivate.setNoteHtml(m_htmlBefore);
}

void EditHyperlinkUndoCommand::init()
{
    QUndoCommand::setText(QObject::tr("Add/edit/remove hyperlink"));
}

} // namespace qute_note
