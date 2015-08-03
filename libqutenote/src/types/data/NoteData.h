#ifndef __LIB_QUTE_NOTE__TYPES__PRIVATE__NOTE_DATA_H
#define __LIB_QUTE_NOTE__TYPES__PRIVATE__NOTE_DATA_H

#include "DataElementWithShortcutData.h"
#include <QEverCloud.h>
#include <QImage>

namespace qute_note {

class NoteData: public DataElementWithShortcutData
{
public:
    NoteData();
    NoteData(const NoteData & other);
    NoteData(NoteData && other);
    NoteData(const qevercloud::Note & other);
    virtual ~NoteData();

    void clear();
    bool checkParameters(QString & errorDescription) const;

    QString plainText(QString * errorMessage) const;
    QStringList listOfWords(QString * errorMessage) const;

    bool containsToDoImpl(const bool checked) const;
    bool containsEncryption() const;

    void setContent(const QString & content);

    qevercloud::Note m_qecNote;

    struct ResourceAdditionalInfo
    {
        QString  localGuid;
        QString  noteLocalGuid;
        bool     isDirty;

        bool operator==(const ResourceAdditionalInfo & other) const;
    };

    QList<ResourceAdditionalInfo> m_resourcesAdditionalInfo;
    QImage   m_thumbnail;

    mutable QString  m_lazyPlainText;
    mutable bool     m_lazyPlainTextIsValid;

    mutable QStringList  m_lazyListOfWords;
    mutable bool         m_lazyListOfWordsIsValid;

    // these flags are int in order to handle the "empty" state:
    // "-1" - empty, undefined
    // "0" - false
    // "1" - true
    mutable int  m_lazyContainsCheckedToDo;
    mutable int  m_lazyContainsUncheckedToDo;
    mutable int  m_lazyContainsEncryption;

private:
    NoteData & operator=(const NoteData & other) Q_DECL_DELETE;
    NoteData & operator=(NoteData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__PRIVATE__NOTE_DATA_H
