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

    bool isLocallyModified() const;
    void setLocallyModified();
    void dropLocallyModified();

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
    bool          m_isLocallyModified;
    std::unique_ptr<Guid> m_pGuid;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H
