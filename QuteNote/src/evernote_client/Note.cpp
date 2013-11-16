#include "../evernote_client_private/NoteImpl.h"
#include "../evernote_client_private/TagImpl.h"
#include "Resource.h"
#include "Notebook.h"
#include "../tools/QuteNoteCheckPtr.h"
#include <QTranslator>

#define CHECK_PIMPL() QUTE_NOTE_CHECK_PTR(m_pImpl, QObject::tr("Null pointer to NoteImpl class"));

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
    if (m_pImpl != nullptr)
    {
        bool result = SynchronizedDataElement::isEmpty();
        if (result) {
            return true;
        }

        if (notebookGuid().isEmpty()) {
            return true;
        }

        // NOTE: should not check for emptiness of title or text:
        // notes with these ones empty are completely legal for Evernote service
        return false;
    }
    else {
        return true;
    }

}

time_t Note::createdTimestamp() const
{
    CHECK_PIMPL()
    return m_pImpl->createdTimestamp();
}

time_t Note::updatedTimestamp() const
{
    CHECK_PIMPL()
    return m_pImpl->updatedTimestamp();
}

const Guid Note::notebookGuid() const
{
    if (m_pImpl != nullptr) {
        return m_pImpl->notebookGuid();
    }
    else {
        return Guid();
    }
}

bool Note::hasAttachedResources() const
{
    CHECK_PIMPL()

    return !(m_pImpl->resources().empty());
}

size_t Note::numAttachedResources() const
{
    CHECK_PIMPL()

    if (!hasAttachedResources())
    {
        return static_cast<size_t>(0);
    }
    else
    {
        return m_pImpl->resources().size();
    }
}

const Resource * Note::getResourceByIndex(const size_t index) const
{
    CHECK_PIMPL()

    size_t numResources = numAttachedResources();
    if (numResources == 0) {
        return nullptr;
    }
    else if (index >= numResources) {
        return nullptr;
    }
    else {
        return &(m_pImpl->resources().at(index));
    }
}

bool Note::addResource(const Resource & resource, QString & errorMessage)
{
    CHECK_PIMPL()
    return m_pImpl->addResource(resource, errorMessage);
}

void Note::getResourcesMetadata(std::vector<ResourceMetadata> & resourcesMetadata) const
{
    CHECK_PIMPL()
    m_pImpl->getResourcesMetadata(resourcesMetadata);
}

bool Note::labeledByAnyTag() const
{
    CHECK_PIMPL()
    const std::vector<Tag> & tags = m_pImpl->tags();
    return (!tags.empty());
}

size_t Note::numTags() const
{
    CHECK_PIMPL()
    const std::vector<Tag> & tags = m_pImpl->tags();
    return tags.size();
}

const Tag * Note::getTagByIndex(const size_t index)
{
    CHECK_PIMPL()

    size_t nTags = numTags();
    if (nTags == 0) {
        return nullptr;
    }
    else if (index >= nTags) {
        return nullptr;
    }
    else {
        return &(m_pImpl->tags().at(index));
    }
}

bool Note::addTag(const Tag & tag, QString & errorMessage)
{
    CHECK_PIMPL()
    return m_pImpl->addTag(tag, errorMessage);
}

}

#undef CHECK_PIMPL
