#include "../evernote_client_private/NoteImpl.h"
#include "../evernote_client/Resource.h"

namespace qute_note {

Note::Note() :
    m_pImpl(new NoteImpl)
{}

Note::Note(const Note & other) :
    m_pImpl(nullptr)
{
    if (!other.isEmpty()) {
        m_pImpl.reset(new NoteImpl(*(other.m_pImpl)));
    }
    else {
        m_pImpl.reset(new NoteImpl);
    }
}

Note & Note::operator =(const Note & other)
{
    if (this != &other) {
        if (!other.isEmpty()) {
            *m_pImpl = *(other.m_pImpl);
        }
    }

    return *this;
}

bool Note::isEmpty() const
{
    if (m_pImpl != nullptr) {
        // TODO: implement
        return true;
    }
    else {
        return true;
    }

}

const Resource * Note::getResourceByIndex(const size_t) const
{
    // TODO: implement
    return nullptr;
}

}
