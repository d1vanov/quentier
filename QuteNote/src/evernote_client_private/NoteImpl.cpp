#include "NoteImpl.h"
#include "../evernote_client/Notebook.h"
#include <QDateTime>

namespace qute_note {

Note::NoteImpl::NoteImpl(const Notebook & notebook) :
    m_notebookGuid(notebook.guid()),
    m_title(),
    m_content(),
    m_resources(),
    m_createdTimestamp(),
    m_updatedTimestamp()
{
    initializeTimestamps();
}

Note::NoteImpl::NoteImpl(const NoteImpl & other) :
    m_notebookGuid(other.m_notebookGuid),
    m_title(other.m_title),
    m_content(other.m_content),
    m_resources(other.m_resources),
    m_createdTimestamp(),
    m_updatedTimestamp()
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
    auto it = std::find_if(m_resources.begin(), m_resources.end(),
                           [&resource](const Resource & other)
                           { return (resource.guid() == other.guid()); });
    if (it != m_resources.end()) {
        errorMessage = QObject::tr("This resource has already been added to the note");
        return false;
    }

    m_resources.push_back(resource);
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
