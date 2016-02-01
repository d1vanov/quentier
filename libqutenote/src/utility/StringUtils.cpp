#include <qute_note/utility/StringUtils.h>
#include "StringUtils_p.h"

namespace qute_note {

StringUtils::StringUtils() :
    d_ptr(new StringUtilsPrivate)
{}

StringUtils::~StringUtils()
{
    delete d_ptr;
}

void StringUtils::removePunctuation(QString & str, const QVector<QChar> & charactersToPreserve) const
{
    Q_D(const StringUtils);
    d->removePunctuation(str, charactersToPreserve);
}

} // namespace qute_note
