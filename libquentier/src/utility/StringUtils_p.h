#ifndef LIB_QUENTIER_UTILITY_STRING_UTILS_PRIVATE_H
#define LIB_QUENTIER_UTILITY_STRING_UTILS_PRIVATE_H

#include <quentier/utility/StringUtils.h>
#include <QHash>
#include <QStringList>

namespace quentier {

class StringUtilsPrivate
{
public:
    StringUtilsPrivate();

    void removePunctuation(QString & str, const QVector<QChar> & charactersToPreserve) const;
    void removeDiacritics(QString & str) const;

private:
    void initialize();

private:
    QString     m_diacriticLetters;
    QStringList m_noDiacriticLetters;
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_STRING_UTILS_PRIVATE_H
