#include "NoteSearchQuery_p.h"

namespace qute_note {

NoteSearchQueryPrivate::NoteSearchQueryPrivate() :
    m_queryString(),
    m_notebookModifier(),
    m_hasAnyModifier(false),
    m_tagNames(),
    m_negatedTagNames(),
    m_titleNames(),
    m_negatedTitleNames(),
    m_creationTimestamps(),
    m_negatedCreationTimestamps(),
    m_modificationTimestamps(),
    m_negatedModificationTimestamps(),
    m_resourceMimeTypes(),
    m_negatedResourceMimeTypes(),
    m_subjectDateTimestamps(),
    m_negatedSubjectDateTimestamps(),
    m_latitudes(),
    m_negatedLatitudes(),
    m_longitudes(),
    m_negatedLongitudes(),
    m_altitudes(),
    m_negatedAltitudes(),
    m_authors(),
    m_negatedAuthors(),
    m_sources(),
    m_negatedSources(),
    m_sourceApplications(),
    m_negatedSourceApplications(),
    m_contentClasses(),
    m_negatedContentClasses(),
    m_placeNames(),
    m_negatedPlaceNames(),
    m_applicationData(),
    m_negatedApplicationData(),
    m_recognitionTypes(),
    m_negatedRecognitionTypes(),
    m_hasReminderOrder(false),
    m_reminderOrders(),
    m_negatedReminderOrders(),
    m_reminderTimes(),
    m_negatedReminderTimes(),
    m_reminderDoneTimes(),
    m_negatedReminderDoneTimes(),
    m_hasUnfinishedToDo(false),
    m_hasFinishedToDo(false),
    m_hasEncryption(false),
    m_contentSearchTerms(),
    m_negatedContentSearchTerms()
{}

bool NoteSearchQueryPrivate::parseQueryString(const QString & queryString, QString & error)
{
    // TODO: implement
    Q_UNUSED(queryString)
    Q_UNUSED(error)
    return true;
}

QTextStream & NoteSearchQueryPrivate::Print(QTextStream & strm) const
{
    // TODO: implement
    return strm;
}

} // namespace qute_note
