#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_H

#include <QStringList>

namespace qute_note {

class SpellChecker
{
public:
    SpellChecker();

    QStringList misSpelledWords(const QStringList & words) const;
    QStringList spellCorrectionSuggestions(const QString & misSpelledWord) const;
    void learnNewWord(const QString & word);
    void removeWord(const QString & word);
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__SPELL_CHECKER_H
