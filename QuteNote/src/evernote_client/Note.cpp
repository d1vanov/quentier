#include "../evernote_client_private/NoteImpl.h"
#include "Resource.h"
#include "Notebook.h"
#include "../tools/QuteNoteCheckPtr.h"
#include <QTranslator>

namespace qute_note {

Note::Note(const Notebook & notebook) :
    m_pImpl(new NoteImpl(notebook))
{}

Note::Note(const Note & other) :
    m_pImpl(nullptr)
{
    QUTE_NOTE_CHECK_PTR(other.m_pImpl, QObject::tr("Detected attempt to create a note from empty one!"));
    m_pImpl.reset(new NoteImpl(*(other.m_pImpl)));
}

Note & Note::operator =(const Note & other)
{
    if ((this != &other) && !other.isEmpty()) {
        *m_pImpl = *(other.m_pImpl);
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

const Guid Note::notebookGuid() const
{
    if (m_pImpl != nullptr) {
        return m_pImpl->notebookGuid();
    }
}

const Resource * Note::getResourceByIndex(const size_t) const
{
    // TODO: implement
    return nullptr;
}

}
