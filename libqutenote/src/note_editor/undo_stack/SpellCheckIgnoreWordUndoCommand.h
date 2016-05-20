#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_SPELL_CHECK_IGNORE_WORD_UNDO_COMMAND_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_SPELL_CHECK_IGNORE_WORD_UNDO_COMMAND_H

#include "INoteEditorUndoCommand.h"
#include <QPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(SpellChecker)

class SpellCheckIgnoreWordUndoCommand: public INoteEditorUndoCommand
{
public:
    SpellCheckIgnoreWordUndoCommand(NoteEditorPrivate & noteEditor, const QString & ignoredWord,
                                    SpellChecker * pSpellChecker, QUndoCommand * parent = Q_NULLPTR);
    SpellCheckIgnoreWordUndoCommand(NoteEditorPrivate & noteEditor, const QString & ignoredWord,
                                    SpellChecker * pSpellChecker, const QString & text, QUndoCommand * parent = Q_NULLPTR);
    virtual ~SpellCheckIgnoreWordUndoCommand();

    virtual void redoImpl() Q_DECL_OVERRIDE;
    virtual void undoImpl() Q_DECL_OVERRIDE;

private:
    QPointer<SpellChecker>  m_pSpellChecker;
    QString                 m_ignoredWord;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_UNDO_STACK_SPELL_CHECK_IGNORE_WORD_UNDO_COMMAND_H
