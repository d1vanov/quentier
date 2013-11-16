#include "SynchronizedDataElement.h"

namespace qute_note {

SynchronizedDataElement::SynchronizedDataElement() :
    m_updateSequenceNumber(0),
    m_isDirty(true),
    m_guid()
{}

SynchronizedDataElement::SynchronizedDataElement(const SynchronizedDataElement & other) :
    m_updateSequenceNumber(other.m_updateSequenceNumber + 1),
    m_isDirty(true),
    m_guid()
{}

SynchronizedDataElement & SynchronizedDataElement::operator =(const SynchronizedDataElement & other)
{
    if (this != &other)
    {
        m_updateSequenceNumber = other.m_updateSequenceNumber + 1;
        m_isDirty = true;
    }

    return *this;
}

SynchronizedDataElement::~SynchronizedDataElement()
{}

unsigned int SynchronizedDataElement::getUpdateSequenceNumber() const
{
    return m_updateSequenceNumber;
}

void SynchronizedDataElement::setUpdateSequenceNumber(const unsigned int usn)
{
    m_updateSequenceNumber = usn;
}

bool SynchronizedDataElement::isDirty() const
{
    return m_isDirty;
}

void SynchronizedDataElement::setDirty()
{
    m_isDirty = true;
}

void SynchronizedDataElement::setSynchronized()
{
    m_isDirty = false;
}

const Guid & SynchronizedDataElement::guid() const
{
    return m_guid;
}

void SynchronizedDataElement::assignGuid(const std::string & guid)
{
    m_guid.setGuidString(guid);
}

bool SynchronizedDataElement::isEmpty() const
{
    return m_guid.isEmpty();
}

bool SynchronizedDataElement::operator <(const SynchronizedDataElement & other)
{
    return (m_guid < other.m_guid);
}

QTextStream & SynchronizedDataElement::Print(QTextStream & strm) const
{
    strm << "SynchronizedDataElement: { " << "\n";
    strm << "  Update sequence number = " << m_updateSequenceNumber << ",\n";
    strm << "  \"Is dirty\" = " << (m_isDirty ? "true" : "false") << ",\n";
    strm << "  Guid = " << m_guid << "\n };";

    return strm;
}

}
