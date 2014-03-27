#ifndef __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_ADAPTER_H
#define __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_ADAPTER_H

#include "ISharedNotebook.h"
#include <Types_types.h>

namespace qute_note {

class SharedNotebookAdapter final: public ISharedNotebook
{
public:
    SharedNotebookAdapter(evernote::edam::SharedNotebook & enSharedNotebook);
    SharedNotebookAdapter(const evernote::edam::SharedNotebook & enSharedNotebook);
    SharedNotebookAdapter(const SharedNotebookAdapter & other);
    virtual ~SharedNotebookAdapter() final override;

private:
    virtual const evernote::edam::SharedNotebook & GetEnSharedNotebook() const final override;
    virtual evernote::edam::SharedNotebook & GetEnSharedNotebook() final override;

    virtual QTextStream & Print(QTextStream & strm) const final override;

    SharedNotebookAdapter() = delete;
    SharedNotebookAdapter & operator=(const SharedNotebookAdapter & other) = delete;

    evernote::edam::SharedNotebook * m_pEnSharedNotebook;
    bool m_isConst;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_ADAPTER_H
