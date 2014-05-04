#include "NoteStoreDataElement.h"

namespace qute_note {

NoteStoreDataElement::NoteStoreDataElement() :
    LocalStorageDataElement(),
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
    LocalStorageDataElement(other),
    Printable(),
    TypeWithError(other),
    m_isDirty(other.m_isDirty)
{}

NoteStoreDataElement::NoteStoreDataElement(NoteStoreDataElement && other) :
    LocalStorageDataElement(std::move(other)),
    Printable(),
    TypeWithError(other),
    m_isDirty(std::move(other.m_isDirty))
{}

NoteStoreDataElement & NoteStoreDataElement::operator=(const NoteStoreDataElement & other)
{
    if (this != &other)
    {
        LocalStorageDataElement::operator=(other);
        TypeWithError::operator =(other);
        setDirty(other.m_isDirty);
    }

    return *this;
}

NoteStoreDataElement & NoteStoreDataElement::operator=(NoteStoreDataElement && other)
{
    LocalStorageDataElement::operator=(other);
    TypeWithError::operator=(other);
    setDirty(std::move(other.isDirty()));

    return *this;
}

} // namespace qute_note
