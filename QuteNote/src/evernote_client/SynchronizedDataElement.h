#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H

#include <cstdint>
#include <memory>

namespace qute_note {

class Guid;

class SynchronizedDataElement
{
public:
    SynchronizedDataElement();
    SynchronizedDataElement(const SynchronizedDataElement & other);
    SynchronizedDataElement & operator=(const SynchronizedDataElement & other);
    virtual ~SynchronizedDataElement();

    unsigned int updateSequenceNumber() const;
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

    const Guid guid() const;
    void assignGuid(const std::string & guid);

    virtual bool isEmpty() const;

    /**
     * @brief operator < - compares two SynchronizedDataElement objects via their guid,
     * in order to make it possible to have sets of synchronized data elements
     * @param other - the other object for comparison
     * @return true if current object's guid is less than the other one's, false otherwise
     */
    bool operator<(const SynchronizedDataElement & other);

private:
    unsigned int  m_updateSequenceNumber;
    bool          m_isDirty;
    std::unique_ptr<Guid> m_pGuid;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H
