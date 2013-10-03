#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_H

#include "types/SynchronizedDataElement.h"
#include <string>

namespace qute_note {

class Resource: public SynchronizedDataElement
{
public:
    Resource() {}

    const std::string & data() const;
    const std::string & dataHash() const;
    int32_t dataSize() const { return static_cast<int32_t>(data().size()); }

    const Guid noteGuid() const;

    // TODO: continue to form interface
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__RESOURCE_H
