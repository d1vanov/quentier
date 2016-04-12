#ifndef __LIB_QUTE_NOTE__TYPES__SHARED_NOTEBOOK_ADAPTER_H
#define __LIB_QUTE_NOTE__TYPES__SHARED_NOTEBOOK_ADAPTER_H

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
class QUTE_NOTE_EXPORT SharedNotebookAdapter: public ISharedNotebook
{
public:
    SharedNotebookAdapter(qevercloud::SharedNotebook & sharedNotebook);
    SharedNotebookAdapter(const qevercloud::SharedNotebook & sharedNotebook);
    SharedNotebookAdapter(const SharedNotebookAdapter & other);
    SharedNotebookAdapter(SharedNotebookAdapter && other);
    SharedNotebookAdapter & operator=(const SharedNotebookAdapter & other);
    SharedNotebookAdapter & operator=(SharedNotebookAdapter && other);

    virtual ~SharedNotebookAdapter();

    virtual const qevercloud::SharedNotebook & GetEnSharedNotebook() const Q_DECL_OVERRIDE;
    virtual qevercloud::SharedNotebook & GetEnSharedNotebook() Q_DECL_OVERRIDE;

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    SharedNotebookAdapter() Q_DECL_DELETE;

    qevercloud::SharedNotebook * m_pEnSharedNotebook;
    bool m_isConst;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__SHARED_NOTEBOOK_ADAPTER_H
