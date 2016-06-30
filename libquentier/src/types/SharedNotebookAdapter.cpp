#include <quentier/types/SharedNotebookAdapter.h>
#include <quentier/exception/SharedNotebookAdapterAccessException.h>
#include <quentier/utility/QuentierCheckPtr.h>

namespace quentier {

SharedNotebookAdapter::SharedNotebookAdapter(qevercloud::SharedNotebook & sharedNotebook) :
    ISharedNotebook(),
    m_pEnSharedNotebook(&sharedNotebook),
    m_isConst(false)
{}

SharedNotebookAdapter & SharedNotebookAdapter::operator=(const SharedNotebookAdapter & other)
{
    if (this != &other) {
        m_pEnSharedNotebook = other.m_pEnSharedNotebook;
        m_isConst = other.m_isConst;
    }

    return *this;
}

SharedNotebookAdapter & SharedNotebookAdapter::operator=(SharedNotebookAdapter && other)
{
    m_pEnSharedNotebook = std::move(other.m_pEnSharedNotebook);
    m_isConst = std::move(other.m_isConst);
    return *this;
}

SharedNotebookAdapter::SharedNotebookAdapter(const qevercloud::SharedNotebook & sharedNotebook) :
    ISharedNotebook(),
    m_pEnSharedNotebook(const_cast<qevercloud::SharedNotebook*>(&sharedNotebook)),
    m_isConst(true)
{}

SharedNotebookAdapter::SharedNotebookAdapter(const SharedNotebookAdapter & other) :
    ISharedNotebook(other),
    m_pEnSharedNotebook(other.m_pEnSharedNotebook),
    m_isConst(other.m_isConst)
{}

SharedNotebookAdapter::SharedNotebookAdapter(SharedNotebookAdapter && other) :
    ISharedNotebook(std::move(other)),
    m_pEnSharedNotebook(other.m_pEnSharedNotebook),
    m_isConst(other.m_isConst)
{}

SharedNotebookAdapter::~SharedNotebookAdapter()
{}

const qevercloud::SharedNotebook & SharedNotebookAdapter::GetEnSharedNotebook() const
{
    QUENTIER_CHECK_PTR(m_pEnSharedNotebook, "Null pointer to external SharedNotebook in SharedNotebookAdapter");
    return *m_pEnSharedNotebook;
}

qevercloud::SharedNotebook & SharedNotebookAdapter::GetEnSharedNotebook()
{
    if (m_isConst) {
        throw SharedNotebookAdapterAccessException("Attempt to access non-const reference "
                                                   "to SharedNotebook from constant SharedNotebookAdapter");
    }

    QUENTIER_CHECK_PTR(m_pEnSharedNotebook, "Null pointer to external SharedNotebook in SharedNotebookAdapter");
    return *m_pEnSharedNotebook;
}

QTextStream & SharedNotebookAdapter::print(QTextStream & strm) const
{
    strm << "SharedNotebookAdapter: { \n";
    ISharedNotebook::print(strm);
    strm << "}; \n";

    return strm;
}

} // namespace quentier
