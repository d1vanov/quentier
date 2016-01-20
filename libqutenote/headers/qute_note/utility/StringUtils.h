#ifndef __LIB_QUTE_NOTE__UTILITY__STRING_UTILS_H
#define __LIB_QUTE_NOTE__UTILITY__STRING_UTILS_H

#include <qute_note/utility/Linkage.h>
#include <QString>

namespace qute_note {

void QUTE_NOTE_EXPORT removePunctuation(QString & str, const bool keepAsterisk = true);

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__STRING_UTILS_H
