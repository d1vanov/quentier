#include "Printable.h"
#include <Types_types.h>

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

QTextStream & operator <<(QTextStream & strm, const evernote::edam::BusinessUserInfo & info)
{
    strm << "BusinessUserInfo: {";

    const auto & isSet = info.__isset;

#define CHECK_AND_PRINT_ATTRIBUTE(name, ...) \
    bool isSet##name = isSet.name; \
    strm << #name " is " << (isSet##name ? "set" : "not set") << "\n"; \
    if (isSet##name) { \
        strm << #name " = " << __VA_ARGS__(info.name) << "\n"; \
    }

    CHECK_AND_PRINT_ATTRIBUTE(businessId);
    CHECK_AND_PRINT_ATTRIBUTE(businessName, QString::fromStdString);
    CHECK_AND_PRINT_ATTRIBUTE(role, static_cast<quint8>);
    CHECK_AND_PRINT_ATTRIBUTE(email, QString::fromStdString);

#undef CHECK_AND_PRINT_ATTRIBUTE

    strm << "}; \n";
    return strm;
}


