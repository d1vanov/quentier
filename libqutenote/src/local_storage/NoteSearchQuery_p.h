#ifndef __LIB_QUTE_NOTE__LOCAL_STORAGE__NOTE_SEARCH_QUERY_P_H
#define __LIB_QUTE_NOTE__LOCAL_STORAGE__NOTE_SEARCH_QUERY_P_H

#include <qute_note/utility/Printable.h>
#include <QStringList>
#include <QVector>

namespace qute_note {

class NoteSearchQueryPrivate : public Printable
{
public:
    NoteSearchQueryPrivate();

    void clear();

    bool parseQueryString(const QString & queryString, QString & error);

    QTextStream & Print(QTextStream & strm) const;

    QString         m_queryString;
    QString         m_notebookModifier;
    bool            m_hasAnyModifier;
    QStringList     m_tagNames;
    QStringList     m_negatedTagNames;
    bool            m_hasAnyTag;
    bool            m_hasNegatedAnyTag;
    QStringList     m_titleNames;
    QStringList     m_negatedTitleNames;
    bool            m_hasAnyTitleName;
    bool            m_hasNegatedAnyTitleName;
    QVector<qint64> m_creationTimestamps;
    QVector<qint64> m_negatedCreationTimestamps;
    bool            m_hasAnyCreationTimestamp;
    bool            m_hasNegatedAnyCreationTimestamp;
    QVector<qint64> m_modificationTimestamps;
    QVector<qint64> m_negatedModificationTimestamps;
    bool            m_hasAnyModificationTimestamp;
    bool            m_hasNegatedAnyModificationTimestamp;
    QStringList     m_resourceMimeTypes;
    QStringList     m_negatedResourceMimeTypes;
    bool            m_hasAnyResourceMimeType;
    bool            m_hasNegatedAnyResourceMimeType;
    QVector<qint64> m_subjectDateTimestamps;
    QVector<qint64> m_negatedSubjectDateTimestamps;
    bool            m_hasAnySubjectDateTimestamp;
    bool            m_hasNegatedAnySubjectDateTimestamp;
    QVector<double> m_latitudes;
    QVector<double> m_negatedLatitudes;
    bool            m_hasAnyLatitude;
    bool            m_hasNegatedAnyLatitude;
    QVector<double> m_longitudes;
    QVector<double> m_negatedLongitudes;
    bool            m_hasAnyLongitude;
    bool            m_hasNegatedAnyLongitude;
    QVector<double> m_altitudes;
    QVector<double> m_negatedAltitudes;
    bool            m_hasAnyAltitude;
    bool            m_hasNegatedAnyAltitude;
    QStringList     m_authors;
    QStringList     m_negatedAuthors;
    bool            m_hasAnyAuthor;
    bool            m_hasNegatedAnyAuthor;
    QStringList     m_sources;
    QStringList     m_negatedSources;
    bool            m_hasAnySource;
    bool            m_hasNegatedAnySource;
    QStringList     m_sourceApplications;
    QStringList     m_negatedSourceApplications;
    bool            m_hasAnySourceApplication;
    bool            m_hasNegatedAnySourceApplication;
    QStringList     m_contentClasses;
    QStringList     m_negatedContentClasses;
    bool            m_hasAnyContentClass;
    bool            m_hasNegatedAnyContentClass;
    QStringList     m_placeNames;
    QStringList     m_negatedPlaceNames;
    bool            m_hasAnyPlaceName;
    bool            m_hasNegatedAnyPlaceName;
    QStringList     m_applicationData;
    QStringList     m_negatedApplicationData;
    bool            m_hasAnyApplicationData;
    bool            m_hasNegatedAnyApplicationData;
    QVector<qint64> m_reminderOrders;
    QVector<qint64> m_negatedReminderOrders;
    bool            m_hasAnyReminderOrder;
    bool            m_hasNegatedAnyReminderOrder;
    QVector<qint64> m_reminderTimes;
    QVector<qint64> m_negatedReminderTimes;
    bool            m_hasAnyReminderTime;
    bool            m_hasNegatedAnyReminderTime;
    QVector<qint64> m_reminderDoneTimes;
    QVector<qint64> m_negatedReminderDoneTimes;
    bool            m_hasAnyReminderDoneTime;
    bool            m_hasNegatedAnyReminderDoneTime;
    bool            m_hasUnfinishedToDo;
    bool            m_hasNegatedUnfinishedToDo;
    bool            m_hasFinishedToDo;
    bool            m_hasNegatedFinishedToDo;
    bool            m_hasAnyToDo;
    bool            m_hasNegatedAnyToDo;
    bool            m_hasEncryption;
    bool            m_hasNegatedEncryption;
    QStringList     m_contentSearchTerms;
    QStringList     m_negatedContentSearchTerms;

    bool isMatcheable() const;

    bool hasAdvancedSearchModifiers() const;

private:
    QStringList splitSearchQueryString(const QString & searchQueryString) const;
    void parseStringValue(const QString & key, QStringList & words,
                          QStringList & container, QStringList & negatedContainer,
                          bool & hasAnyValue, bool & hasNegatedAnyValue) const;
    bool parseIntValue(const QString & key, QStringList & words,
                       QVector<qint64> & container, QVector<qint64> & negatedContainer,
                       bool & hasAnyValue, bool & hasNegatedAnyValue, QString & error) const;
    bool parseDoubleValue(const QString & key, QStringList & words,
                          QVector<double> & container, QVector<double> &negatedContainer,
                          bool & hasAnyValue, bool & hasNegatedAnyValue, QString & error) const;
    bool dateTimeStringToTimestamp(QString dateTimeString, qint64 & timestamp, QString & error) const;
    bool convertAbsoluteAndRelativeDateTimesToTimestamps(QStringList & words, QString & error) const;
    void removeBoundaryQuotesFromWord(QString & word) const;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__LOCAL_STORAGE__NOTE_SEARCH_QUERY_P_H
