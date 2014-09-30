#ifndef __QUTE_NOTE__CLIENT__TYPES__NOTE_H
#define __QUTE_NOTE__CLIENT__TYPES__NOTE_H

#include "DataElementWithShortcut.h"
#include <QEverCloud.h>
#include <QImage>
#include <QSharedDataPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(ResourceAdapter)
QT_FORWARD_DECLARE_CLASS(ResourceWrapper)
QT_FORWARD_DECLARE_CLASS(NoteData)

class QUTE_NOTE_EXPORT Note: public DataElementWithShortcut
{
public:
    QN_DECLARE_LOCAL_GUID
    QN_DECLARE_DIRTY

public:
    Note();
    Note(const Note & other);
    Note(Note && other);
    Note & operator=(const Note & other);
    Note & operator=(Note && other);

    Note(const qevercloud::Note & other);
    Note & operator=(const qevercloud::Note & other);

    virtual ~Note();

    bool operator==(const Note & other) const;
    bool operator!=(const Note & other) const;

    virtual bool hasGuid() const override;
    virtual const QString & guid() const override;
    virtual void setGuid(const QString & guid) override;

    virtual bool hasUpdateSequenceNumber() const override;
    virtual qint32 updateSequenceNumber() const override;
    virtual void setUpdateSequenceNumber(const qint32 usn) override;

    virtual void clear() final override;

    virtual bool checkParameters(QString & errorDescription) const override;

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
    bool containsEncryption() const;

private:
    virtual QTextStream & Print(QTextStream & strm) const override;

    QSharedDataPointer<NoteData> d;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CLIENT__TYPES__NOTE_H
