#ifndef __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__NOTE_SEARCH_QUERY_PRIVATE_H
#define __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__NOTE_SEARCH_QUERY_PRIVATE_H

#include <tools/Printable.h>
#include <QStringList>
#include <QVector>

namespace qute_note {

class NoteSearchQueryPrivate : public Printable
{
public:
    NoteSearchQueryPrivate();

    bool parseQueryString(const QString & queryString, QString & error);

    QTextStream & Print(QTextStream & strm) const;

    QString         m_queryString;
    QString         m_notebookModifier;
    bool            m_hasAnyModifier;
    QStringList     m_tagNames;
    QStringList     m_negatedTagNames;
    QStringList     m_titleNames;
    QStringList     m_negatedTitleNames;
    QVector<qint64> m_creationTimestamps;
    QVector<qint64> m_negatedCreationTimestamps;
    QVector<qint64> m_modificationTimestamps;
    QVector<qint64> m_negatedModificationTimestamps;
    QStringList     m_resourceMimeTypes;
    QStringList     m_negatedResourceMimeTypes;
    QVector<qint64> m_subjectDateTimestamps;
    QVector<qint64> m_negatedSubjectDateTimestamps;
    QVector<double> m_latitudes;
    QVector<double> m_negatedLatitudes;
    QVector<double> m_longitudes;
    QVector<double> m_negatedLongitudes;
    QVector<double> m_altitudes;
    QVector<double> m_negatedAltitudes;
    QStringList     m_authors;
    QStringList     m_negatedAuthors;
    QStringList     m_sources;
    QStringList     m_negatedSources;
    QStringList     m_sourceApplications;
    QStringList     m_negatedSourceApplications;
    QStringList     m_contentClasses;
    QStringList     m_negatedContentClasses;
    QStringList     m_placeNames;
    QStringList     m_negatedPlaceNames;
    QStringList     m_applicationData;
    QStringList     m_negatedApplicationData;
    QStringList     m_recognitionTypes;
    QStringList     m_negatedRecognitionTypes;
    bool            m_hasReminderOrder;
    QVector<qint64> m_reminderOrders;
    QVector<qint64> m_negatedReminderOrders;
    QVector<qint64> m_reminderTimes;
    QVector<qint64> m_negatedReminderTimes;
    QVector<qint64> m_reminderDoneTimes;
    QVector<qint64> m_negatedReminderDoneTimes;
    bool            m_hasUnfinishedToDo;
    bool            m_hasFinishedToDo;
    bool            m_hasEncryption;
    QStringList     m_contentSearchTerms;
    QStringList     m_negatedContentSearchTerms;

private:
    QStringList splitSearchQueryString(const QString & searchQueryString) const;
    void parseStringValue(const QString & key, QStringList & words,
                          QStringList & container, QStringList & negatedContainer) const;
    bool parseIntValue(const QString & key, QStringList & words,
                       QVector<qint64> & container, QVector<qint64> & negatedContainer, QString & error) const;
    bool parseDoubleValue(const QString & key, QStringList & words,
                          QVector<double> & container, QVector<double> &negatedContainer, QString & error) const;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__LOCAL_STORAGE__NOTE_SEARCH_QUERY_PRIVATE_H
