#include "SpellCheckerDynamicHelper.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

SpellCheckerDynamicHelper::SpellCheckerDynamicHelper(QObject * parent) :
    QObject(parent)
{}

#ifdef USE_QT_WEB_ENGINE
void SpellCheckerDynamicHelper::setLastEnteredWords(QVariant words)
{
    QNDEBUG("SpellCheckerDynamicHelper::setLastEnteredWords: " << words);

    QStringList wordsList = words.toStringList();
    emit lastEnteredWords(wordsList);
}
#else
void SpellCheckerDynamicHelper::setLastEnteredWords(QVariantList words)
{
    QNDEBUG("SpellCheckerDynamicHelper::setLastEnteredWords: " << words);

    QStringList wordsList;
    for(auto it = words.begin(), end = words.end(); it != end; ++it) {
        const QVariant & word = *it;
        wordsList << word.toString();
    }
    emit lastEnteredWords(wordsList);
}
#endif

} // namespace qute_note
