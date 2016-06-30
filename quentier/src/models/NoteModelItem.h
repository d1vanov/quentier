#ifndef QUENTIER_MODELS_NOTE_MODEL_ITEM_H
#define QUENTIER_MODELS_NOTE_MODEL_ITEM_H

#include <quentier/utility/Printable.h>
#include <QStringList>
#include <QImage>

namespace quentier {

class NoteModelItem: public Printable
{
public:
    explicit NoteModelItem(const QString & localUid = QString(),
                           const QString & guid = QString(),
                           const QString & notebookLocalUid = QString(),
                           const QString & notebookGuid = QString(),
                           const QString & title = QString(),
                           const QString & previewText = QString(),
                           const QImage & thumbnail = QImage(),
                           const QString & notebookName = QString(),
                           const QStringList & tagLocalUids = QStringList(),
                           const QStringList & tagGuids = QStringList(),
                           const QStringList & tagNameList = QStringList(),
                           const qint64 creationTimestamp = 0,
                           const qint64 modificationTimestamp = 0,
                           const qint64 deletionTimestamp = 0,
                           const quint64 sizeInBytes = 0,
                           const bool isSynchronizable = false,
                           const bool isDirty = false);
    ~NoteModelItem();

    const QString & localUid() const { return m_localUid; }
    void setLocalUid(const QString & localUid) { m_localUid = localUid; }

    const QString & guid() const { return m_guid; }
    void setGuid(const QString & guid) { m_guid = guid; }

    const QString & notebookLocalUid() const { return m_notebookLocalUid; }
    void setNotebookLocalUid(const QString & notebookLocalUid) { m_notebookLocalUid = notebookLocalUid; }

    const QString & notebookGuid() const { return m_notebookGuid; }
    void setNotebookGuid(const QString & notebookGuid) { m_notebookGuid = notebookGuid; }

    const QString & title() const { return m_title; }
    void setTitle(const QString & title) { m_title = title; }

    const QString & previewText() const { return m_previewText; }
    void setPreviewText(const QString & previewText) { m_previewText = previewText; }

    const QImage & thumbnail() const { return m_thumbnail; }
    void setThumbnail(const QImage & thumbnail) { m_thumbnail = thumbnail; }

    const QString & notebookName() const { return m_notebookName; }
    void setNotebookName(const QString & notebookName) { m_notebookName = notebookName; }

    const QStringList & tagLocalUids() const { return m_tagLocalUids; }
    void setTagLocalUids(const QStringList & tagLocalUids) { m_tagLocalUids = tagLocalUids; }
    void addTagLocalUid(const QString & tagLocalUid);
    void removeTagLocalUid(const QString & tagLocalUid);
    bool hasTagLocalUid(const QString & tagLocalUid) const;
    int numTagLocalUids() const;

    const QStringList & tagGuids() const { return m_tagGuids; }
    void setTagGuids(const QStringList & tagGuids) { m_tagGuids = tagGuids; }
    void addTagGuid(const QString & tagGuid);
    void removeTagGuid(const QString & tagGuid);
    bool hasTagGuid(const QString & tagGuid) const;
    int numTagGuids() const;

    const QStringList & tagNameList() const { return m_tagNameList; }
    void setTagNameList(const QStringList & tagNameList) { m_tagNameList = tagNameList; }
    void addTagName(const QString & tagName);
    void removeTagName(const QString & tagName);
    bool hasTagName(const QString & tagName) const;
    int numTagNames() const;

    qint64 creationTimestamp() const { return m_creationTimestamp; }
    void setCreationTimestamp(const qint64 creationTimestamp) { m_creationTimestamp = creationTimestamp; }

    qint64 modificationTimestamp() const { return m_modificationTimestamp; }
    void setModificationTimestamp(const qint64 modificationTimestamp) { m_modificationTimestamp = modificationTimestamp; }

    qint64 deletionTimestamp() const { return m_deletionTimestamp; }
    void setDeletionTimestamp(const qint64 deletionTimestamp) { m_deletionTimestamp = deletionTimestamp; }

    quint64 sizeInBytes() const { return m_sizeInBytes; }
    void setSizeInBytes(const quint64 sizeInBytes) { m_sizeInBytes = sizeInBytes; }

    bool isSynchronizable() const { return m_isSynchronizable; }
    void setSynchronizable(const bool synchronizable) { m_isSynchronizable = synchronizable; }

    bool isDirty() const { return m_isDirty; }
    void setDirty(const bool dirty) { m_isDirty = dirty; }

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QString     m_localUid;
    QString     m_guid;
    QString     m_notebookLocalUid;
    QString     m_notebookGuid;
    QString     m_title;
    QString     m_previewText;
    QImage      m_thumbnail;
    QString     m_notebookName;
    QStringList m_tagLocalUids;
    QStringList m_tagGuids;
    QStringList m_tagNameList;
    qint64      m_creationTimestamp;
    qint64      m_modificationTimestamp;
    qint64      m_deletionTimestamp;
    quint64     m_sizeInBytes;
    bool        m_isSynchronizable;
    bool        m_isDirty;
};

} // namespace quentier

#endif // QUENTIER_MODELS_NOTE_MODEL_ITEM_H
