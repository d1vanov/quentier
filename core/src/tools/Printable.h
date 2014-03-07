#ifndef __QUTE_NOTE__TOOLS_PRINTABLE_H
#define __QUTE_NOTE__TOOLS_PRINTABLE_H

#include "Linkage.h"
#include <QString>
#include <QTextStream>
#include <QDebug>
#include <Types_types.h>

namespace qute_note {

/**
 * @brief The Printable class is the interface for QuteNote's internal classes
 * which should be able to write themselves into QTextStream and/or convert to QString
 */
class QUTE_NOTE_EXPORT Printable {
public:
    virtual ~Printable();

    virtual QTextStream & Print(QTextStream & strm) const = 0;

    virtual const QString ToQString() const;

    friend QUTE_NOTE_EXPORT QTextStream & operator << (QTextStream & strm,
                                                       const Printable & printable);
    friend QUTE_NOTE_EXPORT QDebug & operator << (QDebug & debug,
                                                  const Printable & printable);
};

} // namespace qute_note

// QTextStream operators for existing classes not inheriting from Printable

QTextStream & operator << (QTextStream & strm, const evernote::edam::BusinessUserInfo & info);
QTextStream & operator << (QTextStream & strm, const evernote::edam::PremiumInfo & info);
QTextStream & operator << (QTextStream & strm, const evernote::edam::Accounting & accounting);
QTextStream & operator << (QTextStream & strm, const evernote::edam::UserAttributes & attributes);
QTextStream & operator << (QTextStream & strm, const evernote::edam::NoteAttributes & attributes);
QTextStream & operator << (QTextStream & strm, const evernote::edam::ResourceAttributes & attributes);
QTextStream & operator << (QTextStream & strm, const evernote::edam::PrivilegeLevel::type level);
QTextStream & operator << (QTextStream & strm, const evernote::edam::Guid & guid);

template<class T>
const QString ToQString(const T & object)
{
    QString str;
    QTextStream strm(&str, QIODevice::WriteOnly);
    strm << object;
    return std::move(str);
}

#endif // __QUTE_NOTE__TOOLS_PRINTABLE_H
