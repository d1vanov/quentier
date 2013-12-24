#include "Tag.h"
#include "Note.h"
#include "ResourceMetadata.h"
#include "Notebook.h"
#include "INoteStore.h"
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

    bool addResourceMetadata(const ResourceMetadata & resourceMetadata, QString & errorMessage);
    bool addTag(const Tag & tag, QString & errorMessage);

    Guid     m_notebookGuid;
    QString  m_title;
    QString  m_content;
    QString  m_author;
    std::vector<ResourceMetadata>  m_resourcesMetadata;
    std::vector<Tag>  m_tags;
    time_t   m_createdTimestamp;
    time_t   m_updatedTimestamp;
    time_t   m_subjectDateTimestamp;
    Location m_location;
    QString  m_source;
    QString  m_sourceUrl;
    QString  m_sourceApplication;
};

Note Note::Create(const Notebook & notebook, INoteStore & noteStore)
{
    Note note(notebook);

    if (notebook.isEmpty()) {
        note.SetError("Notebook is empty");
        return std::move(note);
    }

    noteStore.CreateNote(note);
    return std::move(note);
}

Note::Note(const Note & other) :
    d_ptr(new NotePrivate(*(other.d_func())))
{}

Note & Note::operator =(const Note & other)
{
    if (this != &other) {
        if (other.d_func() != nullptr) {
            *(d_func()) = *(other.d_func());
        }
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

void Note::setTitle(const QString & title)
{
    Q_D(Note);
    d->m_title = title;
}

const QString Note::content() const
{
    Q_D(const Note);
    return d->m_content;
}

void Note::setContent(const QString & content)
{
    Q_D(Note);
    d->m_content = content;
}

time_t Note::createdTimestamp() const
{
    Q_D(const Note);
    return d->m_createdTimestamp;
}

void Note::setCreatedTimestamp(const time_t timestamp)
{
    Q_D(Note);
    d->m_createdTimestamp = timestamp;
}

time_t Note::updatedTimestamp() const
{
    Q_D(const Note);
    return d->m_updatedTimestamp;
}

void Note::setUpdatedTimestamp(const time_t timestamp)
{
    Q_D(Note);
    d->m_updatedTimestamp = timestamp;
}

time_t Note::subjectDateTimestamp() const
{
    Q_D(const Note);
    return d->m_subjectDateTimestamp;
}

void Note::setSubjectDateTimestamp(const time_t timestamp)
{
    Q_D(Note);
    d->m_subjectDateTimestamp = timestamp;
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
    return !(d->m_resourcesMetadata.empty());
}

std::size_t Note::numAttachedResources() const
{
    if (!hasAttachedResources()) {
        return static_cast<std::size_t>(0);
    }
    else {
        Q_D(const Note);
        return d->m_resourcesMetadata.size();
    }
}

void Note::getResourcesMetadata(std::vector<ResourceMetadata> & resourcesMetadata) const
{
    if (!hasAttachedResources()) {
        return;
    }

    Q_D(const Note);
    resourcesMetadata.assign(d->m_resourcesMetadata.cbegin(),
                             d->m_resourcesMetadata.cend());
}

bool Note::addResourceMetadata(const ResourceMetadata & resourceMetadata,
                               QString & errorMessage)
{
    Q_D(Note);
    return d->addResourceMetadata(resourceMetadata, errorMessage);
}

bool Note::labeledByTag(const Tag & tag) const
{
    if (tag.isEmpty()) {
        return false;
    }

    Q_D(const Note);
    if (std::find(d->m_tags.begin(), d->m_tags.end(), tag) == d->m_tags.end()) {
        return false;
    }
    else {
        return true;
    }
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
        strm << "Metadata of resources attached to note: " << "\n";
        std::size_t numResources = numAttachedResources();
        Q_D(const Note);
        for(std::size_t i = 0; i < numResources; ++i)
        {
            const ResourceMetadata & resourceMetadata = d->m_resourcesMetadata.at(i);
            strm << "Resource metadata #" << i << ": mime type: ";
            strm << resourceMetadata.mimeType() << ", MD5 hash: ";
            strm << resourceMetadata.dataHash() << ", number of bytes: ";
            strm << resourceMetadata.dataSize();

            std::size_t width = resourceMetadata.width();
            if (width != 0) {
                strm << ", display width: " << width;
            }

            std::size_t height = resourceMetadata.height();
            if (height != 0) {
                strm << ", display height: " << height;
            }

            strm << "\n";
        }
    }

    return strm;
}

Note::Note(const Notebook & notebook) :
    d_ptr(new NotePrivate(notebook))
{}

NotePrivate::NotePrivate(const Notebook &notebook) :
    m_notebookGuid(notebook.guid())
{}

NotePrivate::NotePrivate(const NotePrivate &other) :
    m_notebookGuid(other.m_notebookGuid),
    m_title(other.m_title),
    m_content(other.m_content),
    m_author(other.m_author),
    m_resourcesMetadata(other.m_resourcesMetadata),
    m_tags(other.m_tags),
    m_createdTimestamp(other.m_createdTimestamp),
    m_updatedTimestamp(other.m_updatedTimestamp),
    m_subjectDateTimestamp(other.m_subjectDateTimestamp),
    m_location(other.m_location),
    m_source(other.m_source),
    m_sourceUrl(other.m_sourceUrl),
    m_sourceApplication(other.m_sourceApplication)
{}

NotePrivate & NotePrivate::operator=(const NotePrivate & other)
{
    if (this != &other)
    {
        m_notebookGuid = other.m_notebookGuid;
        m_title   = other.m_title;
        m_content = other.m_content;
        m_author  = other.m_author;
        m_resourcesMetadata = other.m_resourcesMetadata;
        m_tags = other.m_tags;
        m_createdTimestamp = other.m_createdTimestamp;
        m_updatedTimestamp = other.m_updatedTimestamp;
        m_subjectDateTimestamp = other.m_subjectDateTimestamp;
        m_location = other.m_location;
        m_source = other.m_source;
        m_sourceUrl = other.m_sourceUrl;
        m_sourceApplication = other.m_sourceApplication;
    }

    return *this;
}

bool NotePrivate::addResourceMetadata(const ResourceMetadata & resourceMetadata,
                                      QString & errorMessage)
{
    auto it = std::find_if(m_resourcesMetadata.cbegin(), m_resourcesMetadata.cend(),
                           [&resourceMetadata](const ResourceMetadata & other)
                           { return (resourceMetadata.guid() == other.guid()); });
    if (it != m_resourcesMetadata.cend()) {
        errorMessage = QObject::tr("This resource has already been added to the note");
        return false;
    }

    m_resourcesMetadata.push_back(resourceMetadata);
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

}
