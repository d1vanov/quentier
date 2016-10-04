#include "SpellCheckIgnoreWordUndoCommand.h"
#include "../NoteEditor_p.h"
#include "../SpellChecker.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

SpellCheckIgnoreWordUndoCommand::SpellCheckIgnoreWordUndoCommand(NoteEditorPrivate & noteEditor, const QString & ignoredWord,
                                                                 SpellChecker * pSpellChecker, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_pSpellChecker(pSpellChecker),
    m_ignoredWord(ignoredWord)
{
    setText(tr("Ignore word"));
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
    QNDEBUG(QStringLiteral("SpellCheckIgnoreWordUndoCommand::redoImpl"));

    if (Q_UNLIKELY(m_pSpellChecker.isNull())) {
        QNTRACE(QStringLiteral("No spell checker"));
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
    QNDEBUG(QStringLiteral("SpellCheckIgnoreWordUndoCommand::undoImpl"));

    if (Q_UNLIKELY(m_pSpellChecker.isNull())) {
        QNTRACE(QStringLiteral("No spell checker"));
        return;
    }

    m_pSpellChecker->removeWord(m_ignoredWord);

    if (m_noteEditorPrivate.spellCheckEnabled()) {
        m_noteEditorPrivate.refreshMisSpelledWordsList();
        m_noteEditorPrivate.applySpellCheck();
    }
}

} // namespace quentier
