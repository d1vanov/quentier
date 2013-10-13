#include "Printable.h"
#include <ostream>
#include <sstream>

namespace qute_note {

std::ostream & operator << (std::ostream & strm, const Printable & printable)
{
    return printable.Print(strm);
}

const QString Printable::ToQString() const
{
    std::ostringstream sstrm;
    sstrm << *this;
    return QString(sstrm.str().c_str());
}

} // namespace qute_note
