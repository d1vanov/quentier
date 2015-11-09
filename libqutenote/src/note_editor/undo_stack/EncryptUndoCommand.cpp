#include "EncryptUndoCommand.h"
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/utility/EncryptionManager.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

EncryptUndoCommand::EncryptUndoCommand(const EncryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                                       EncryptionManager & encryptionManager, NoteEditor & noteEditor, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_info(info),
    m_decryptedTextManager(decryptedTextManager),
    m_encryptionManager(encryptionManager)
{}

EncryptUndoCommand:: EncryptUndoCommand(const EncryptUndoCommandInfo & info, DecryptedTextManager & decryptedTextManager,
                                        EncryptionManager & encryptionManager, NoteEditor & noteEditor, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_info(info),
    m_decryptedTextManager(decryptedTextManager),
    m_encryptionManager(encryptionManager)
{}

EncryptUndoCommand::~EncryptUndoCommand()
{}

void EncryptUndoCommand::redo()
{
    QNDEBUG("EncryptUndoCommand::redo");
    Q_UNUSED(m_decryptedTextManager);
    Q_UNUSED(m_encryptionManager);
    // TODO: implement
}

void EncryptUndoCommand::undo()
{
    QNDEBUG("EncryptUndoCommand::undo");
    // TODO: implement
}

} // namespace qute_note

