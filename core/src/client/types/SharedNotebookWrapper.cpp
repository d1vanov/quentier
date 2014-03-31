#include "SharedNotebookWrapper.h"

namespace qute_note {

SharedNotebookWrapper::SharedNotebookWrapper() :
    ISharedNotebook(),
    m_enSharedNotebook()
{}

SharedNotebookWrapper::SharedNotebookWrapper(const evernote::edam::SharedNotebook & sharedNotebook) :
    ISharedNotebook(),
    m_enSharedNotebook(sharedNotebook)
{}

SharedNotebookWrapper::SharedNotebookWrapper(const SharedNotebookWrapper & other) :
    ISharedNotebook(other),
    m_enSharedNotebook(other.m_enSharedNotebook)
{}

SharedNotebookWrapper & SharedNotebookWrapper::operator=(const SharedNotebookWrapper & other)
{
    if (this != &other) {
        ISharedNotebook::operator=(other);
        m_enSharedNotebook = other.m_enSharedNotebook;
    }

    return *this;
}

SharedNotebookWrapper::~SharedNotebookWrapper()
{}

const evernote::edam::SharedNotebook & SharedNotebookWrapper::GetEnSharedNotebook() const
{
    return m_enSharedNotebook;
}

evernote::edam::SharedNotebook & SharedNotebookWrapper::GetEnSharedNotebook()
{
    return m_enSharedNotebook;
}

QTextStream & SharedNotebookWrapper::Print(QTextStream &strm) const
{
    strm << "SharedNotebookWrapper: { \n";
    ISharedNotebook::Print(strm);
    strm << "}; \n";

    return strm;
}

} // namespace qute_note
