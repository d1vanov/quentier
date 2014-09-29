#include "NoteStoreDataElement.h"

namespace qute_note {

NoteStoreDataElement::NoteStoreDataElement() :
    Printable(),
    TypeWithError(),
    m_isDirty(true)
{}

NoteStoreDataElement::~NoteStoreDataElement()
{}

bool NoteStoreDataElement::isDirty() const
{
    return m_isDirty;
}

void NoteStoreDataElement::setDirty(const bool isDirty)
{
    m_isDirty = isDirty;
}

NoteStoreDataElement::NoteStoreDataElement(const NoteStoreDataElement & other) :
    Printable(),
    TypeWithError(other),
    m_isDirty(other.m_isDirty)
{}

NoteStoreDataElement::NoteStoreDataElement(NoteStoreDataElement && other) :
    Printable(),
    TypeWithError(std::move(other)),
    m_isDirty(std::move(other.m_isDirty))
{}

NoteStoreDataElement & NoteStoreDataElement::operator=(const NoteStoreDataElement & other)
{
    TypeWithError::operator =(other);

    if (this != std::addressof(other)) {
        setDirty(other.m_isDirty);
    }

    return *this;
}

NoteStoreDataElement & NoteStoreDataElement::operator=(NoteStoreDataElement && other)
{
    TypeWithError::operator=(other);
    setDirty(std::move(other.isDirty()));

    return *this;
}

} // namespace qute_note
