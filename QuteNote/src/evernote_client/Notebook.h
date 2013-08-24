#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H

#include "types/TypeWithError.h"
#include "types/SynchronizedDataElement.h"

namespace qute_note {

class Notebook: public TypeWithError,
                public SynchronizedDataElement
{
public:
    Notebook();

    virtual bool isEmpty() const;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H
