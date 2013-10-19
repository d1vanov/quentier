#include "SynchronizedDataElement.h"
#include "../Guid.h"

namespace qute_note {

SynchronizedDataElement::SynchronizedDataElement() :
    m_updateSequenceNumber(0),
    m_isLocallyModified(true),
    m_pGuid(new Guid)
{}

SynchronizedDataElement::SynchronizedDataElement(const SynchronizedDataElement & other) :
    m_updateSequenceNumber(other.m_updateSequenceNumber + 1),
    m_isLocallyModified(true),
    m_pGuid(new Guid)
{}

SynchronizedDataElement & SynchronizedDataElement::operator =(const SynchronizedDataElement & other)
{
    if (this != &other)
    {
        m_updateSequenceNumber = other.m_updateSequenceNumber + 1;
        m_isLocallyModified = true;
    }

    return *this;
}

SynchronizedDataElement::~SynchronizedDataElement()
{}

uint64_t SynchronizedDataElement::updateSequenceNumber() const
{
    return m_updateSequenceNumber;
}

void SynchronizedDataElement::setUpdateSequenceNumber(const uint64_t usn)
{
    m_updateSequenceNumber = usn;
}

bool SynchronizedDataElement::isLocallyModified() const
{
    return m_isLocallyModified;
}

void SynchronizedDataElement::setLocallyModified()
{
    m_isLocallyModified = true;
}

void SynchronizedDataElement::dropLocallyModified()
{
    m_isLocallyModified = false;
}

const Guid SynchronizedDataElement::guid() const
{
    if (m_pGuid.get() != nullptr) {
        return *(m_pGuid.get());
    }
    else {
        return Guid();
    }
}

void SynchronizedDataElement::assignGuid(const std::string & guid)
{
    m_pGuid.reset(new Guid(guid));
}

bool SynchronizedDataElement::isEmpty() const
{
    if (m_pGuid.get() != nullptr) {
        return m_pGuid->isEmpty();
    }
    else {
        return true;
    }
}

bool SynchronizedDataElement::operator <(const SynchronizedDataElement & other)
{
    if ((m_pGuid.get() != nullptr) && (other.m_pGuid.get() != nullptr)) {
        return (*m_pGuid < *(other.m_pGuid));
    }
    else {
        return true;
    }
}

}
