#include "SpellCorrectionUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditorPrivate.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't undo/redo spelling correction: can't get note editor's page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

SpellCorrectionUndoCommand::SpellCorrectionUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback,
                                                       QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_callback(callback)
{
    setText(tr("Spelling correction"));
}

SpellCorrectionUndoCommand::SpellCorrectionUndoCommand(NoteEditorPrivate & noteEditor, const Callback & callback,
                                                       const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_callback(callback)
{}

SpellCorrectionUndoCommand::~SpellCorrectionUndoCommand()
{}

void SpellCorrectionUndoCommand::redoImpl()
{
    GET_PAGE()
    page->executeJavaScript("spellChecker.redo();", m_callback);
}

void SpellCorrectionUndoCommand::undoImpl()
{
    GET_PAGE()
    page->executeJavaScript("spellChecker.undo();", m_callback);
}

} // namespace quentier
