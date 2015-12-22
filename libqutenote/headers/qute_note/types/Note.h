#ifndef __LIB_QUTE_NOTE__TYPES__NOTE_H
#define __LIB_QUTE_NOTE__TYPES__NOTE_H

#include "IDataElementWithShortcut.h"
#include <QEverCloud.h>
#include <QImage>
#include <QSharedDataPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(ResourceAdapter)
QT_FORWARD_DECLARE_CLASS(ResourceWrapper)
QT_FORWARD_DECLARE_CLASS(NoteData)

class QUTE_NOTE_EXPORT Note: public IDataElementWithShortcut
{
public:
    QN_DECLARE_LOCAL_GUID
    QN_DECLARE_DIRTY
    QN_DECLARE_SHORTCUT
    QN_DECLARE_LOCAL

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

    operator const qevercloud::Note&() const;
    operator qevercloud::Note&();

    virtual bool hasGuid() const Q_DECL_OVERRIDE;
    virtual const QString & guid() const Q_DECL_OVERRIDE;
    virtual void setGuid(const QString & guid) Q_DECL_OVERRIDE;

    virtual bool hasUpdateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual qint32 updateSequenceNumber() const Q_DECL_OVERRIDE;
    virtual void setUpdateSequenceNumber(const qint32 usn) Q_DECL_OVERRIDE;

    virtual void clear() Q_DECL_OVERRIDE;

    virtual bool checkParameters(QString & errorDescription) const Q_DECL_OVERRIDE;

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
    bool updateResource(const IResource & resource);
    bool removeResource(const IResource & resource);

    bool hasNoteAttributes() const;
    const qevercloud::NoteAttributes & noteAttributes() const;
    qevercloud::NoteAttributes & noteAttributes();

    QImage thumbnail() const;
    void setThumbnail(const QImage & thumbnail);

    QString plainText(QString * pErrorMessage = Q_NULLPTR) const;
    QStringList listOfWords(QString * pErrorMessage = Q_NULLPTR) const;
    std::pair<QString, QStringList> plainTextAndListOfWords(QString * pErrorMessage = Q_NULLPTR) const;

    bool containsCheckedTodo() const;
    bool containsUncheckedTodo() const;
    bool containsTodo() const;
    bool containsEncryption() const;

private:
    virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

    QSharedDataPointer<NoteData> d;
};

} // namespace qute_note

Q_DECLARE_METATYPE(qute_note::Note)

#endif // __LIB_QUTE_NOTE__TYPES__NOTE_H
