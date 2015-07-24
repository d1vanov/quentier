#include "NoteSearchQuery.h"
#include "NoteSearchQuery_p.h"

namespace qute_note {

NoteSearchQuery::NoteSearchQuery() :
    d_ptr(new NoteSearchQueryPrivate)
{}

NoteSearchQuery::~NoteSearchQuery()
{
    if (d_ptr) {
        delete d_ptr;
    }
}

bool NoteSearchQuery::isEmpty() const
{
    Q_D(const NoteSearchQuery);
    return d->m_queryString.isEmpty();
}

const QString NoteSearchQuery::queryString() const
{
    Q_D(const NoteSearchQuery);
    return d->m_queryString;
}

bool NoteSearchQuery::setQueryString(const QString & queryString, QString & error)
{
    Q_D(NoteSearchQuery);
    d->clear();
    return d->parseQueryString(queryString, error);
}

const QString NoteSearchQuery::notebookModifier() const
{
    Q_D(const NoteSearchQuery);
    return d->m_notebookModifier;
}

bool NoteSearchQuery::hasAnyModifier() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyModifier;
}

const QStringList & NoteSearchQuery::tagNames() const
{
    Q_D(const NoteSearchQuery);
    return d->m_tagNames;
}

const QStringList & NoteSearchQuery::negatedTagNames() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedTagNames;
}

bool NoteSearchQuery::hasAnyTag() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyTag;
}

bool NoteSearchQuery::hasNegatedAnyTag() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyTag;
}

const QStringList & NoteSearchQuery::titleNames() const
{
    Q_D(const NoteSearchQuery);
    return d->m_titleNames;
}

const QStringList & NoteSearchQuery::negatedTitleNames() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedTitleNames;
}

bool NoteSearchQuery::hasAnyTitleName() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyTitleName;
}

bool NoteSearchQuery::hasNegatedAnyTitleName() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyTitleName;
}

const QVector<qint64> & NoteSearchQuery::creationTimestamps() const
{
    Q_D(const NoteSearchQuery);
    return d->m_creationTimestamps;
}

const QVector<qint64> & NoteSearchQuery::negatedCreationTimestamps() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedCreationTimestamps;
}

bool NoteSearchQuery::hasAnyCreationTimestamp() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyCreationTimestamp;
}

bool NoteSearchQuery::hasNegatedAnyCreationTimestamp() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyCreationTimestamp;
}

const QVector<qint64> & NoteSearchQuery::modificationTimestamps() const
{
    Q_D(const NoteSearchQuery);
    return d->m_modificationTimestamps;
}

const QVector<qint64> & NoteSearchQuery::negatedModificationTimestamps() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedModificationTimestamps;
}

bool NoteSearchQuery::hasAnyModificationTimestamp() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyModificationTimestamp;
}

bool NoteSearchQuery::hasNegatedAnyModificationTimestamp() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyModificationTimestamp;
}

const QStringList & NoteSearchQuery::resourceMimeTypes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_resourceMimeTypes;
}

const QStringList & NoteSearchQuery::negatedResourceMimeTypes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedResourceMimeTypes;
}

bool NoteSearchQuery::hasAnyResourceMimeType() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyResourceMimeType;
}

bool NoteSearchQuery::hasNegatedAnyResourceMimeType() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyResourceMimeType;
}

const QVector<qint64> & NoteSearchQuery::subjectDateTimestamps() const
{
    Q_D(const NoteSearchQuery);
    return d->m_subjectDateTimestamps;
}

const QVector<qint64> & NoteSearchQuery::negatedSubjectDateTimestamps() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedSubjectDateTimestamps;
}

bool NoteSearchQuery::hasAnySubjectDateTimestamp() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnySubjectDateTimestamp;
}

bool NoteSearchQuery::hasNegatedAnySubjectDateTimestamp() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnySubjectDateTimestamp;
}

const QVector<double> & NoteSearchQuery::latitudes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_latitudes;
}

const QVector<double> & NoteSearchQuery::negatedLatitudes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedLatitudes;
}

bool NoteSearchQuery::hasAnyLatitude() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyLatitude;
}

bool NoteSearchQuery::hasNegatedAnyLatitude() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyLatitude;
}

const QVector<double> & NoteSearchQuery::longitudes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_longitudes;
}

const QVector<double> & NoteSearchQuery::negatedLongitudes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedLongitudes;
}

bool NoteSearchQuery::hasAnyLongitude() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyLongitude;
}

bool NoteSearchQuery::hasNegatedAnyLongitude() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyLongitude;
}

const QVector<double> & NoteSearchQuery::altitudes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_altitudes;
}

const QVector<double> & NoteSearchQuery::negatedAltitudes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedAltitudes;
}

bool NoteSearchQuery::hasAnyAltitude() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyAltitude;
}

bool NoteSearchQuery::hasNegatedAnyAltitude() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyAltitude;
}

const QStringList & NoteSearchQuery::authors() const
{
    Q_D(const NoteSearchQuery);
    return d->m_authors;
}

const QStringList & NoteSearchQuery::negatedAuthors() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedAuthors;
}

bool NoteSearchQuery::hasAnyAuthor() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyAuthor;
}

bool NoteSearchQuery::hasNegatedAnyAuthor() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyAuthor;
}

const QStringList & NoteSearchQuery::sources() const
{
    Q_D(const NoteSearchQuery);
    return d->m_sources;
}

const QStringList & NoteSearchQuery::negatedSources() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedSources;
}

bool NoteSearchQuery::hasAnySource() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnySource;
}

bool NoteSearchQuery::hasNegatedAnySource() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnySource;
}

const QStringList & NoteSearchQuery::sourceApplications() const
{
    Q_D(const NoteSearchQuery);
    return d->m_sourceApplications;
}

const QStringList & NoteSearchQuery::negatedSourceApplications() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedSourceApplications;
}

bool NoteSearchQuery::hasAnySourceApplication() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnySourceApplication;
}

bool NoteSearchQuery::hasNegatedAnySourceApplication() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnySourceApplication;
}

const QStringList & NoteSearchQuery::contentClasses() const
{
    Q_D(const NoteSearchQuery);
    return d->m_contentClasses;
}

const QStringList & NoteSearchQuery::negatedContentClasses() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedContentClasses;
}

bool NoteSearchQuery::hasAnyContentClass() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyContentClass;
}

bool NoteSearchQuery::hasNegatedAnyContentClass() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyContentClass;
}

const QStringList & NoteSearchQuery::placeNames() const
{
    Q_D(const NoteSearchQuery);
    return d->m_placeNames;
}

const QStringList & NoteSearchQuery::negatedPlaceNames() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedPlaceNames;
}

bool NoteSearchQuery::hasAnyPlaceName() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyPlaceName;
}

bool NoteSearchQuery::hasNegatedAnyPlaceName() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyPlaceName;
}

const QStringList & NoteSearchQuery::applicationData() const
{
    Q_D(const NoteSearchQuery);
    return d->m_applicationData;
}

const QStringList & NoteSearchQuery::negatedApplicationData() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedApplicationData;
}

bool NoteSearchQuery::hasAnyApplicationData() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyApplicationData;
}

bool NoteSearchQuery::hasNegatedAnyApplicationData() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyApplicationData;
}

const QStringList & NoteSearchQuery::recognitionTypes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_recognitionTypes;
}

const QStringList & NoteSearchQuery::negatedRecognitionTypes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedRecognitionTypes;
}

bool NoteSearchQuery::hasAnyRecognitionType() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyRecognitionType;
}

bool NoteSearchQuery::hasNegatedAnyRecognitionType() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyRecognitionType;
}

bool NoteSearchQuery::hasAnyReminderOrder() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyReminderOrder;
}

bool NoteSearchQuery::hasNegatedAnyReminderOrder() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyReminderOrder;
}

const QVector<qint64> & NoteSearchQuery::reminderOrders() const
{
    Q_D(const NoteSearchQuery);
    return d->m_reminderOrders;
}

const QVector<qint64> & NoteSearchQuery::negatedReminderOrders() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedReminderOrders;
}

const QVector<qint64> & NoteSearchQuery::reminderTimes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_reminderTimes;
}

const QVector<qint64> & NoteSearchQuery::negatedReminderTimes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedReminderTimes;
}

bool NoteSearchQuery::hasAnyReminderTime() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyReminderTime;
}

bool NoteSearchQuery::hasNegatedAnyReminderTime() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyReminderTime;
}

const QVector<qint64> & NoteSearchQuery::reminderDoneTimes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_reminderDoneTimes;
}

const QVector<qint64> & NoteSearchQuery::negatedReminderDoneTimes() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedReminderDoneTimes;
}

bool NoteSearchQuery::hasAnyReminderDoneTime() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyReminderDoneTime;
}

bool NoteSearchQuery::hasNegatedAnyReminderDoneTime() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyReminderDoneTime;
}

bool NoteSearchQuery::hasUnfinishedToDo() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasUnfinishedToDo;
}

bool NoteSearchQuery::hasNegatedUnfinishedToDo() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedUnfinishedToDo;
}

bool NoteSearchQuery::hasFinishedToDo() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasFinishedToDo;
}

bool NoteSearchQuery::hasNegatedFinishedToDo() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedFinishedToDo;
}

bool NoteSearchQuery::hasAnyToDo() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasAnyToDo;
}

bool NoteSearchQuery::hasNegatedAnyToDo() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedAnyToDo;
}

bool NoteSearchQuery::hasEncryption() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasEncryption;
}

bool NoteSearchQuery::hasNegatedEncryption() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasNegatedEncryption;
}

const QStringList & NoteSearchQuery::contentSearchTerms() const
{
    Q_D(const NoteSearchQuery);
    return d->m_contentSearchTerms;
}

const QStringList & NoteSearchQuery::negatedContentSearchTerms() const
{
    Q_D(const NoteSearchQuery);
    return d->m_negatedContentSearchTerms;
}

bool NoteSearchQuery::isMatcheable() const
{
    Q_D(const NoteSearchQuery);
    return d->isMatcheable();
}

QTextStream & NoteSearchQuery::Print(QTextStream & strm) const
{
    Q_D(const NoteSearchQuery);
    return d->Print(strm);
}

} // namespace qute_note
