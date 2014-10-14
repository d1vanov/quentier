#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__NOTEBOOK_DATA_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__NOTEBOOK_DATA_H

#include "DataElementWithShortcutData.h"
#include "SynchronizableDataElementData.h"
#include <QEverCloud.h>

namespace qute_note {

class NotebookData: public DataElementWithShortcutData,
                    public SynchronizableDataElementData
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

    qevercloud::Notebook m_qecNotebook;
    bool   m_isLocal;
    bool   m_isLastUsed;

private:
    NotebookData & operator=(const NotebookData & other) = delete;
    NotebookData & operator=(NotebookData && other) = delete;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__NOTEBOOK_DATA_H
