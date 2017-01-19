#include "SpellCheckAddToUserWordListUndoCommand.h"
#include "../NoteEditor_p.h"
#include <quentier/note_editor/SpellChecker.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

SpellCheckAddToUserWordListUndoCommand::SpellCheckAddToUserWordListUndoCommand(NoteEditorPrivate & noteEditor, const QString & word,
                                                                               SpellChecker * pSpellChecker, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, parent),
    m_pSpellChecker(pSpellChecker),
    m_word(word)
{
    setText(tr("Add to user word list"));
}

SpellCheckAddToUserWordListUndoCommand::SpellCheckAddToUserWordListUndoCommand(NoteEditorPrivate & noteEditor, const QString & word,
                                                                               SpellChecker * pSpellChecker, const QString & text,
                                                                               QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditor, text, parent),
    m_pSpellChecker(pSpellChecker),
    m_word(word)
{}

SpellCheckAddToUserWordListUndoCommand::~SpellCheckAddToUserWordListUndoCommand()
{}

void SpellCheckAddToUserWordListUndoCommand::redoImpl()
{
    QNDEBUG(QStringLiteral("SpellCheckAddToUserWordListUndoCommand::redoImpl"));

    if (Q_UNLIKELY(m_pSpellChecker.isNull())) {
        QNTRACE(QStringLiteral("No spell checker"));
        return;
    }

    m_pSpellChecker->addToUserWordlist(m_word);

    if (m_noteEditorPrivate.spellCheckEnabled()) {
        m_noteEditorPrivate.refreshMisSpelledWordsList();
        m_noteEditorPrivate.applySpellCheck();
    }
}

void SpellCheckAddToUserWordListUndoCommand::undoImpl()
{
    QNDEBUG(QStringLiteral("SpellCheckAddToUserWordListUndoCommand::undoImpl"));

    if (Q_UNLIKELY(m_pSpellChecker.isNull())) {
        QNTRACE(QStringLiteral("No spell checker"));
        return;
    }

    m_pSpellChecker->removeFromUserWordList(m_word);

    if (m_noteEditorPrivate.spellCheckEnabled()) {
        m_noteEditorPrivate.refreshMisSpelledWordsList();
        m_noteEditorPrivate.applySpellCheck();
    }
}

} // namespace quentier
