#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__PARSE_PAGE_SCROLL_DATA_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__PARSE_PAGE_SCROLL_DATA_H

#include <QString>
#include <QVariant>

namespace qute_note {

bool parsePageScrollData(const QVariant & data, int & pageXOffset, int & pageYOffset, QString & errorDescription);

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__PARSE_PAGE_SCROLL_DATA_H
