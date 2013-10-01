#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H

#include <memory>
#include <string>
#include "types/TypeWithError.h"
#include "types/SynchronizedDataElement.h"

namespace qute_note {

class Guid;
class TagImpl;

class Tag: public TypeWithError,
           public SynchronizedDataElement
{
public:
    Tag(const std::string & name);
    Tag(const std::string & name, const Guid & parentGuid);

    /**
     * @brief parentGuid - guid of parent tag
     * @return guid of parent tag if it exists or empty guid otherwise
     */
    Guid parentGuid() const;
    // TODO: continue

private:
    Tag() = delete;
    Tag(const Tag & other) = delete;
    Tag & operator=(const Tag & other) = delete;

private:
    class TagImpl;
    std::unique_ptr<TagImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TAG_H
