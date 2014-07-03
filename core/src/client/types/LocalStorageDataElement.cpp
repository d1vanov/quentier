#include "LocalStorageDataElement.h"

namespace qute_note {

LocalStorageDataElement::LocalStorageDataElement() :
    m_localGuid(QUuid::createUuid())
{}

LocalStorageDataElement::~LocalStorageDataElement()
{}

const QString LocalStorageDataElement::localGuid() const
{
    if (m_localGuid.isNull()) {
        return std::move(QString());
    }
    else {
        return std::move(m_localGuid.toString());
    }
}

void LocalStorageDataElement::setLocalGuid(const QString & guid)
{
    m_localGuid = QUuid(guid);
}

void LocalStorageDataElement::unsetLocalGuid()
{
    m_localGuid = QUuid();
}

LocalStorageDataElement::LocalStorageDataElement(LocalStorageDataElement && other) :
    m_localGuid(std::move(other.m_localGuid))
{}

LocalStorageDataElement & LocalStorageDataElement::operator=(LocalStorageDataElement && other)
{
    if (this != &other) {
        m_localGuid = std::move(other.m_localGuid);
    }

    return *this;
}

} // namespace qute_note
