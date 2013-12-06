#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <ctime>
#include "../tools/TypeWithError.h"
#include "SynchronizedDataElement.h"
#include <QScopedPointer>

namespace qute_note {

class Guid;
class Resource;
class ResourceMetadata;
class Notebook;
class Tag;

class NotePrivate;
class Note final: public TypeWithError,
                  public SynchronizedDataElement
{
public:
    Note(const Notebook & notebook);
    Note(const Note & other);
    Note & operator =(const Note & other);
    virtual ~Note();

    virtual bool isEmpty() const;

    const QString title() const;
    void setTitle(const QString & title);

    /**
     * @return content of the note in ENML format
     */
    const QString content() const;
    void setContent(const QString & content);

    time_t createdTimestamp() const;
    time_t updatedTimestamp() const;

    /**
     * @return timestamp of the date to which the note refers to
     */
    time_t subjectDateTimestamp() const;

    /**
     * @return latitude of the location the note refers to
     */
    double latitude() const;
    void setLatitude(const double latitude);

    /**
     * @return longitude of the location the note refers to
     */
    double longitude() const;
    void setLongitude(const double longitude);

    /**
     * @return altitude of the location the note refers to
     */
    double altitude() const;
    void setAltitude(const double altitude);

    /**
     * @return true if location markers are valid for this note, false otherwise
     */
    bool hasValidLocation() const;

    const Guid notebookGuid() const;

    /**
     * @return name of author of the note
     */
    const QString author() const;
    void setAuthor(const QString & author);

    /**
     * @brief As Evernote API reference says, source is "the method that the note was added
     * to the account, if the note wasn't directly authored in an Evernote desktop client."
     */
    const QString source() const;
    void setSource(const QString & source);

    const QString sourceUrl() const;
    void setSourceUrl(const QString & sourceUrl);

    const QString sourceApplication() const;
    void setSourceApplication(const QString & sourceApplication);

    bool hasAttachedResources() const;
    std::size_t numAttachedResources() const;
    const Resource * getResourceByIndex(const std::size_t index) const;
    bool addResource(const Resource & resource, QString & errorMessage);

    bool labeledByAnyTag() const;
    std::size_t numTags() const;
    const Tag * getTagByIndex(const std::size_t index) const;
    bool addTag(const Tag & tag, QString & errorMessage);

    bool isDeleted() const;
    void deleteNote();
    void undeleteNote();

    virtual QTextStream & Print(QTextStream & strm) const;

private:
    Note() = delete;

    const QScopedPointer<NotePrivate> d_ptr;
    Q_DECLARE_PRIVATE(Note)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
