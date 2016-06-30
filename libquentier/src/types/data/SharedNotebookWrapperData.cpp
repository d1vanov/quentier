#include "SharedNotebookWrapperData.h"

namespace quentier {

SharedNotebookWrapperData::SharedNotebookWrapperData() :
    QSharedData(),
    m_qecSharedNotebook()
{}

SharedNotebookWrapperData::SharedNotebookWrapperData(const SharedNotebookWrapperData & other) :
    QSharedData(other),
    m_qecSharedNotebook(other.m_qecSharedNotebook)
{}

SharedNotebookWrapperData::SharedNotebookWrapperData(SharedNotebookWrapperData && other) :
    QSharedData(std::move(other)),
    m_qecSharedNotebook(std::move(other.m_qecSharedNotebook))
{}

SharedNotebookWrapperData::SharedNotebookWrapperData(const qevercloud::SharedNotebook & other) :
    QSharedData(),
    m_qecSharedNotebook(other)
{}

SharedNotebookWrapperData::SharedNotebookWrapperData(qevercloud::SharedNotebook && other) :
    QSharedData(),
    m_qecSharedNotebook(std::move(other))
{}

SharedNotebookWrapperData::~SharedNotebookWrapperData()
{}

} // namespace quentier
