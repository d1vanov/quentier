#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SHARED_NOTEBOOK_WRAPPER_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SHARED_NOTEBOOK_WRAPPER_H

#include <QEverCloud.h>
#include <QSharedData>

namespace qute_note {

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

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SHARED_NOTEBOOK_WRAPPER_H
