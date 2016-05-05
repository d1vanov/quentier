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

    QString plainText(QString * pErrorMessage) const;
    QStringList listOfWords(QString * pErrorMessage) const;
    std::pair<QString, QStringList> plainTextAndListOfWords(QString * pErrorMessage) const;

    bool containsToDoImpl(const bool checked) const;
    bool containsEncryption() const;

    void setContent(const QString & content);

    qevercloud::Note m_qecNote;

    struct ResourceAdditionalInfo
    {
        QString  localUid;
        bool     isDirty;

        bool operator==(const ResourceAdditionalInfo & other) const;
    };

    QList<ResourceAdditionalInfo>   m_resourcesAdditionalInfo;
    qevercloud::Optional<QString>   m_notebookLocalUid;
    QStringList                     m_tagLocalUids;
    QImage                          m_thumbnail;

private:
    NoteData & operator=(const NoteData & other) Q_DECL_DELETE;
    NoteData & operator=(NoteData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__PRIVATE__NOTE_DATA_H
