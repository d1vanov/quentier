#include "LocalStorageDataElement.h"

namespace qute_note {

LocalStorageDataElement::LocalStorageDataElement() :
    m_localGuid(QUuid::createUuid())
{}

LocalStorageDataElement::~LocalStorageDataElement()
{}

const QString LocalStorageDataElement::localGuid() const
{
    return std::move(m_localGuid.toString());
}

void LocalStorageDataElement::setLocalGuid(const QString & guid)
{
    m_localGuid = QUuid(guid);
}

} // namespace qute_note
