#ifndef __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_WRAPPER_H
#define __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_WRAPPER_H

#include "ISharedNotebook.h"
#include <Types_types.h>

namespace qute_note {

/**
 * @brief The SharedNotebookWrapper class creates and manages its own instance of
 * evernote::edam::SharedNotebook object
 */
class SharedNotebookWrapper final: public ISharedNotebook
{
public:
    SharedNotebookWrapper();
    SharedNotebookWrapper(const evernote::edam::SharedNotebook & sharedNotebook);
    SharedNotebookWrapper(const SharedNotebookWrapper & other);
    SharedNotebookWrapper & operator=(const SharedNotebookWrapper & other);
    virtual ~SharedNotebookWrapper() final override;

private:
    virtual const evernote::edam::SharedNotebook & GetEnSharedNotebook() const final override;
    virtual evernote::edam::SharedNotebook & GetEnSharedNotebook() final override;

    virtual QTextStream & Print(QTextStream & strm) const final override;

    evernote::edam::SharedNotebook m_enSharedNotebook;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_WRAPPER_H
