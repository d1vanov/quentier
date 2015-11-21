#include "EncryptUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

EncryptUndoCommand::EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                       QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_htmlWithEncryption()
{
    init();
}

EncryptUndoCommand:: EncryptUndoCommand(NoteEditorPrivate & noteEditorPrivate,
                                        const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_htmlWithEncryption()
{
    init();
}

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

void EncryptUndoCommand::setHtmlWithEncryption(const QString & html)
{
    QNDEBUG("EncryptUndoCommand::setHtmlWithEncryption");

    m_htmlWithEncryption = html;
    m_ready = true;
}

void EncryptUndoCommand::init()
{
    m_ready = false;
    QUndoCommand::setText(QObject::tr("Encrypt text"));
}

} // namespace qute_note

