#ifndef __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_ADAPTER_H
#define __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_ADAPTER_H

#include "ISharedNotebook.h"
#include <QEverCloud.h>

namespace qute_note {

/**
 * @brief The SharedNotebookAdapter class uses reference to external qevercloud::SharedNotebook
 * and adapts its interface to that of ISharedNotebook. The instances of this class
 * should be used only within the same scope as the referenced external
 * qevercloud::SharedNotebook object, otherwise it is possible to run into undefined behaviour.
 * SharedNotebookAdapter class is aware of constness of external object it references,
 * it would throw SharedNotebookAdapterAccessException exception in attempts to use
 * referenced const object in non-const context
 *
 * @see SharedNotebookAdapterAccessException
 */
class QUTE_NOTE_EXPORT SharedNotebookAdapter final: public ISharedNotebook
{
public:
    SharedNotebookAdapter(qevercloud::SharedNotebook & sharedNotebook);
    SharedNotebookAdapter(const qevercloud::SharedNotebook & sharedNotebook);
    SharedNotebookAdapter(const SharedNotebookAdapter & other);
    SharedNotebookAdapter(SharedNotebookAdapter && other);
    SharedNotebookAdapter & operator=(const SharedNotebookAdapter & other);
    SharedNotebookAdapter & operator=(SharedNotebookAdapter && other);

    virtual ~SharedNotebookAdapter() final override;

    virtual const qevercloud::SharedNotebook & GetEnSharedNotebook() const final override;
    virtual qevercloud::SharedNotebook & GetEnSharedNotebook() final override;

private:
    virtual QTextStream & Print(QTextStream & strm) const final override;

    SharedNotebookAdapter() = delete;

    qevercloud::SharedNotebook * m_pEnSharedNotebook;
    bool m_isConst;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__SHARED_NOTEBOOK_ADAPTER_H
