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

bool NoteSearchQuery::hasReminderOrder() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasReminderOrder;
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

bool NoteSearchQuery::hasEncryption() const
{
    Q_D(const NoteSearchQuery);
    return d->m_hasEncryption;
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

QTextStream & NoteSearchQuery::Print(QTextStream & strm) const
{
    Q_D(const NoteSearchQuery);
    return d->Print(strm);
}

} // namespace qute_note
