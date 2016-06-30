#ifndef LIB_QUENTIER_TYPES_DATA_SHARED_NOTEBOOK_WRAPPER_DATA_H
#define LIB_QUENTIER_TYPES_DATA_SHARED_NOTEBOOK_WRAPPER_DATA_H

#include <QEverCloud.h>
#include <QSharedData>

namespace quentier {

class SharedNotebookWrapperData : public QSharedData
{
public:
    SharedNotebookWrapperData();
    SharedNotebookWrapperData(const SharedNotebookWrapperData & other);
    SharedNotebookWrapperData(SharedNotebookWrapperData && other);
    SharedNotebookWrapperData(const qevercloud::SharedNotebook & other);
    SharedNotebookWrapperData(qevercloud::SharedNotebook && other);
    virtual ~SharedNotebookWrapperData();

    qevercloud::SharedNotebook    m_qecSharedNotebook;

private:
    SharedNotebookWrapperData & operator=(const SharedNotebookWrapperData & other) Q_DECL_DELETE;
    SharedNotebookWrapperData & operator=(SharedNotebookWrapperData && other) Q_DECL_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_SHARED_NOTEBOOK_WRAPPER_DATA_H
