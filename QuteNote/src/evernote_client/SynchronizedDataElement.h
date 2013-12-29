#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H

#include "Guid.h"

namespace qute_note {

class SynchronizedDataElement: public Printable
{
public:
    SynchronizedDataElement();
    SynchronizedDataElement(const SynchronizedDataElement & other);
    SynchronizedDataElement & operator=(const SynchronizedDataElement & other);
    SynchronizedDataElement(SynchronizedDataElement && other) = default;
    SynchronizedDataElement & operator=(SynchronizedDataElement && other) = default;
    virtual ~SynchronizedDataElement();

    unsigned int getUpdateSequenceNumber() const;
    void setUpdateSequenceNumber(const unsigned int usn);

    /**
     * @brief isDirty - tells the caller whether this synchronized data element
     * in its latest revision has been synchronized with Evernote's online database
     * @return true is object was not synchronized or has local modifications,
     * false otherwise
     */
    bool isDirty() const;

    /**
     * @brief setDirty - marks the object either locally modified and needing synchronization
     * with remote database or completely synchronized with remote database
     * @param dirty - if true, the object is marked locally modified, otherwise -
     * as synchronized with remote database.
     */
    void setDirty(const bool dirty);

    /**
     * @return true if the synchronizable object has never actually been synchronized
     * with remote service.
     */
    bool isLocal() const;
    /**
     * @brief setDirty - marks the object either local i.e. not yet synchronized
     * with remote database or remote i.e. synchronized with remote database at least once.
     * @param local - if true, the object is marked locally modified, otherwise -
     * as at least once synchronized with remote database.
     */
    void setLocal(const bool local);

    /**
     * @brief isDeleted - determines whether data element has been marked as deleted
     * from local storage
     * @return true if object has been marked deleted in local storage, false otherwise
     */
    bool isDeleted() const;

    /**
     * @brief setDeleted - mark data element as either deleted or not deleted
     * in local storage database
     * @param deleted - if true, the object would be marked as deleted, otherwise -
     * as not deleted
     */
    void setDeleted(const bool deleted);

    const Guid & guid() const;

    virtual void Clear() = 0;

    virtual bool isEmpty() const;

    /**
     * @brief operator < - compares two SynchronizedDataElement objects via their guid,
     * in order to make it possible to have sets of synchronized data elements
     * @param other - the other object for comparison
     * @return true if current object's guid is less than the other one's, false otherwise
     */
    bool operator<(const SynchronizedDataElement & other);

    virtual QTextStream & Print(QTextStream & strm) const;

protected:
    void assignGuid(const Guid & guid);

private:
    unsigned int  m_updateSequenceNumber;
    bool          m_isDirty;
    bool          m_isLocal;
    bool          m_isDeleted;
    Guid          m_guid;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H
