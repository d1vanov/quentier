#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H

#include <memory>
#include <cstdint>
#include "types/TypeWithError.h"
#include "types/SynchronizedDataElement.h"

namespace qute_note {

class Guid;

class Note: public TypeWithError,
            public SynchronizedDataElement
{
public:
    Note();

    virtual bool isEmpty() const;

private:
    class NoteImpl;
    std::unique_ptr<NoteImpl> m_pImpl;
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
