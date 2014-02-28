#ifndef __QUTE_NOTE__CLIENT__TYPES__IRESOURCE_H
#define __QUTE_NOTE__CLIENT__TYPES__IRESOURCE_H

#include <QtGlobal>

namespace evernote {
namespace edam {
QT_FORWARD_DECLARE_CLASS(Resource)
}
}

QT_FORWARD_DECLARE_CLASS(QString)

namespace qute_note {

class IResource
{
public:
    IResource();
    virtual ~IResource();

    void Clear();

    bool IsDirty() const;
    void SetDirty();
    void SetClean();

    bool CheckParameters(QString & errorDescription, const bool isFreeAccount = true) const;

    virtual const evernote::edam::Resource & GetEnResource() const = 0;
    virtual evernote::edam::Resource & GetEnResource() = 0;

protected:
    IResource(const IResource & other);

private:
    // prevent slicing
    IResource & operator=(const IResource & other) = delete;

    bool m_isDirty;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__IRESOURCE_H
