#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_PARSE_PAGE_SCROLL_DATA_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_PARSE_PAGE_SCROLL_DATA_H

#include <QString>
#include <QVariant>

namespace qute_note {

bool parsePageScrollData(const QVariant & data, int & pageXOffset, int & pageYOffset, QString & errorDescription);

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_PARSE_PAGE_SCROLL_DATA_H
