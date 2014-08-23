#ifndef __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_WRAPPER_H
#define __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_WRAPPER_H

#include "ISharedNotebook.h"
#include <QEverCloud.h>

namespace qute_note {

/**
 * @brief The SharedNotebookWrapper class creates and manages its own instance of
 * qevercloud::SharedNotebook object
 */
class QUTE_NOTE_EXPORT SharedNotebookWrapper final: public ISharedNotebook
{
public:
    SharedNotebookWrapper() = default;
    SharedNotebookWrapper(const SharedNotebookWrapper & other) = default;
    SharedNotebookWrapper(SharedNotebookWrapper && other);
    SharedNotebookWrapper & operator=(const SharedNotebookWrapper & other) = default;
    SharedNotebookWrapper & operator=(SharedNotebookWrapper && other);

    SharedNotebookWrapper(const qevercloud::SharedNotebook & other);
    SharedNotebookWrapper(qevercloud::SharedNotebook && other);

    virtual ~SharedNotebookWrapper() final override;

    virtual const qevercloud::SharedNotebook & GetEnSharedNotebook() const final override;
    virtual qevercloud::SharedNotebook & GetEnSharedNotebook() final override;

private:
    virtual QTextStream & Print(QTextStream & strm) const final override;

    qevercloud::SharedNotebook m_qecSharedNotebook;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_WRAPPER_H
