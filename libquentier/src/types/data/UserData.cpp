#include "UserData.h"

namespace quentier {

UserData::UserData() :
    m_qecUser(),
    m_isLocal(true),
    m_isDirty(true)
{}

UserData::UserData(const UserData & other) :
    m_qecUser(other.m_qecUser),
    m_isLocal(other.m_isLocal),
    m_isDirty(other.m_isDirty)
{}

UserData::UserData(UserData && other) :
    m_qecUser(std::move(other.m_qecUser)),
    m_isLocal(std::move(other.m_isLocal)),
    m_isDirty(std::move(other.m_isDirty))
{}

UserData::UserData(const qevercloud::User & user) :
    m_qecUser(user),
    m_isLocal(false),
    m_isDirty(true)
{}

UserData::~UserData()
{}

} // namespace quentier
