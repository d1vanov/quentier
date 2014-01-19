#ifndef __QUTE_NOTE__TOOLS_PRINTABLE_H
#define __QUTE_NOTE__TOOLS_PRINTABLE_H

#include <QString>
#include <QTextStream>

namespace qute_note {

class Printable {
public:
    virtual ~Printable();

    virtual QTextStream & Print(QTextStream & strm) const = 0;

    virtual const QString ToQString() const;

    friend QTextStream & operator << (QTextStream & strm,
                                      const Printable & printable);
};

} // namespace qute_note

#endif // __QUTE_NOTE__TOOLS_PRINTABLE_H
