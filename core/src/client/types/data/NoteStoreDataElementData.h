#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__NOTE_STORE_DATA_ELEMENT_DATA_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__NOTE_STORE_DATA_ELEMENT_DATA_H

#include "LocalStorageDataElementData.h"

namespace qute_note {

class NoteStoreDataElementData : public LocalStorageDataElementData
{
public:
    NoteStoreDataElementData();
    virtual ~NoteStoreDataElementData();

    NoteStoreDataElementData(const NoteStoreDataElementData & other);
    NoteStoreDataElementData(NoteStoreDataElementData && other);

    bool    m_isDirty;

private:
    NoteStoreDataElementData & operator=(const NoteStoreDataElementData & other) Q_DECL_DELETE;
    NoteStoreDataElementData & operator=(NoteStoreDataElementData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__NOTE_STORE_DATA_ELEMENT_DATA_H
