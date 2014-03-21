#include "ISharedNotebook.h"

namespace qute_note {

ISharedNotebook::ISharedNotebook()
{}

ISharedNotebook::~ISharedNotebook()
{}

bool ISharedNotebook::hasId() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.__isset.id;
}

qint64 ISharedNotebook::id() const
{
    const auto & enSharedNotebook = GetEnSharedNotebook();
    return enSharedNotebook.id;
}

void ISharedNotebook::setId(const qint64 id)
{
    auto & enSharedNotebook = GetEnSharedNotebook();
    enSharedNotebook.id = id;
    enSharedNotebook.__isset.id = true;
}

QTextStream & ISharedNotebook::Print(QTextStream & strm) const
{
    // TODO: implement
    return strm;
}


// TODO: continue

} // namespace qute_note
