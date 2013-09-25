#include "NoteImpl.h"

namespace qute_note {

Note::Note() :
    m_pImpl(new NoteImpl)
{}

Note::Note(const Note & other) :
    m_pImpl(new NoteImpl(*(other.m_pImpl)))
{}

Note & Note::operator =(const Note & other)
{
    if (this != &other) {
        *m_pImpl = *(other.m_pImpl);
    }

    return *this;
}

bool Note::isEmpty() const
{
    // TODO: implement
    return true;
}

}
