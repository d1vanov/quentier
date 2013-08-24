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

    int64_t updateSequenceNumber() const;
    void setUpdateSequenceNumber(const int64_t usn);

    bool isLocallyModified() const;
    void setLocallyModified();
    void dropLocallyModified();

    const Guid & guid() const;
    void assignGuid(const std::string & guid);

    virtual bool isEmpty() const;

private:
    int64_t m_updateSequenceNumber;
    bool    m_isLocallyModified;
    std::unique_ptr<Guid> m_pGuid;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__TYPES__SYNCHRONIZED_DATA_ELEMENT_H
