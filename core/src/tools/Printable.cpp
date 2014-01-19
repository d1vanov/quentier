#include "Printable.h"

namespace qute_note {

Printable::~Printable()
{}

const QString Printable::ToQString() const
{
    QTextStream strm;
    strm << *this;
    return strm.readAll();
}

QTextStream & operator <<(QTextStream & strm,
                          const Printable & printable)
{
    return printable.Print(strm);
}

} // namespace qute_note
