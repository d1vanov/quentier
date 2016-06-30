#ifndef LIB_QUENTIER_TYPES_DATA_NOTEBOOK_DATA_H
#define LIB_QUENTIER_TYPES_DATA_NOTEBOOK_DATA_H

#include "FavoritableDataElementData.h"
#include <QEverCloud.h>

namespace quentier {

class NotebookData: public FavoritableDataElementData
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

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_NOTEBOOK_DATA_H
