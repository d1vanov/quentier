#ifndef __QUTE_NOTE__CLIENT__LOCAL_STORAGE__NOTE_SEARCH_QUERY_H
#define __QUTE_NOTE__CLIENT__LOCAL_STORAGE__NOTE_SEARCH_QUERY_H

#include <tools/Printable.h>

namespace qute_note {

class NoteSearchQueryPrivate;
class QUTE_NOTE_EXPORT NoteSearchQuery : public Printable
{
public:
    NoteSearchQuery();
    NoteSearchQuery(const NoteSearchQuery & other) = delete;
    NoteSearchQuery(NoteSearchQuery && other);
    NoteSearchQuery & operator=(const NoteSearchQuery & other) = delete;
    NoteSearchQuery & operator=(NoteSearchQuery && other);
    ~NoteSearchQuery();

    bool isEmpty() const;

    /**
     * Returns the original non-parsed query string
     */
    const QString queryString() const;

    bool setQueryString(const QString & queryString, QString & error);

    /**
     * If query string has "notebook:<notebook name>" scope modifier,
     * this method returns the name of the notebook, otherwise it returns
     * empty string
     */
    const QString notebookModifier() const;

    bool hasAnyModifier() const;

    const QStringList & tagNames() const;
    const QStringList & negatedTagNames() const;
    bool hasAnyTag() const;

    const QStringList & titleNames() const;
    const QStringList & negatedTitleNames() const;
    bool hasAnyTitle() const;

    const QVector<qint64> & creationTimestamps() const;
    const QVector<qint64> & negatedCreationTimestamps() const;
    bool hasAnyCreationTimestamp() const;

    const QVector<qint64> & modificationTimestamps() const;
    const QVector<qint64> & negatedModificationTimestamps() const;
    bool hasAnyModificationTimestamp() const;

    const QStringList & resourceMimeTypes() const;
    const QStringList & negatedResourceMimeTypes() const;
    bool hasAnyResourceMimeType() const;

    const QVector<qint64> & subjectDateTimestamps() const;
    const QVector<qint64> & negatedSubjectDateTimestamps() const;
    bool hasAnySubjectDateTimestamp() const;

    const QVector<double> & latitudes() const;
    const QVector<double> & negatedLatitudes() const;
    bool hasAnyLatitude() const;

    const QVector<double> & longitudes() const;
    const QVector<double> & negatedLongitudes() const;
    bool hasAnyLongitude() const;

    const QVector<double> & altitudes() const;
    const QVector<double> & negatedAltitudes() const;
    bool hasAnyAltitude() const;

    const QStringList & authors() const;
    const QStringList & negatedAuthors() const;
    bool hasAnyAuthor() const;

    const QStringList & sources() const;
    const QStringList & negatedSources() const;
    bool hasAnySource() const;

    const QStringList & sourceApplications() const;
    const QStringList & negatedSourceApplications() const;
    bool hasAnySourceApplication() const;

    const QStringList & contentClasses() const;
    const QStringList & negatedContentClasses() const;
    bool hasAnyContentClass() const;

    const QStringList & placeNames() const;
    const QStringList & negatedPlaceNames() const;
    bool hasAnyPlaceName() const;

    const QStringList & applicationData() const;
    const QStringList & negatedApplicationData() const;
    bool hasAnyApplicationData() const;

    const QStringList & recognitionTypes() const;
    const QStringList & negatedRecognitionTypes() const;
    bool hasAnyRecognitionType() const;

    const QVector<qint64> & reminderOrders() const;
    const QVector<qint64> & negatedReminderOrders() const;
    bool hasAnyReminderOrder() const;

    const QVector<qint64> & reminderTimes() const;
    const QVector<qint64> & negatedReminderTimes() const;
    bool hasAnyReminderTime() const;

    const QVector<qint64> & reminderDoneTimes() const;
    const QVector<qint64> & negatedReminderDoneTimes() const;
    bool hasAnyReminderDoneTime() const;

    bool hasUnfinishedToDo() const;
    bool hasNegatedUnfinishedToDo() const;

    bool hasFinishedToDo() const;
    bool hasNegatedFinishedToDo() const;

    bool hasAnyToDo() const;

    bool hasEncryption() const;

    const QStringList & contentSearchTerms() const;
    const QStringList & negatedContentSearchTerms() const;
    bool hasAnyContentSearchTerms() const;

private:
    QTextStream & Print(QTextStream & strm) const;

    NoteSearchQueryPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(NoteSearchQuery)
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__NOTE_SEARCH_QUERY_H
