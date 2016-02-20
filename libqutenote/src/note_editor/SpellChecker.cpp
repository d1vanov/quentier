#include "SpellChecker.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

SpellChecker::SpellChecker()
{}

QStringList SpellChecker::misSpelledWords(const QStringList & words) const
{
    QNDEBUG("SpellChecker::misSpelledWords");
    QNTRACE(words.join(" "));

    // TODO: implement
    return QStringList();
}

QStringList SpellChecker::spellCorrectionSuggestions(const QString & misSpelledWord) const
{
    QNDEBUG("SpellChecker::spellCorrectionSuggestions: " << misSpelledWord);

    // TODO: implement
    return QStringList();
}

void SpellChecker::learnNewWord(const QString & word)
{
    QNDEBUG("SpellChecker::learnNewWord: " << word);

    // TODO: implement
}

void SpellChecker::removeWord(const QString & word)
{
    QNDEBUG("SpellChecker::removeWord: " << word);

    // TODO: implement
}

} // namespace qute_note
