#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H

#include <memory>
#include <string>
#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"

namespace qute_note {

class Guid;
class TagImpl;

class Tag: public TypeWithError,
           public SynchronizedDataElement
{
public:
    Tag(const std::string & name);
    Tag(const std::string & name, const Guid & parentGuid);
    Tag(const Tag & other);
    Tag & operator=(const Tag & other);

    /**
     * @brief parentGuid - guid of parent tag
     * @return guid of parent tag if it exists or empty guid otherwise
     */
    const Guid parentGuid() const;

    /**
     * @brief name - name of tag
     */
    const std::string name() const;

    void setName(const std::string & name);
    void setParentGuid(const Guid & parentGuid);

private:
    Tag() = delete;

private:
    class TagImpl;
    std::unique_ptr<TagImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
