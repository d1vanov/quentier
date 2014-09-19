#include "UserWrapperData.h"

namespace qute_note {

UserWrapperData::UserWrapperData() :
    m_qecUser()
{}

UserWrapperData::UserWrapperData(const UserWrapperData & other) :
    m_qecUser(other.m_qecUser)
{}

UserWrapperData::UserWrapperData(UserWrapperData && other) :
    m_qecUser(std::move(other.m_qecUser))
{}

UserWrapperData & UserWrapperData::operator=(const UserWrapperData & other)
{
    if (this != std::addressof(other)) {
        m_qecUser = other.m_qecUser;
    }

    return *this;
}

UserWrapperData & UserWrapperData::operator=(UserWrapperData && other)
{
    if (this != std::addressof(other)) {
        m_qecUser = std::move(other.m_qecUser);
    }

    return *this;
}

UserWrapperData::~UserWrapperData()
{}

} // namespace qute_note
