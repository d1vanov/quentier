#include "Tag.h"
#include "Note.h"
#include "Resource.h"
#include "Notebook.h"
#include "../evernote_client_private/Location.h"
#include <QTranslator>
#include <QDateTime>

namespace qute_note {

class NotePrivate
{
public:
    NotePrivate(const Notebook & notebook);
    NotePrivate(const NotePrivate & other);
    NotePrivate & operator=(const NotePrivate & other);

    bool addResource(const Resource & resource, QString & errorMessage);
    bool addTag(const Tag & tag, QString & errorMessage);

    Guid     m_notebookGuid;
    QString  m_title;
    QString  m_content;
    QString  m_author;
    std::vector<Resource> m_resources;
    std::vector<Tag>      m_tags;
    time_t   m_createdTimestamp;
    time_t   m_updatedTimestamp;
    time_t   m_subjectDateTimestamp;
    Location m_location;
    QString  m_source;
    QString  m_sourceUrl;
    QString  m_sourceApplication;
    bool     m_isDeleted;

private:
    void initializeTimestamps();

    void updateTimestamp();
};

Note::Note(const Notebook & notebook) :
    d_ptr(new NotePrivate(notebook))
{}

Note::Note(const Note & other) :
    d_ptr(new NotePrivate(*(other.d_func())))
{}

Note & Note::operator =(const Note & other)
{
    if (this != &other) {
        *(d_func()) = *(other.d_func());
    }

    return *this;
}

Note::~Note()
{}

bool Note::isEmpty() const
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

const QString Note::title() const
{
    Q_D(const Note);
    return d->m_title;
}

const QString Note::content() const
{
    Q_D(const Note);
    return d->m_content;
}

time_t Note::createdTimestamp() const
{
    Q_D(const Note);
    return d->m_createdTimestamp;
}

time_t Note::updatedTimestamp() const
{
    Q_D(const Note);
    return d->m_updatedTimestamp;
}

time_t Note::subjectDateTimestamp() const
{
    Q_D(const Note);
    return d->m_subjectDateTimestamp;
}

double Note::latitude() const
{
    Q_D(const Note);
    return d->m_location.latitude();
}

void Note::setLatitude(const double latitude)
{
    Q_D(Note);
    d->m_location.setLatitude(latitude);
}

double Note::longitude() const
{
    Q_D(const Note);
    return d->m_location.longitude();
}

void Note::setLongitude(const double longitude)
{
    Q_D(Note);
    d->m_location.setLongitude(longitude);
}

double Note::altitude() const
{
    Q_D(const Note);
    return d->m_location.altitude();
}

void Note::setAltitude(const double altitude)
{
    Q_D(Note);
    d->m_location.setAltitude(altitude);
}

bool Note::hasValidLocation() const
{
    Q_D(const Note);
    return d->m_location.isValid();
}

const Guid Note::notebookGuid() const
{
    Q_D(const Note);
    return d->m_notebookGuid;
}

const QString Note::author() const
{
    Q_D(const Note);
    return d->m_author;
}

const QString Note::source() const
{
    Q_D(const Note);
    return d->m_source;
}

void Note::setSource(const QString & source)
{
    Q_D(Note);
    d->m_source = source;
}

const QString Note::sourceUrl() const
{
    Q_D(const Note);
    return d->m_sourceUrl;
}

void Note::setSourceUrl(const QString & sourceUrl)
{
    Q_D(Note);
    d->m_sourceUrl = sourceUrl;
}

const QString Note::sourceApplication() const
{
    Q_D(const Note);
    return d->m_sourceApplication;
}

void Note::setSourceApplication(const QString & sourceApplication)
{
    Q_D(Note);
    d->m_sourceApplication = sourceApplication;
}

bool Note::hasAttachedResources() const
{
    Q_D(const Note);
    return !(d->m_resources.empty());
}

std::size_t Note::numAttachedResources() const
{
    if (!hasAttachedResources()) {
        return static_cast<std::size_t>(0);
    }
    else {
        Q_D(const Note);
        return d->m_resources.size();
    }
}

const Resource * Note::getResourceByIndex(const size_t index) const
{
    std::size_t numResources = numAttachedResources();
    if (numResources == 0) {
        return nullptr;
    }
    else if (index >= numResources) {
        return nullptr;
    }
    else {
        Q_D(const Note);
        return &(d->m_resources.at(index));
    }
}

bool Note::addResource(const Resource & resource, QString & errorMessage)
{
    Q_D(Note);
    return d->addResource(resource, errorMessage);
}

bool Note::labeledByAnyTag() const
{
    Q_D(const Note);
    return !(d->m_tags.empty());
}

std::size_t Note::numTags() const
{
    Q_D(const Note);
    return d->m_tags.size();
}

const Tag * Note::getTagByIndex(const std::size_t index) const
{
    std::size_t nTags = numTags();
    if (nTags == 0) {
        return nullptr;
    }
    else if (index >= nTags) {
        return nullptr;
    }
    else {
        Q_D(const Note);
        return &(d->m_tags.at(index));
    }
}

bool Note::addTag(const Tag & tag, QString & errorMessage)
{
    Q_D(Note);
    return d->addTag(tag, errorMessage);
}

bool Note::isDeleted() const
{
    Q_D(const Note);
    return d->m_isDeleted;
}

void Note::deleteNote()
{
    Q_D(Note);
    d->m_isDeleted = true;
}

void Note::undeleteNote()
{
    Q_D(Note);
    d->m_isDeleted = false;
}

QTextStream & Note::Print(QTextStream & strm) const
{
    SynchronizedDataElement::Print(strm);

    if (HasError()) {
        strm << "Error: " << GetError() << ".\n";
    }

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

    return strm;
}

NotePrivate::NotePrivate(const Notebook &notebook) :
    m_notebookGuid(notebook.guid()),
    m_isDeleted(false)
{
    initializeTimestamps();
    m_subjectDateTimestamp = m_createdTimestamp;
}

NotePrivate::NotePrivate(const NotePrivate &other) :
    m_notebookGuid(other.m_notebookGuid),
    m_title(other.m_title),
    m_content(other.m_content),
    m_author(other.m_author),
    m_resources(other.m_resources),
    m_tags(other.m_tags),
    m_subjectDateTimestamp(other.m_subjectDateTimestamp),
    m_location(other.m_location),
    m_source(other.m_source),
    m_sourceUrl(other.m_sourceUrl),
    m_sourceApplication(other.m_sourceApplication),
    m_isDeleted(other.m_isDeleted)
{
    initializeTimestamps();
}

NotePrivate & NotePrivate::operator=(const NotePrivate & other)
{
    if (this != &other) {
        m_notebookGuid = other.m_notebookGuid;
        m_title   = other.m_title;
        m_content = other.m_content;
        m_author  = other.m_author;
        m_resources = other.m_resources;
        m_tags = other.m_tags;
        m_subjectDateTimestamp = other.m_subjectDateTimestamp;
        m_location = other.m_location;
        m_source = other.m_source;
        m_sourceUrl = other.m_sourceUrl;
        m_sourceApplication = other.m_sourceApplication;

        updateTimestamp();
    }

    return *this;
}

bool NotePrivate::addResource(const Resource & resource, QString & errorMessage)
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

bool NotePrivate::addTag(const Tag & tag, QString & errorMessage)
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

void NotePrivate::initializeTimestamps()
{
    m_createdTimestamp = static_cast<time_t>(QDateTime::currentMSecsSinceEpoch());
    m_updatedTimestamp = m_createdTimestamp;
}

void NotePrivate::updateTimestamp()
{
    m_updatedTimestamp = static_cast<time_t>(QDateTime::currentMSecsSinceEpoch());
}

}
