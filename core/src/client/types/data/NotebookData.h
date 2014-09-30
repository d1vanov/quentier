#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__NOTEBOOK_DATA_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__NOTEBOOK_DATA_H

#include "NoteStoreDataElementData.h"
#include <QEverCloud.h>

namespace qute_note {

class NotebookData : public NoteStoreDataElementData
{
public:
    NotebookData();
    NotebookData(const NotebookData & other);
    NotebookData(NotebookData && other);
    NotebookData(const qevercloud::Notebook & other);
    NotebookData(qevercloud::Notebook && other);
    NotebookData & operator=(const NotebookData & other);
    NotebookData & operator=(NotebookData && other);
    NotebookData & operator=(const qevercloud::Notebook & other);
    NotebookData & operator=(qevercloud::Notebook && other);
    virtual ~NotebookData();

    bool checkParameters(QString & errorDescription) const;

    bool operator==(const NotebookData & other) const;
    bool operator!=(const NotebookData & other) const;

    qevercloud::Notebook m_qecNotebook;
    bool   m_isLocal;
    bool   m_isLastUsed;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__NOTEBOOK_DATA_H
