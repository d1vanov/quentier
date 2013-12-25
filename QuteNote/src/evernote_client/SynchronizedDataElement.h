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
     * @brief setDirty - mark the object as locally modified and needing synchronization
     * with Evernote online database
     */
    void setDirty();

    /**
     * @brief setSynchronized - mark the object as synchronized with Evernote online database
     */
    void setSynchronized();

    /**
     * @return true if the synchronizable object has never actually been synchronized
     * with remote Evernote service.
     */
    bool isLocal() const;

    /**
     * @brief isDeleted - determines whether data element has been marked as deleted
     * from local storage
     * @return true if object has been marked deleted in local storage, false otherwise
     */
    bool isDeleted() const;

    /**
     * @brief setDeleted - mark data element as deleted in local storage database
     */
    void setDeleted();

    /**
     * @brief undelete - mark data element as non-deleted in local storage database
     */
    void undelete();

    const Guid & guid() const;

    virtual bool isEmpty() const;

    /**
     * @brief operator < - compares two SynchronizedDataElement objects via their guid,
     * in order to make it possible to have sets of synchronized data elements
     * @param other - the other object for comparison
     * @return true if current object's guid is less than the other one's, false otherwise
     */
    bool operator<(const SynchronizedDataElement & other);

    virtual QTextStream & Print(QTextStream & strm) const;

    void assignGuid(const std::string & guid);
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
