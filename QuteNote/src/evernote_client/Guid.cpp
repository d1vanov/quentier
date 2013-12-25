#include "Guid.h"

namespace qute_note {

Guid::Guid() :
    m_guid()
{}

Guid::Guid(const std::string & guid) :
    m_guid(guid)
{}

Guid::Guid(const Guid & other) :
    m_guid(other.m_guid)
{}

Guid & Guid::operator=(const Guid & other)
{
    if (this != &other) {
        m_guid = other.m_guid;
    }

    return *this;
}

bool Guid::isEmpty() const
{
    return m_guid.empty();
}

bool Guid::operator ==(const Guid & other) const
{
    return (m_guid == other.m_guid);
}

bool Guid::operator !=(const Guid & other) const
{
    return !(*this == other);
}

bool Guid::operator <(const Guid & other) const
{
    return (m_guid < other.m_guid);
}

QTextStream & Guid::Print(QTextStream & strm) const
{
    strm << QString::fromStdString(m_guid);
    return strm;
}

}
