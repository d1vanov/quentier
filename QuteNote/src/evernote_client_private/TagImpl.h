#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TAG_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TAG_IMPL_H

#include <string>
#include "../evernote_client/Guid.h"
#include "../evernote_client/Tag.h"

namespace qute_note {

class Tag::TagImpl
{
public:
    TagImpl(const std::string & name);
    TagImpl(const std::string & name, const Guid & parentGuid);

    const std::string name() const;
    const Guid & parentGuid() const;

    void setName(const std::string & name);
    void setParentGuid(const Guid & parentGuid);

private:
    TagImpl() = delete;
    TagImpl(const TagImpl & other) = delete;
    TagImpl & operator=(const TagImpl & other) = delete;

private:
    std::string m_name;
    Guid m_parentGuid;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TAG_IMPL_H
