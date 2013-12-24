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

QT_FORWARD_DECLARE_CLASS(Guid)
QT_FORWARD_DECLARE_CLASS(ResourceMetadata)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(Tag)
QT_FORWARD_DECLARE_CLASS(INoteStore)

QT_FORWARD_DECLARE_CLASS(NotePrivate)
class Note final: public TypeWithError,
                  public SynchronizedDataElement
{
public:
    static Note Create(const Notebook & notebook, INoteStore & noteStore);

    Note(const Note & other);
    Note & operator =(const Note & other);
    virtual ~Note() override;

    virtual bool isEmpty() const override;

    const QString title() const;
    void setTitle(const QString & title);

    /**
     * @return content of the note in ENML format
     */
    const QString content() const;
    void setContent(const QString & content);

    time_t createdTimestamp() const;
    void setCreatedTimestamp(const time_t timestamp);

    time_t updatedTimestamp() const;
    void setUpdatedTimestamp(const time_t timestamp);

    /**
     * @return timestamp of the date to which the note refers to
     */
    time_t subjectDateTimestamp() const;
    void setSubjectDateTimestamp(const time_t timestamp);

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
    void getResourcesMetadata(std::vector<ResourceMetadata> & resourcesMetadata) const;
    bool addResourceMetadata(const ResourceMetadata & resourceMetadata,
                             QString & errorMessage);

    bool labeledByTag(const Tag & tag) const;
    bool labeledByAnyTag() const;
    std::size_t numTags() const;
    const Tag * getTagByIndex(const std::size_t index) const;
    bool addTag(const Tag & tag, QString & errorMessage);

    virtual QTextStream & Print(QTextStream & strm) const override;

private:
    Note(const Notebook & notebook);
    Note() = delete;

    const QScopedPointer<NotePrivate> d_ptr;
    Q_DECLARE_PRIVATE(Note)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
