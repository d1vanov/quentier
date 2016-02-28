#include "SpellCorrectionUndoCommand.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't undo/redo spelling correction: can't get note editor's page"); \
        QNWARNING(error); \
        return; \
    }

SpellCorrectionUndoCommand::SpellCorrectionUndoCommand(NoteEditorPrivate & noteEditor, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent)
{
    setText(QObject::tr("Spelling correction"));
}

SpellCorrectionUndoCommand::SpellCorrectionUndoCommand(NoteEditorPrivate & noteEditor, const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent)
{}

SpellCorrectionUndoCommand::~SpellCorrectionUndoCommand()
{}

void SpellCorrectionUndoCommand::redoImpl()
{
    GET_PAGE()
    page->executeJavaScript("spellChecker.redo();");
}

void SpellCorrectionUndoCommand::undoImpl()
{
    GET_PAGE()
    page->executeJavaScript("spellChecker.undo();");
}

} // namespace qute_note
