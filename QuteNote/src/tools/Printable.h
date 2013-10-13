#ifndef __QUTE_NOTE__TOOLS_PRINTABLE_H
#define __QUTE_NOTE__TOOLS_PRINTABLE_H

#include <iosfwd>
#include <QString>

namespace qute_note {

class Printable {
public:
    virtual ~Printable() {}

    friend std::ostream & operator << (std::ostream & strm, const Printable & printable);

    virtual std::ostream & Print(std::ostream & strm) const = 0;

    virtual const QString ToQString() const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__TOOLS_PRINTABLE_H
