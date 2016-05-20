#ifndef LIB_QUTE_NOTE_TYPES_DATA_NOTEBOOK_DATA_H
#define LIB_QUTE_NOTE_TYPES_DATA_NOTEBOOK_DATA_H

#include "DataElementWithShortcutData.h"
#include <QEverCloud.h>

namespace qute_note {

class NotebookData: public DataElementWithShortcutData
{
public:
    NotebookData();
    NotebookData(const NotebookData & other);
    NotebookData(NotebookData && other);
    NotebookData(const qevercloud::Notebook & other);
    NotebookData(qevercloud::Notebook && other);

    virtual ~NotebookData();

    bool checkParameters(QString & errorDescription) const;

    bool operator==(const NotebookData & other) const;
    bool operator!=(const NotebookData & other) const;

    qevercloud::Notebook            m_qecNotebook;
    bool                            m_isLastUsed;
    qevercloud::Optional<QString>   m_linkedNotebookGuid;

private:
    NotebookData & operator=(const NotebookData & other) Q_DECL_DELETE;
    NotebookData & operator=(NotebookData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_TYPES_DATA_NOTEBOOK_DATA_H
