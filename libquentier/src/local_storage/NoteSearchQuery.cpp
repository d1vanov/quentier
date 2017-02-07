/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include <quentier/local_storage/NoteSearchQuery.h>
#include "NoteSearchQueryData.h"

namespace quentier {

NoteSearchQuery::NoteSearchQuery() :
    d(new NoteSearchQueryData)
{}

NoteSearchQuery::NoteSearchQuery(const NoteSearchQuery & other) :
    Printable(),
    d(other.d)
{}

NoteSearchQuery::NoteSearchQuery(NoteSearchQuery && other) :
    d(std::move(other.d))
{}

NoteSearchQuery & NoteSearchQuery::operator=(const NoteSearchQuery & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

NoteSearchQuery & NoteSearchQuery::operator=(NoteSearchQuery && other)
{
    if (this != &other) {
        d = std::move(other.d);
    }

    return *this;
}

NoteSearchQuery::~NoteSearchQuery()
{}

bool NoteSearchQuery::isEmpty() const
{
    return d->m_queryString.isEmpty();
}

void NoteSearchQuery::clear()
{
    d->clear();
}

const QString NoteSearchQuery::queryString() const
{
    return d->m_queryString;
}

bool NoteSearchQuery::setQueryString(const QString & queryString, ErrorString & error)
{
    d->clear();
    return d->parseQueryString(queryString, error);
}

const QString NoteSearchQuery::notebookModifier() const
{
    return d->m_notebookModifier;
}

bool NoteSearchQuery::hasAnyModifier() const
{
    return d->m_hasAnyModifier;
}

const QStringList & NoteSearchQuery::tagNames() const
{
    return d->m_tagNames;
}

const QStringList & NoteSearchQuery::negatedTagNames() const
{
    return d->m_negatedTagNames;
}

bool NoteSearchQuery::hasAnyTag() const
{
    return d->m_hasAnyTag;
}

bool NoteSearchQuery::hasNegatedAnyTag() const
{
    return d->m_hasNegatedAnyTag;
}

const QStringList & NoteSearchQuery::titleNames() const
{
    return d->m_titleNames;
}

const QStringList & NoteSearchQuery::negatedTitleNames() const
{
    return d->m_negatedTitleNames;
}

bool NoteSearchQuery::hasAnyTitleName() const
{
    return d->m_hasAnyTitleName;
}

bool NoteSearchQuery::hasNegatedAnyTitleName() const
{
    return d->m_hasNegatedAnyTitleName;
}

const QVector<qint64> & NoteSearchQuery::creationTimestamps() const
{
    return d->m_creationTimestamps;
}

const QVector<qint64> & NoteSearchQuery::negatedCreationTimestamps() const
{
    return d->m_negatedCreationTimestamps;
}

bool NoteSearchQuery::hasAnyCreationTimestamp() const
{
    return d->m_hasAnyCreationTimestamp;
}

bool NoteSearchQuery::hasNegatedAnyCreationTimestamp() const
{
    return d->m_hasNegatedAnyCreationTimestamp;
}

const QVector<qint64> & NoteSearchQuery::modificationTimestamps() const
{
    return d->m_modificationTimestamps;
}

const QVector<qint64> & NoteSearchQuery::negatedModificationTimestamps() const
{
    return d->m_negatedModificationTimestamps;
}

bool NoteSearchQuery::hasAnyModificationTimestamp() const
{
    return d->m_hasAnyModificationTimestamp;
}

bool NoteSearchQuery::hasNegatedAnyModificationTimestamp() const
{
    return d->m_hasNegatedAnyModificationTimestamp;
}

const QStringList & NoteSearchQuery::resourceMimeTypes() const
{
    return d->m_resourceMimeTypes;
}

const QStringList & NoteSearchQuery::negatedResourceMimeTypes() const
{
    return d->m_negatedResourceMimeTypes;
}

bool NoteSearchQuery::hasAnyResourceMimeType() const
{
    return d->m_hasAnyResourceMimeType;
}

bool NoteSearchQuery::hasNegatedAnyResourceMimeType() const
{
    return d->m_hasNegatedAnyResourceMimeType;
}

const QVector<qint64> & NoteSearchQuery::subjectDateTimestamps() const
{
    return d->m_subjectDateTimestamps;
}

const QVector<qint64> & NoteSearchQuery::negatedSubjectDateTimestamps() const
{
    return d->m_negatedSubjectDateTimestamps;
}

bool NoteSearchQuery::hasAnySubjectDateTimestamp() const
{
    return d->m_hasAnySubjectDateTimestamp;
}

bool NoteSearchQuery::hasNegatedAnySubjectDateTimestamp() const
{
    return d->m_hasNegatedAnySubjectDateTimestamp;
}

const QVector<double> & NoteSearchQuery::latitudes() const
{
    return d->m_latitudes;
}

const QVector<double> & NoteSearchQuery::negatedLatitudes() const
{
    return d->m_negatedLatitudes;
}

bool NoteSearchQuery::hasAnyLatitude() const
{
    return d->m_hasAnyLatitude;
}

bool NoteSearchQuery::hasNegatedAnyLatitude() const
{
    return d->m_hasNegatedAnyLatitude;
}

const QVector<double> & NoteSearchQuery::longitudes() const
{
    return d->m_longitudes;
}

const QVector<double> & NoteSearchQuery::negatedLongitudes() const
{
    return d->m_negatedLongitudes;
}

bool NoteSearchQuery::hasAnyLongitude() const
{
    return d->m_hasAnyLongitude;
}

bool NoteSearchQuery::hasNegatedAnyLongitude() const
{
    return d->m_hasNegatedAnyLongitude;
}

const QVector<double> & NoteSearchQuery::altitudes() const
{
    return d->m_altitudes;
}

const QVector<double> & NoteSearchQuery::negatedAltitudes() const
{
    return d->m_negatedAltitudes;
}

bool NoteSearchQuery::hasAnyAltitude() const
{
    return d->m_hasAnyAltitude;
}

bool NoteSearchQuery::hasNegatedAnyAltitude() const
{
    return d->m_hasNegatedAnyAltitude;
}

const QStringList & NoteSearchQuery::authors() const
{
    return d->m_authors;
}

const QStringList & NoteSearchQuery::negatedAuthors() const
{
    return d->m_negatedAuthors;
}

bool NoteSearchQuery::hasAnyAuthor() const
{
    return d->m_hasAnyAuthor;
}

bool NoteSearchQuery::hasNegatedAnyAuthor() const
{
    return d->m_hasNegatedAnyAuthor;
}

const QStringList & NoteSearchQuery::sources() const
{
    return d->m_sources;
}

const QStringList & NoteSearchQuery::negatedSources() const
{
    return d->m_negatedSources;
}

bool NoteSearchQuery::hasAnySource() const
{
    return d->m_hasAnySource;
}

bool NoteSearchQuery::hasNegatedAnySource() const
{
    return d->m_hasNegatedAnySource;
}

const QStringList & NoteSearchQuery::sourceApplications() const
{
    return d->m_sourceApplications;
}

const QStringList & NoteSearchQuery::negatedSourceApplications() const
{
    return d->m_negatedSourceApplications;
}

bool NoteSearchQuery::hasAnySourceApplication() const
{
    return d->m_hasAnySourceApplication;
}

bool NoteSearchQuery::hasNegatedAnySourceApplication() const
{
    return d->m_hasNegatedAnySourceApplication;
}

const QStringList & NoteSearchQuery::contentClasses() const
{
    return d->m_contentClasses;
}

const QStringList & NoteSearchQuery::negatedContentClasses() const
{
    return d->m_negatedContentClasses;
}

bool NoteSearchQuery::hasAnyContentClass() const
{
    return d->m_hasAnyContentClass;
}

bool NoteSearchQuery::hasNegatedAnyContentClass() const
{
    return d->m_hasNegatedAnyContentClass;
}

const QStringList & NoteSearchQuery::placeNames() const
{
    return d->m_placeNames;
}

const QStringList & NoteSearchQuery::negatedPlaceNames() const
{
    return d->m_negatedPlaceNames;
}

bool NoteSearchQuery::hasAnyPlaceName() const
{
    return d->m_hasAnyPlaceName;
}

bool NoteSearchQuery::hasNegatedAnyPlaceName() const
{
    return d->m_hasNegatedAnyPlaceName;
}

const QStringList & NoteSearchQuery::applicationData() const
{
    return d->m_applicationData;
}

const QStringList & NoteSearchQuery::negatedApplicationData() const
{
    return d->m_negatedApplicationData;
}

bool NoteSearchQuery::hasAnyApplicationData() const
{
    return d->m_hasAnyApplicationData;
}

bool NoteSearchQuery::hasNegatedAnyApplicationData() const
{
    return d->m_hasNegatedAnyApplicationData;
}

bool NoteSearchQuery::hasAnyReminderOrder() const
{
    return d->m_hasAnyReminderOrder;
}

bool NoteSearchQuery::hasNegatedAnyReminderOrder() const
{
    return d->m_hasNegatedAnyReminderOrder;
}

const QVector<qint64> & NoteSearchQuery::reminderOrders() const
{
    return d->m_reminderOrders;
}

const QVector<qint64> & NoteSearchQuery::negatedReminderOrders() const
{
    return d->m_negatedReminderOrders;
}

const QVector<qint64> & NoteSearchQuery::reminderTimes() const
{
    return d->m_reminderTimes;
}

const QVector<qint64> & NoteSearchQuery::negatedReminderTimes() const
{
    return d->m_negatedReminderTimes;
}

bool NoteSearchQuery::hasAnyReminderTime() const
{
    return d->m_hasAnyReminderTime;
}

bool NoteSearchQuery::hasNegatedAnyReminderTime() const
{
    return d->m_hasNegatedAnyReminderTime;
}

const QVector<qint64> & NoteSearchQuery::reminderDoneTimes() const
{
    return d->m_reminderDoneTimes;
}

const QVector<qint64> & NoteSearchQuery::negatedReminderDoneTimes() const
{
    return d->m_negatedReminderDoneTimes;
}

bool NoteSearchQuery::hasAnyReminderDoneTime() const
{
    return d->m_hasAnyReminderDoneTime;
}

bool NoteSearchQuery::hasNegatedAnyReminderDoneTime() const
{
    return d->m_hasNegatedAnyReminderDoneTime;
}

bool NoteSearchQuery::hasUnfinishedToDo() const
{
    return d->m_hasUnfinishedToDo;
}

bool NoteSearchQuery::hasNegatedUnfinishedToDo() const
{
    return d->m_hasNegatedUnfinishedToDo;
}

bool NoteSearchQuery::hasFinishedToDo() const
{
    return d->m_hasFinishedToDo;
}

bool NoteSearchQuery::hasNegatedFinishedToDo() const
{
    return d->m_hasNegatedFinishedToDo;
}

bool NoteSearchQuery::hasAnyToDo() const
{
    return d->m_hasAnyToDo;
}

bool NoteSearchQuery::hasNegatedAnyToDo() const
{
    return d->m_hasNegatedAnyToDo;
}

bool NoteSearchQuery::hasEncryption() const
{
    return d->m_hasEncryption;
}

bool NoteSearchQuery::hasNegatedEncryption() const
{
    return d->m_hasNegatedEncryption;
}

const QStringList & NoteSearchQuery::contentSearchTerms() const
{
    return d->m_contentSearchTerms;
}

const QStringList & NoteSearchQuery::negatedContentSearchTerms() const
{
    return d->m_negatedContentSearchTerms;
}

bool NoteSearchQuery::hasAnyContentSearchTerms() const
{
    return (!d->m_contentSearchTerms.isEmpty()) || (!d->m_negatedContentSearchTerms.isEmpty());
}

bool NoteSearchQuery::isMatcheable() const
{
    return d->isMatcheable();
}

QTextStream & NoteSearchQuery::print(QTextStream & strm) const
{
    return d->print(strm);
}

} // namespace quentier
