#ifndef LIB_QUTE_NOTE_TYPES_DATA_LINKED_NOTEBOOK_DATA_H
#define LIB_QUTE_NOTE_TYPES_DATA_LINKED_NOTEBOOK_DATA_H

#include <QSharedData>
#include <QEverCloud.h>

namespace qute_note {

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
    bool checkParameters(QString & errorDescription) const;

    bool operator==(const LinkedNotebookData & other) const;
    bool operator!=(const LinkedNotebookData & other) const;

    qevercloud::LinkedNotebook    m_qecLinkedNotebook;
    bool                          m_isDirty;

private:
    LinkedNotebookData & operator=(const LinkedNotebookData & other) Q_DECL_DELETE;
    LinkedNotebookData & operator=(LinkedNotebookData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_TYPES_DATA_LINKED_NOTEBOOK_DATA_H
