#include "SpellCheckIgnoreWordUndoCommand.h"
#include "../NoteEditor_p.h"
#include "../SpellChecker.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

SpellCheckIgnoreWordUndoCommand::SpellCheckIgnoreWordUndoCommand(NoteEditorPrivate & noteEditor, const QString & ignoredWord,
                                                                 SpellChecker * pSpellChecker, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_pSpellChecker(pSpellChecker),
    m_ignoredWord(ignoredWord)
{
    setText(QObject::tr("Ignore word"));
}

SpellCheckIgnoreWordUndoCommand::SpellCheckIgnoreWordUndoCommand(NoteEditorPrivate & noteEditor, const QString & ignoredWord,
                                                                 SpellChecker * pSpellChecker, const QString & text,
                                                                 QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_pSpellChecker(pSpellChecker),
    m_ignoredWord(ignoredWord)
{}

SpellCheckIgnoreWordUndoCommand::~SpellCheckIgnoreWordUndoCommand()
{}

void SpellCheckIgnoreWordUndoCommand::redoImpl()
{
    QNDEBUG("SpellCheckIgnoreWordUndoCommand::redoImpl");

    if (Q_UNLIKELY(m_pSpellChecker.isNull())) {
        QNTRACE("No spell checker");
        return;
    }

    m_pSpellChecker->ignoreWord(m_ignoredWord);

    if (m_noteEditorPrivate.spellCheckEnabled()) {
        m_noteEditorPrivate.refreshMisSpelledWordsList();
        m_noteEditorPrivate.applySpellCheck();
    }
}

void SpellCheckIgnoreWordUndoCommand::undoImpl()
{
    QNDEBUG("SpellCheckIgnoreWordUndoCommand::undoImpl");

    if (Q_UNLIKELY(m_pSpellChecker.isNull())) {
        QNTRACE("No spell checker");
        return;
    }

    m_pSpellChecker->removeWord(m_ignoredWord);

    if (m_noteEditorPrivate.spellCheckEnabled()) {
        m_noteEditorPrivate.refreshMisSpelledWordsList();
        m_noteEditorPrivate.applySpellCheck();
    }
}

} // namespace qute_note
