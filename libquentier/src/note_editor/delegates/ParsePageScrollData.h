#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATES_PARSE_PAGE_SCROLL_DATA_H
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATES_PARSE_PAGE_SCROLL_DATA_H

#include <QString>
#include <QVariant>

namespace quentier {

bool parsePageScrollData(const QVariant & data, int & pageXOffset, int & pageYOffset, QString & errorDescription);

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATES_PARSE_PAGE_SCROLL_DATA_H
