#ifndef LIB_QUTE_NOTE_TYPES_DATA_NOTE_STORE_DATA_ELEMENT_DATA_H
#define LIB_QUTE_NOTE_TYPES_DATA_NOTE_STORE_DATA_ELEMENT_DATA_H

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
    bool    m_isLocal;

private:
    NoteStoreDataElementData & operator=(const NoteStoreDataElementData & other) Q_DECL_DELETE;
    NoteStoreDataElementData & operator=(NoteStoreDataElementData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_TYPES_DATA_NOTE_STORE_DATA_ELEMENT_DATA_H
