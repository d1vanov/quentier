#include "Tag.h"
#include "Note.h"
#include "ResourceMetadata.h"
#include "Notebook.h"
#include "INoteStore.h"
#include "../evernote_client_private/Note_p.h"
#include <QTranslator>
#include <QDateTime>

namespace qute_note {

Note Note::Create(const Guid & notebookGuid, INoteStore & noteStore)
{
    Note note(notebookGuid);

    if (notebookGuid.isEmpty()) {
        note.SetError("Notebook guid is empty");
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

const Guid Note::notebookGuid() const
{
    Q_D(const Note);
    return d->m_notebookGuid;
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

const std::vector<Guid> & Note::resourceGuids() const
{
    Q_D(const Note);
    return d->m_resourceGuids;
}

void Note::setResourceGuids(const std::vector<Guid> & resourceGuids)
{
    Q_D(Note);
    d->m_resourceGuids = resourceGuids;
}

const std::vector<Guid> & Note::tagGuids() const
{
    Q_D(const Note);
    return d->m_tagGuids;
}

void Note::setTagGuids(const std::vector<Guid> & tagGuids)
{
    Q_D(Note);
    d->m_tagGuids = tagGuids;
}

bool Note::hasValidLocation() const
{
    Q_D(const Note);
    return d->m_location.isValid();
}

bool Note::hasAttachedResources() const
{
    Q_D(const Note);
    return !(d->m_resourceGuids.empty());
}

std::size_t Note::numAttachedResources() const
{
    if (!hasAttachedResources()) {
        return static_cast<std::size_t>(0);
    }
    else {
        Q_D(const Note);
        return d->m_resourceGuids.size();
    }
}

bool Note::attachResource(const ResourceMetadata & resourceMetadata,
                          QString & errorMessage)
{
    Q_D(Note);
    std::vector<Guid> & resourceGuids = d->m_resourceGuids;
    auto it = std::find_if(resourceGuids.cbegin(), resourceGuids.cend(),
                           [&resourceMetadata](const Guid & other)
                           { return (resourceMetadata.guid() == other); });
    if (it != resourceGuids.cend()) {
        errorMessage = QObject::tr("This resource has already been attached to the note");
        return false;
    }

    resourceGuids.push_back(resourceMetadata.guid());
    return true;
}

bool Note::labeledByTag(const Tag & tag) const
{
    if (tag.isEmpty()) {
        return false;
    }

    Q_D(const Note);
    const std::vector<Guid> & tagGuids = d->m_tagGuids;
    if (std::find(tagGuids.cbegin(), tagGuids.cend(), tag.guid()) == tagGuids.end()) {
        return false;
    }
    else {
        return true;
    }
}

bool Note::labeledByAnyTag() const
{
    Q_D(const Note);
    return !(d->m_tagGuids.empty());
}

std::size_t Note::numTags() const
{
    Q_D(const Note);
    return d->m_tagGuids.size();
}

bool Note::labelByTag(const Tag & tag, QString & errorMessage)
{
    Q_D(Note);
    std::vector<Guid> & tagGuids = d->m_tagGuids;
    auto it = std::find_if(tagGuids.cbegin(), tagGuids.cend(),
                           [&tag](const Guid & other)
                           { return (tag.guid() == other); });
    if (it != tagGuids.cend()) {
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

    tagGuids.push_back(tag.guid());
    return true;
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

    Q_D(const Note);

    if (hasAttachedResources())
    {
        strm << "Guids of resources attached to note: " << "\n";
        const std::vector<Guid> & resourceGuids = d->m_resourceGuids;
        std::size_t numResources = resourceGuids.size();
        for(std::size_t i = 0; i < numResources; ++i)
        {
            const Guid & resourceGuid = resourceGuids[i];

            strm << "Resource guid #" << i << ": " << resourceGuid << "\n";
        }
    }

    // TODO: print tag guids and other stuff

    return strm;
}

Note::Note(const Guid & notebookGuid) :
    d_ptr(new NotePrivate(notebookGuid))
{}

}
