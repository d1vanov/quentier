#include "NoteImpl.h"

namespace qute_note {

Note::Note() :
    m_pImpl(new NoteImpl)
{}

bool Note::isEmpty() const
{
    // TODO: implement
    return true;
}

}
