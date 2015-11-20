#include "EncryptUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

EncryptUndoCommand::EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                       QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent)
{
    init();
}

EncryptUndoCommand:: EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                        const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent)
{
    init();
}

EncryptUndoCommand::~EncryptUndoCommand()
{}

void EncryptUndoCommand::redoImpl()
{
    QNDEBUG("EncryptUndoCommand::redoImpl");

    m_noteEditorPrivate.switchEditorPage();
}

void EncryptUndoCommand::undoImpl()
{
    QNDEBUG("EncryptUndoCommand::undoImpl");

    m_noteEditorPrivate.popEditorPage();
}

void EncryptUndoCommand::init()
{
    QUndoCommand::setText(QObject::tr("Encrypt text"));
}

} // namespace qute_note

