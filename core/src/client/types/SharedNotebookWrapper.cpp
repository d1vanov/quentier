#include "SharedNotebookWrapper.h"

namespace qute_note {

SharedNotebookWrapper::SharedNotebookWrapper(const qevercloud::SharedNotebook & other) :
    ISharedNotebook(),
    m_qecSharedNotebook(other)
{}

SharedNotebookWrapper::SharedNotebookWrapper(qevercloud::SharedNotebook && other) :
    ISharedNotebook(),
    m_qecSharedNotebook(std::move(other))
{}

SharedNotebookWrapper::~SharedNotebookWrapper()
{}

const qevercloud::SharedNotebook & SharedNotebookWrapper::GetEnSharedNotebook() const
{
    return m_qecSharedNotebook;
}

qevercloud::SharedNotebook & SharedNotebookWrapper::GetEnSharedNotebook()
{
    return m_qecSharedNotebook;
}

QTextStream & SharedNotebookWrapper::Print(QTextStream &strm) const
{
    strm << "SharedNotebookWrapper: { \n";
    ISharedNotebook::Print(strm);
    strm << "}; \n";

    return strm;
}

} // namespace qute_note
