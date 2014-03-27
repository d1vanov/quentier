#include "SharedNotebookAdapter.h"
#include "SharedNotebookAdapterAccessException.h"
#include <tools/QuteNoteCheckPtr.h>

namespace qute_note {

SharedNotebookAdapter::SharedNotebookAdapter(evernote::edam::SharedNotebook & enSharedNotebook) :
    ISharedNotebook(),
    m_pEnSharedNotebook(&enSharedNotebook),
    m_isConst(false)
{}

SharedNotebookAdapter::SharedNotebookAdapter(const evernote::edam::SharedNotebook & enSharedNotebook) :
    ISharedNotebook(),
    m_pEnSharedNotebook(const_cast<evernote::edam::SharedNotebook*>(&enSharedNotebook)),
    m_isConst(true)
{}

SharedNotebookAdapter::SharedNotebookAdapter(const SharedNotebookAdapter & other) :
    ISharedNotebook(other),
    m_pEnSharedNotebook(other.m_pEnSharedNotebook),
    m_isConst(other.m_isConst)
{}

SharedNotebookAdapter::~SharedNotebookAdapter()
{}

const evernote::edam::SharedNotebook & SharedNotebookAdapter::GetEnSharedNotebook() const
{
    QUTE_NOTE_CHECK_PTR(m_pEnSharedNotebook, "Null pointer to external SharedNotebook in SharedNotebookAdapter");
    return *m_pEnSharedNotebook;
}

evernote::edam::SharedNotebook & SharedNotebookAdapter::GetEnSharedNotebook()
{
    if (m_isConst) {
        throw SharedNotebookAdapterAccessException("Attempt to access non-const reference "
                                                   "to SharedNotebook from constant SharedNotebookAdapter");
    }

    QUTE_NOTE_CHECK_PTR(m_pEnSharedNotebook, "Null pointer to external SharedNotebook in SharedNotebookAdapter");
    return *m_pEnSharedNotebook;
}

QTextStream & SharedNotebookAdapter::Print(QTextStream & strm) const
{
    strm << "SharedNotebookAdapter: { \n";
    ISharedNotebook::Print(strm);
    strm << "}; \n";

    return strm;
}

} // namespace qute_note
