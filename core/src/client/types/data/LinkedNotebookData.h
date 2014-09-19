#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__LINKED_NOTEBOOK_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__LINKED_NOTEBOOK_H

#include <QEverCloud.h>
#include <QSharedData>

namespace qute_note {

class LinkedNotebookData : public QSharedData
{
public:
    LinkedNotebookData();
    LinkedNotebookData(const LinkedNotebookData & other);
    LinkedNotebookData(LinkedNotebookData && other);
    LinkedNotebookData(const qevercloud::LinkedNotebook & other);
    LinkedNotebookData(qevercloud::LinkedNotebook && other);
    LinkedNotebookData & operator=(const LinkedNotebookData & other);
    LinkedNotebookData & operator=(LinkedNotebookData && other);
    LinkedNotebookData & operator=(const qevercloud::LinkedNotebook & other);
    virtual ~LinkedNotebookData();

    void clear();
    bool checkParameters(QString & errorDescription) const;

    bool operator==(const LinkedNotebookData & other) const;
    bool operator!=(const LinkedNotebookData & other) const;

    qevercloud::LinkedNotebook    m_qecLinkedNotebook;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__LINKED_NOTEBOOK_H
