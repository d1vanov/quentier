#ifndef __LIB_QUTE_NOTE__UTILITY__STRING_UTILS_H
#define __LIB_QUTE_NOTE__UTILITY__STRING_UTILS_H

#include <qute_note/utility/Linkage.h>
#include <QString>
#include <QVector>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(StringUtilsPrivate)

class QUTE_NOTE_EXPORT StringUtils
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

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__UTILITY__STRING_UTILS_H
