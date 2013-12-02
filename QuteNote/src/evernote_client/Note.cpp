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

const QString Note::title() const
{
    CHECK_PIMPL()
    return m_pImpl->title();
}

const QString Note::content() const
{
    CHECK_PIMPL()
    return m_pImpl->content();
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

time_t Note::subjectDateTimestamp() const
{
    CHECK_PIMPL()
    return m_pImpl->subjectDateTimestamp();
}

double Note::latitude() const
{
    CHECK_PIMPL()
    return m_pImpl->location().latitude();
}

void Note::setLatitude(const double latitude)
{
    CHECK_PIMPL()
    m_pImpl->location().setLatitude(latitude);
}

double Note::longitude() const
{
    CHECK_PIMPL()
    return m_pImpl->location().longitude();
}

void Note::setLongitude(const double longitude)
{
    CHECK_PIMPL()
    m_pImpl->location().setLongitude(longitude);
}

double Note::altitude() const
{
    CHECK_PIMPL()
    return m_pImpl->location().altitude();
}

void Note::setAltitude(const double altitude)
{
    CHECK_PIMPL()
    m_pImpl->location().setAltitude(altitude);
}

bool Note::hasValidLocationMarks() const
{
    CHECK_PIMPL()
    return m_pImpl->location().isValid();
}

const Guid Note::notebookGuid() const
{
    CHECK_PIMPL()
    return m_pImpl->notebookGuid();
}

const QString Note::author() const
{
    CHECK_PIMPL()
    return m_pImpl->author();
}

const QString Note::source() const
{
    CHECK_PIMPL()
    return m_pImpl->source();
}

void Note::setSource(const QString & source)
{
    CHECK_PIMPL()
    m_pImpl->setSource(source);
}

const QString Note::sourceUrl() const
{
    CHECK_PIMPL()
    return m_pImpl->sourceUrl();
}

void Note::setSourceUrl(const QString & sourceUrl)
{
    CHECK_PIMPL()
    return m_pImpl->setSourceUrl(sourceUrl);
}

const QString Note::sourceApplication() const
{
    CHECK_PIMPL()
    return m_pImpl->sourceApplication();
}

void Note::setSourceApplication(const QString & sourceApplication)
{
    CHECK_PIMPL()
    m_pImpl->setSourceApplication(sourceApplication);
}

bool Note::hasAttachedResources() const
{
    CHECK_PIMPL()
    return !(m_pImpl->resources().empty());
}

std::size_t Note::numAttachedResources() const
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

std::size_t Note::numTags() const
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

bool Note::isDeleted() const
{
    CHECK_PIMPL()
    return m_pImpl->isDeleted();
}

void Note::deleteNote()
{
    CHECK_PIMPL()
    m_pImpl->setDeletedFlag(true);
}

void Note::undeleteNote()
{
    CHECK_PIMPL()
    m_pImpl->setDeletedFlag(false);
}

QTextStream & Note::Print(QTextStream & strm) const
{
    SynchronizedDataElement::Print(strm);

    if (HasError()) {
        strm << "Error: " << GetError() << ".\n";
    }

    if (m_pImpl == nullptr) {
        strm << "WARNING: pointer to implementation is null!";
    }
    else
    {
        strm << "Notebook's guid: " << notebookGuid() << ";\n";
        strm << "Note title: " << title() << ";\n";
        strm << "Note's ENML content: {\n";
        strm << content() << "\n};\n";

        if (hasAttachedResources())
        {
            std::size_t numResources = numAttachedResources();
            for(std::size_t i = 0; i < numResources; ++i)
            {
                const Resource * pResource = getResourceByIndex(i);
                if (pResource == nullptr) {
                    strm << "WARNING: null resource for index " << static_cast<int>(i) << "\n";
                    continue;
                }
                // else TODO: print resource
            }
        }
    }

    return strm;
}

}

#undef CHECK_PIMPL
