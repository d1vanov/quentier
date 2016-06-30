#include <quentier/utility/StringUtils.h>
#include "StringUtils_p.h"

namespace quentier {

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

void StringUtils::removeDiacritics(QString & str) const
{
    Q_D(const StringUtils);
    d->removeDiacritics(str);
}

} // namespace quentier
