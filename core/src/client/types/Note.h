#ifndef __QUTE_NOTE__CLIENT__TYPES__NOTE_H
#define __QUTE_NOTE__CLIENT__TYPES__NOTE_H

#include "DataElementWithShortcut.h"
#include <QEverCloud.h>
#include <QImage>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(ResourceAdapter)
QT_FORWARD_DECLARE_CLASS(ResourceWrapper)

class QUTE_NOTE_EXPORT Note final: public DataElementWithShortcut
{
public:
    Note();
    Note(const Note & other) = default;
    Note(Note && other);
    Note & operator=(const Note & other) = default;
    Note & operator=(Note && other);

    Note(const qevercloud::Note & other);
    Note(qevercloud::Note && other);
    Note & operator=(const qevercloud::Note & other);
    Note & operator=(qevercloud::Note && other);

    virtual ~Note() final override;

    bool operator==(const Note & other) const;
    bool operator!=(const Note & other) const;

    virtual bool hasGuid() const final override;
    virtual const QString & guid() const final override;
    virtual void setGuid(const QString & guid) final override;

    virtual bool hasUpdateSequenceNumber() const final override;
    virtual qint32 updateSequenceNumber() const final override;
    virtual void setUpdateSequenceNumber(const qint32 usn) final override;

    virtual void clear() final override;

    virtual bool checkParameters(QString & errorDescription) const final override;

    bool hasTitle() const;
    const QString & title() const;
    void setTitle(const QString & title);

    bool hasContent() const;
    const QString & content() const;
    void setContent(const QString & content);

    bool hasContentHash() const;
    const QByteArray & contentHash() const;
    void setContentHash(const QByteArray & contentHash);

    bool hasContentLength() const;
    qint32 contentLength() const;
    void setContentLength(const qint32 length);

    bool hasCreationTimestamp() const;
    qint64 creationTimestamp() const;
    void setCreationTimestamp(const qint64 timestamp);

    bool hasModificationTimestamp() const;
    qint64 modificationTimestamp() const;
    void setModificationTimestamp(const qint64 timestamp);

    bool hasDeletionTimestamp() const;
    qint64 deletionTimestamp() const;
    void setDeletionTimestamp(const qint64 timestamp);

    bool hasActive() const;
    bool active() const;
    void setActive(const bool active);

    bool hasNotebookGuid() const;
    const QString & notebookGuid() const;
    void setNotebookGuid(const QString & guid);

    bool hasTagGuids() const;
    void tagGuids(QStringList & guids) const;
    void setTagGuids(const QStringList & guids);
    void addTagGuid(const QString & guid);
    void removeTagGuid(const QString & guid);

    bool hasResources() const;
    QList<ResourceAdapter> resourceAdapters() const;
    QList<ResourceWrapper> resources() const;
    void setResources(const QList<ResourceAdapter> & resources);
    void setResources(const QList<ResourceWrapper> & resources);
    void addResource(const IResource & resource);
    void removeResource(const IResource & resource);

    bool hasNoteAttributes() const;
    const qevercloud::NoteAttributes & noteAttributes() const;
    qevercloud::NoteAttributes & noteAttributes();

    bool isLocal() const;
    void setLocal(const bool local);

    QImage thumbnail() const;
    void setThumbnail(const QImage & thumbnail);

    QString plainText(QString * errorMessage = nullptr) const;
    QStringList listOfWords(QString * errorMessage = nullptr) const;

    bool containsCheckedTodo() const;
    bool containsUncheckedTodo() const;
    bool containsTodo() const;

private:
    virtual QTextStream & Print(QTextStream & strm) const final override;
    bool containsToDoImpl(const bool checked) const;

    qevercloud::Note m_qecNote;

    struct ResourceAdditionalInfo
    {
        QString localGuid;
        QString noteLocalGuid;
        bool    isDirty;

        bool operator==(const ResourceAdditionalInfo & other) const
        {
            return (localGuid == other.localGuid) &&
                   (noteLocalGuid == other.noteLocalGuid) &&
                   (isDirty == other.isDirty);
        }
    };

    QList<ResourceAdditionalInfo> m_resourcesAdditionalInfo;
    bool m_isLocal;
    QImage m_thumbnail;

    mutable QString m_lazyPlainText;
    mutable bool    m_lazyPlainTextIsValid;

    mutable QStringList m_lazyListOfWords;
    mutable bool        m_lazyListOfWordsIsValid;

    // these flags are int in order to handle the "empty" state:
    // "-1" - empty, undefined
    // "0" - false
    // "1" - true
    mutable int m_lazyContainsCheckedToDo;
    mutable int m_lazyContainsUncheckedToDo;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__NOTE_H
