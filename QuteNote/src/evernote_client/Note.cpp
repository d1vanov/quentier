#include "NoteImpl.h"

namespace qute_note {

Note::Note() :
    m_pImpl(new NoteImpl)
{}

}
