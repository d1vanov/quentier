#ifndef LIB_QUENTIER_UTILITY_STRING_UTILS_H
#define LIB_QUENTIER_UTILITY_STRING_UTILS_H

#include <quentier/utility/Linkage.h>
#include <QString>
#include <QVector>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(StringUtilsPrivate)

class QUENTIER_EXPORT StringUtils
{
public:
    StringUtils();
    virtual ~StringUtils();

    void removePunctuation(QString & str, const QVector<QChar> & charactersToPreserve = QVector<QChar>()) const;
    void removeDiacritics(QString & str) const;

private:
    StringUtilsPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(StringUtils);
};

} // namespace quentier

#endif // LIB_QUENTIER_UTILITY_STRING_UTILS_H
