#ifndef __QUTE_NOTE__CLIENT__TYPES__NOTE_STORE_DATA_ELEMENT_H
#define __QUTE_NOTE__CLIENT__TYPES__NOTE_STORE_DATA_ELEMENT_H

#include "LocalStorageDataElement.h"
#include <tools/Printable.h>
#include <tools/TypeWithError.h>
#include <QtGlobal>
#include <QUuid>

namespace qute_note {

class QUTE_NOTE_EXPORT NoteStoreDataElement: public LocalStorageDataElement,
                                             public Printable,
                                             public TypeWithError
{
public:
    NoteStoreDataElement();
    virtual ~NoteStoreDataElement();

    virtual void clear() = 0;

    virtual bool hasGuid() const = 0;
    virtual const QString & guid() const = 0;
    virtual void setGuid(const QString & guid) = 0;

    virtual bool hasUpdateSequenceNumber() const = 0;
    virtual qint32 updateSequenceNumber() const = 0;
    virtual void setUpdateSequenceNumber(const qint32 usn) = 0;

    virtual bool checkParameters(QString & errorDescription) const = 0;

    bool isDirty() const;
    void setDirty(const bool isDirty);

protected:
    NoteStoreDataElement(const NoteStoreDataElement & other);
    NoteStoreDataElement(NoteStoreDataElement && other);
    NoteStoreDataElement & operator=(const NoteStoreDataElement & other);
    NoteStoreDataElement & operator=(NoteStoreDataElement && other);

    virtual QTextStream & Print(QTextStream & strm) const = 0;

private:
    bool   m_isDirty;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__NOTE_STORE_DATA_ELEMENT_H
