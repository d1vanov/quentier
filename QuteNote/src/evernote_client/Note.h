#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H

#include <memory>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <ctime>
#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"

namespace qute_note {

class Guid;
class Resource;

class Note: public TypeWithError,
            public SynchronizedDataElement
{
public:
    Note();
    Note(const Note & other);
    Note & operator =(const Note & other);

    virtual bool isEmpty() const;

    std::string title() const;
    void setTitle(const std::string & title);

    /**
     * @brief content - returns content of the note in ENML format
     * @return
     */
    std::string content() const;
    void setContent(const std::string & content);

    time_t createdTimestamp() const;
    time_t updatedTimestamp() const;
    bool deletedTimestamp(time_t & timestamp, const char *& errorMessage) const;

    bool isActive() const;

    const Guid notebookGuid() const;

    bool hasAttachedResources() const;
    size_t numAttachedResources() const;
    // TODO: think how to implement getting the resources metadata conveniently
    // void getResources(std::vector<Resource> & resources) const;

private:
    class NoteImpl;
    std::unique_ptr<NoteImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
