#ifndef __LIB_QUTE_NOTE__UTILITY__STRING_UTILS_PRIVATE_H
#define __LIB_QUTE_NOTE__UTILITY__STRING_UTILS_PRIVATE_H

#include <qute_note/utility/StringUtils.h>

namespace qute_note {

class StringUtilsPrivate
{
public:
    StringUtilsPrivate();

    void removePunctuation(QString & str, const QVector<QChar> & charactersToPreserve) const;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__STRING_UTILS_PRIVATE_H
