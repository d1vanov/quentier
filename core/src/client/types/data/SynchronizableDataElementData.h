#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SYNCHRONIZABLE_DATA_ELEMENT_DATA_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SYNCHRONIZABLE_DATA_ELEMENT_DATA_H

#include "NoteStoreDataElementData.h"

namespace qute_note {

class SynchronizableDataElementData: public virtual NoteStoreDataElementData
{
public:
    SynchronizableDataElementData();
    virtual ~SynchronizableDataElementData();

    SynchronizableDataElementData(const SynchronizableDataElementData & other);
    SynchronizableDataElementData(SynchronizableDataElementData && other);

    bool m_synchronizable;

private:
    SynchronizableDataElementData & operator=(const SynchronizableDataElementData & other) Q_DECL_DELETE;
    SynchronizableDataElementData & operator=(SynchronizableDataElementData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__SYNCHRONIZABLE_DATA_ELEMENT_DATA_H
