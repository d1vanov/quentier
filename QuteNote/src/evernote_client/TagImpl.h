#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TAG_IMPL_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TAG_IMPL_H

#include <string>
#include "Guid.h"

namespace qute_note {

class TagImpl
{
public:
    TagImpl(const std::string & name);
    TagImpl(const std::string & name, const Guid & parentGuid);

    std::string name() const;
    Guid parentGuid() const;

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
