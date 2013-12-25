#include "SynchronizedDataElement.h"

namespace qute_note {

SynchronizedDataElement::SynchronizedDataElement() :
    m_updateSequenceNumber(0),
    m_isDirty(true),
    m_isLocal(true),
    m_isDeleted(false),
    m_guid()
{}

SynchronizedDataElement::SynchronizedDataElement(const SynchronizedDataElement & other) :
    m_updateSequenceNumber(other.m_updateSequenceNumber + 1),
    m_isDirty(true),
    m_isLocal(other.m_isLocal),
    m_isDeleted(other.m_isDeleted),
    m_guid()
{}

SynchronizedDataElement & SynchronizedDataElement::operator =(const SynchronizedDataElement & other)
{
    if (this != &other)
    {
        m_updateSequenceNumber = other.m_updateSequenceNumber + 1;
        m_isDirty = true;
        m_isLocal = other.m_isLocal;
        m_isDeleted = other.m_isDeleted;
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
    return (m_isDirty || m_isLocal);
}

void SynchronizedDataElement::setDirty()
{
    m_isDirty = true;
}

void SynchronizedDataElement::setSynchronized()
{
    m_isDirty = false;
    m_isLocal = false;
}

bool SynchronizedDataElement::isLocal() const
{
    return m_isLocal;
}

bool SynchronizedDataElement::isDeleted() const
{
    return m_isDeleted;
}

void SynchronizedDataElement::setDeleted()
{
    m_isDeleted = true;
}

void SynchronizedDataElement::undelete()
{
    m_isDeleted = false;
}

const Guid & SynchronizedDataElement::guid() const
{
    return m_guid;
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
    strm << "  \"Is local\" = " << (m_isLocal ? "true" : "false") << ",\n";
    strm << "  Guid = " << m_guid << "\n };";

    return strm;
}

void SynchronizedDataElement::assignGuid(const Guid & guid)
{
    m_guid = guid;
}

}
