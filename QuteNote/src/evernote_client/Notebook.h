#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H

#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"

namespace qute_note {

class Notebook: public TypeWithError,
                public SynchronizedDataElement
{
public:
    Notebook();

    virtual bool isEmpty() const;

    /**
     * @return name of the notebook
     */
    const QString name() const;
    void setName(const QString & name);

    time_t createdTimestamp() const;
    time_t updatedTimestamp() const;

    bool isDefault() const;
    bool isLastUsed() const;

    // TODO: create pimpl to NotebookImpl, in NotebookPimpl hold a ref to NoteStore
    // whicn in turn would be able to tell the name of the owner of this notebook
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTEBOOK_H
