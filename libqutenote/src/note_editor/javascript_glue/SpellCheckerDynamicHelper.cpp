#include "SpellCheckerDynamicHelper.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

SpellCheckerDynamicHelper::SpellCheckerDynamicHelper(QObject * parent) :
    QObject(parent)
{}

void SpellCheckerDynamicHelper::setLastEnteredWords(QVariant words)
{
    QNDEBUG("SpellCheckerDynamicHelper::setLastEnteredWords: " << words);

    QStringList wordsList = words.toStringList();
    emit lastEnteredWords(wordsList);
}

} // namespace qute_note
