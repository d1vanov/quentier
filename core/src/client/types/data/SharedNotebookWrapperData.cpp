#include "SharedNotebookWrapperData.h"

namespace qute_note {

SharedNotebookWrapperData::SharedNotebookWrapperData() :
    m_qecSharedNotebook()
{}

SharedNotebookWrapperData::SharedNotebookWrapperData(const SharedNotebookWrapperData & other) :
    m_qecSharedNotebook(other.m_qecSharedNotebook)
{}

SharedNotebookWrapperData::SharedNotebookWrapperData(SharedNotebookWrapperData && other) :
    m_qecSharedNotebook(std::move(other.m_qecSharedNotebook))
{}

SharedNotebookWrapperData::SharedNotebookWrapperData(const qevercloud::SharedNotebook & other) :
    m_qecSharedNotebook(other)
{}

SharedNotebookWrapperData::SharedNotebookWrapperData(qevercloud::SharedNotebook && other) :
    m_qecSharedNotebook(std::move(other))
{}

SharedNotebookWrapperData::~SharedNotebookWrapperData()
{}

} // namespace qute_note
