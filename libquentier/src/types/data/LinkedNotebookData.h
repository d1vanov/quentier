#ifndef LIB_QUENTIER_TYPES_DATA_LINKED_NOTEBOOK_DATA_H
#define LIB_QUENTIER_TYPES_DATA_LINKED_NOTEBOOK_DATA_H

#include <QSharedData>
#include <quentier/utility/QNLocalizedString.h>
#include <QEverCloud.h>

namespace quentier {

class LinkedNotebookData : public QSharedData
{
public:
    LinkedNotebookData();
    LinkedNotebookData(const LinkedNotebookData & other);
    LinkedNotebookData(LinkedNotebookData && other);
    LinkedNotebookData(const qevercloud::LinkedNotebook & other);
    LinkedNotebookData(qevercloud::LinkedNotebook && other);
    virtual ~LinkedNotebookData();

    void clear();
    bool checkParameters(QNLocalizedString & errorDescription) const;

    bool operator==(const LinkedNotebookData & other) const;
    bool operator!=(const LinkedNotebookData & other) const;

    qevercloud::LinkedNotebook    m_qecLinkedNotebook;
    bool                          m_isDirty;

private:
    LinkedNotebookData & operator=(const LinkedNotebookData & other) Q_DECL_DELETE;
    LinkedNotebookData & operator=(LinkedNotebookData && other) Q_DECL_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_LINKED_NOTEBOOK_DATA_H
