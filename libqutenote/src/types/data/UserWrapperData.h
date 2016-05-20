#ifndef LIB_QUTE_NOTE_TYPES_DATA_USER_WRAPPER_DATA_H
#define LIB_QUTE_NOTE_TYPES_DATA_USER_WRAPPER_DATA_H

#include <QEverCloud.h>
#include <QSharedData>

namespace qute_note {

class UserWrapperData : public QSharedData
{
public:
    UserWrapperData();
    UserWrapperData(const UserWrapperData & other);
    UserWrapperData(UserWrapperData && other);
    virtual ~UserWrapperData();

    qevercloud::User    m_qecUser;

private:
    UserWrapperData & operator=(const UserWrapperData & other) Q_DECL_DELETE;
    UserWrapperData & operator=(UserWrapperData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_TYPES_DATA_USER_WRAPPER_DATA_H
