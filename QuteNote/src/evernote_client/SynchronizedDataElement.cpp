#include "SynchronizedDataElement.h"
#include "Guid.h"

namespace qute_note {

SynchronizedDataElement::SynchronizedDataElement() :
    m_updateSequenceNumber(0),
    m_isDirty(true),
    m_pGuid(new Guid)
{}

SynchronizedDataElement::SynchronizedDataElement(const SynchronizedDataElement & other) :
    m_updateSequenceNumber(other.m_updateSequenceNumber + 1),
    m_isDirty(true),
    m_pGuid(new Guid)
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

unsigned int SynchronizedDataElement::updateSequenceNumber() const
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
