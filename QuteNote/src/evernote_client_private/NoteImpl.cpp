#include "NoteImpl.h"
#include "TagImpl.h"
#include "../evernote_client/Notebook.h"
#include <QDateTime>

namespace qute_note {

Note::NoteImpl::NoteImpl(const Notebook & notebook) :
    m_notebookGuid(notebook.guid()),
    m_title(),
    m_content(),
    m_resources(),
    m_tags(),
    m_createdTimestamp(),
    m_updatedTimestamp(),
    m_subjectDateTimestamp(),
    m_location()
{
    initializeTimestamps();
    m_subjectDateTimestamp = m_createdTimestamp;
}

Note::NoteImpl::NoteImpl(const NoteImpl & other) :
    m_notebookGuid(other.m_notebookGuid),
    m_title(other.m_title),
    m_content(other.m_content),
    m_resources(other.m_resources),
    m_tags(other.m_tags),
    m_createdTimestamp(),
    m_updatedTimestamp(),
    m_subjectDateTimestamp(other.m_subjectDateTimestamp),
    m_location(other.m_location)
{
    initializeTimestamps();
}

Note::NoteImpl & Note::NoteImpl::operator=(const NoteImpl & other)
{
    if (this != &other)
    {
        m_notebookGuid = other.m_notebookGuid;
        m_title = other.m_title;
        m_content = other.m_content;
        m_resources = other.m_resources;
        m_tags = other.m_tags;
        m_subjectDateTimestamp = other.m_subjectDateTimestamp;
        m_location = other.m_location;

        updateTimestamp();
    }

    return *this;
}

const QString & Note::NoteImpl::title() const
{
    return m_title;
}

void Note::NoteImpl::setTitle(const QString & title)
{
    m_title = title;
    updateTimestamp();
}

const QString & Note::NoteImpl::content() const
{
    return m_content;
}

void Note::NoteImpl::setContent(const QString & content)
{
    m_content = content;
    updateTimestamp();
}

time_t Note::NoteImpl::createdTimestamp() const
{
    return m_createdTimestamp;
}

time_t Note::NoteImpl::updatedTimestamp() const
{
    return m_updatedTimestamp;
}

time_t Note::NoteImpl::subjectDateTimestamp() const
{
    return m_subjectDateTimestamp;
}

const Location & Note::NoteImpl::location() const
{
    return m_location;
}

Location & Note::NoteImpl::location()
{
    return m_location;
}

const std::vector<Resource> & Note::NoteImpl::resources() const
{
    return m_resources;
}

void Note::NoteImpl::getResourcesMetadata(std::vector<ResourceMetadata> & resourcesMetadata) const
{
    size_t numResources = m_resources.size();
    resourcesMetadata.clear();
    resourcesMetadata.reserve(numResources);
    for(size_t i = 0; i < numResources; ++i) {
        resourcesMetadata.push_back(ResourceMetadata(m_resources[i]));
    }
}

bool Note::NoteImpl::addResource(const Resource & resource, QString & errorMessage)
{
    auto it = std::find_if(m_resources.cbegin(), m_resources.cend(),
                           [&resource](const Resource & other)
                           { return (resource.guid() == other.guid()); });
    if (it != m_resources.cend()) {
        errorMessage = QObject::tr("This resource has already been added to the note");
        return false;
    }

    m_resources.push_back(resource);
    return true;
}

const std::vector<Tag> & Note::NoteImpl::tags() const
{
    return m_tags;
}

bool Note::NoteImpl::addTag(const Tag & tag, QString & errorMessage)
{
    auto it = std::find_if(m_tags.cbegin(), m_tags.cend(),
                           [&tag](const Tag & other)
                           { return (tag.guid() == other.guid()); });
    if (it != m_tags.cend()) {
        errorMessage = QObject::tr("The note has already been labeled by this tag");
        return false;
    }

    if (tag.isEmpty()) {
        errorMessage = QObject::tr("The tag attempted to be added is empty");
        return false;
    }

    if (tag.HasError()) {
        errorMessage = QObject::tr("The tag attempted to be added has error: ");
        errorMessage.append(tag.GetError());
        return false;
    }

    m_tags.push_back(tag);
    return true;
}

const Guid & Note::NoteImpl::notebookGuid() const
{
    return m_notebookGuid;
}

void Note::NoteImpl::initializeTimestamps()
{
    m_createdTimestamp = static_cast<time_t>(QDateTime::currentMSecsSinceEpoch());
    m_updatedTimestamp = m_createdTimestamp;
}

void Note::NoteImpl::updateTimestamp()
{
    m_updatedTimestamp = static_cast<time_t>(QDateTime::currentMSecsSinceEpoch());
}

}
