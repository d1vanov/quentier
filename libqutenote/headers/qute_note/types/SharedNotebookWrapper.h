#ifndef __LIB_QUTE_NOTE__TYPES__SHARED_NOTEBOOK_WRAPPER_H
#define __LIB_QUTE_NOTE__TYPES__SHARED_NOTEBOOK_WRAPPER_H

#include "ISharedNotebook.h"
#include <QEverCloud.h>
#include <QSharedDataPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(SharedNotebookWrapperData)

/**
 * @brief The SharedNotebookWrapper class creates and manages its own instance of
 * qevercloud::SharedNotebook object
 */
class QUTE_NOTE_EXPORT SharedNotebookWrapper: public ISharedNotebook
{
public:
    SharedNotebookWrapper();
    SharedNotebookWrapper(const SharedNotebookWrapper & other);
    SharedNotebookWrapper(SharedNotebookWrapper && other);
    SharedNotebookWrapper & operator=(const SharedNotebookWrapper & other);
    SharedNotebookWrapper & operator=(SharedNotebookWrapper && other);

    SharedNotebookWrapper(const qevercloud::SharedNotebook & other);
    SharedNotebookWrapper(qevercloud::SharedNotebook && other);

    virtual ~SharedNotebookWrapper();

    virtual const qevercloud::SharedNotebook & GetEnSharedNotebook() const Q_DECL_OVERRIDE;
    virtual qevercloud::SharedNotebook & GetEnSharedNotebook() Q_DECL_OVERRIDE;

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<SharedNotebookWrapperData> d;
};

} // namespace qute_note

Q_DECLARE_METATYPE(qute_note::SharedNotebookWrapper)

#endif // __LIB_QUTE_NOTE__TYPES__SHARED_NOTEBOOK_WRAPPER_H
