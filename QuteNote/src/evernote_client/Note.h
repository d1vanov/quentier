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
    static Note Create(const Guid & notebookGuid, INoteStore & noteStore);

    Note(const Note & other);
    Note & operator =(const Note & other);
    virtual ~Note() override;

    virtual bool isEmpty() const override;

    const Guid notebookGuid() const;

    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString content READ content WRITE setContent)
    Q_PROPERTY(time_t createdTimestamp READ createdTimestamp WRITE setCreatedTimestamp)
    Q_PROPERTY(time_t updatedTimestamp READ updatedTimestamp WRITE setUpdatedTimestamp)
    Q_PROPERTY(time_t subjectDateTimestamp READ subjectDateTimestamp WRITE setSubjectDateTimestamp)
    Q_PROPERTY(double latitude READ latitude WRITE setLatitude)
    Q_PROPERTY(double longitude READ longitude WRITE setLongitude)
    Q_PROPERTY(double altitude READ altitude WRITE setAltitude)
    Q_PROPERTY(QString author READ author WRITE setAuthor)
    Q_PROPERTY(QString source READ source WRITE setSource)
    Q_PROPERTY(QString sourceUrl READ sourceUrl WRITE setSourceUrl)
    Q_PROPERTY(QString sourceApplicartion READ sourceApplication WRITE setSourceApplication)
    Q_PROPERTY(std::vector<Guid> resourceGuids READ resourceGuids WRITE setResourceGuids)
    Q_PROPERTY(std::vector<Guid> tagGuids READ tagGuids WRITE setTagGuids)

    const QString title() const;
    void setTitle(const QString & title);

    const QString content() const;
    void setContent(const QString & content);

    time_t createdTimestamp() const;
    void setCreatedTimestamp(const time_t timestamp);

    time_t updatedTimestamp() const;
    void setUpdatedTimestamp(const time_t timestamp);

    time_t subjectDateTimestamp() const;
    void setSubjectDateTimestamp(const time_t timestamp);

    double latitude() const;
    void setLatitude(const double latitude);

    double longitude() const;
    void setLongitude(const double longitude);

    double altitude() const;
    void setAltitude(const double altitude);

    const QString author() const;
    void setAuthor(const QString & author);

    const QString source() const;
    void setSource(const QString & source);

    const QString sourceUrl() const;
    void setSourceUrl(const QString & sourceUrl);

    const QString sourceApplication() const;
    void setSourceApplication(const QString & sourceApplication);

    const std::vector<Guid> & resourceGuids() const;
    void setResourceGuids(const std::vector<Guid> & resourceGuids);

    const std::vector<Guid> & tagGuids() const;
    void setTagGuids(const std::vector<Guid> & tagGuids);

    /**
     * @return true if location markers are valid for this note, false otherwise
     */
    bool hasValidLocation() const;

    bool hasAttachedResources() const;
    std::size_t numAttachedResources() const;
    bool attachResource(const ResourceMetadata & resourceMetadata, QString & errorMessage);

    bool labeledByTag(const Tag & tag) const;
    bool labeledByAnyTag() const;
    std::size_t numTags() const;
    bool labelByTag(const Tag & tag, QString & errorMessage);

    virtual QTextStream & Print(QTextStream & strm) const override;

private:
    Note(const Guid & notebookGuid);
    Note() = delete;

    const QScopedPointer<NotePrivate> d_ptr;
    Q_DECLARE_PRIVATE(Note)
};

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__NOTE_H
