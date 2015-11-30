#include "EncryptUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

EncryptUndoCommand::EncryptUndoCommand(const QString & htmlWithEncryption, NoteEditorPrivate & noteEditorPrivate,
                                       QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_htmlWithEncryption(htmlWithEncryption)
{
    setText(QObject::tr("Encrypt selected text"));
}

EncryptUndoCommand:: EncryptUndoCommand(const QString & htmlWithEncryption, NoteEditorPrivate & noteEditorPrivate,
                                        const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_htmlWithEncryption(htmlWithEncryption)
{}

EncryptUndoCommand::~EncryptUndoCommand()
{}

void EncryptUndoCommand::redoImpl()
{
    QNDEBUG("EncryptUndoCommand::redoImpl");

    m_noteEditorPrivate.switchEditorPage(/* should convert from note = */ false);
    m_noteEditorPrivate.setNoteHtml(m_htmlWithEncryption);
}

void EncryptUndoCommand::undoImpl()
{
    QNDEBUG("EncryptUndoCommand::undoImpl");

    m_noteEditorPrivate.popEditorPage();
}

} // namespace qute_note

