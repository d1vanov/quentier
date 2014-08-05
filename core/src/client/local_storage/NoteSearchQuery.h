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

    const QStringList & titleNames() const;
    const QStringList & negatedTitleNames() const;

    const QVector<qint64> & creationTimestamps() const;
    const QVector<qint64> & negatedCreationTimestamps() const;

    const QVector<qint64> & modificationTimestamps() const;
    const QVector<qint64> & negatedModificationTimestamps() const;

    const QStringList & resourceMimeTypes() const;
    const QStringList & negatedResourceMimeTypes() const;

    const QVector<qint64> & subjectDateTimestamps() const;
    const QVector<qint64> & negatedSubjectDateTimestamps() const;

    const QVector<double> & latitudes() const;
    const QVector<double> & negatedLatitudes() const;

    const QVector<double> & longitudes() const;
    const QVector<double> & negatedLongitudes() const;

    const QVector<double> & altitudes() const;
    const QVector<double> & negatedAltitudes() const;

    const QStringList & authors() const;
    const QStringList & negatedAuthors() const;

    const QStringList & sources() const;
    const QStringList & negatedSources() const;

    const QStringList & sourceApplications() const;
    const QStringList & negatedSourceApplications() const;

    const QStringList & contentClasses() const;
    const QStringList & negatedContentClasses() const;

    const QStringList & placeNames() const;
    const QStringList & negatedPlaceNames() const;

    const QStringList & applicationData() const;
    const QStringList & negatedApplicationData() const;

    const QStringList & recognitionTypes() const;
    const QStringList & negatedRecognitionTypes() const;

    // Method to detect whether some wildcard reminder order has been set
    bool hasReminderOrder() const;

    const QVector<qint64> & reminderOrders() const;
    const QVector<qint64> & negatedReminderOrders() const;

    const QVector<qint64> & reminderTimes() const;
    const QVector<qint64> & negatedReminderTimes() const;

    const QVector<qint64> & reminderDoneTimes() const;
    const QVector<qint64> & negatedReminderDoneTimes() const;

    bool hasUnfinishedToDo() const;
    bool hasNegatedUnfinishedToDo() const;

    bool hasFinishedToDo() const;
    bool hasNegatedFinishedToDo() const;

    bool hasAnyToDo() const;

    bool hasEncryption() const;

    const QStringList & contentSearchTerms() const;
    const QStringList & negatedContentSearchTerms() const;

private:
    QTextStream & Print(QTextStream & strm) const;

    NoteSearchQueryPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(NoteSearchQuery)
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__LOCAL_STORAGE__NOTE_SEARCH_QUERY_H
