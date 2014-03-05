#ifndef __QUTE_NOTE__CLIENT__TYPES__IUSER_H
#define __QUTE_NOTE__CLIENT__TYPES__IUSER_H

#include <tools/Printable.h>
#include <QtGlobal>

namespace evernote {
namespace edam {
QT_FORWARD_DECLARE_CLASS(User)
}
}

namespace qute_note {

class IUser: public Printable
{
public:
    IUser();
    virtual ~IUser();

    void Clear();

    bool IsDirty() const;
    void SetDirty();
    void SetClean();

    bool IsLocal() const;
    void SetLocal();
    void SetNonLocal();

    bool CheckParameters(QString & errorDescription) const;

    virtual const evernote::edam::User & GetEnUser() const = 0;
    virtual evernote::edam::User & GetEnUser() = 0;

protected:
    IUser(const IUser & other);

private:
    // prevent slicing:
    IUser & operator=(const IUser & other) = delete;

    virtual QTextStream & Print(QTextStream & strm) const;

    bool m_isDirty;
    bool m_isLocal;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__IUSER_H
