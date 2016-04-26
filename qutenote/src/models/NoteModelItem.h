#ifndef __QUTE_NOTE__MODELS__NOTE_MODEL_ITEM_H
#define __QUTE_NOTE__MODELS__NOTE_MODEL_ITEM_H

#include <qute_note/utility/Printable.h>
#include <QStringList>

namespace qute_note {

class NoteModelItem: public Printable
{
public:
    explicit NoteModelItem(const QString & localUid = QString(),
                           const QString & guid = QString(),
                           const QString & notebookLocalUid = QString(),
                           const QString & linkedNotebookGuid = QString(),
                           const QString & title = QString(),
                           const QString & previewText = QString(),
                           const QString & thumbnailImageFilePath = QString(),
                           const QString & notebookName = QString(),
                           const QStringList & tagNameList = QStringList(),
                           const qint64 creationTimestamp = 0,
                           const qint64 modificationTimestamp = 0,
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

    const QString & linkedNotebookGuid() const { return m_linkedNotebookGuid; }
    void setLinkedNotebookGuid(const QString & linkedNotebookGuid) { m_linkedNotebookGuid = linkedNotebookGuid; }

    const QString & title() const { return m_title; }
    void setTitle(const QString & title) { m_title = title; }

    const QString & previewText() const { return m_previewText; }
    void setPreviewText(const QString & previewText) { m_previewText = previewText; }

    const QString & thumbnailImageFilePath() const { return m_thumbnailImageFilePath; }
    void setThumbnailImageFilePath(const QString & thumbnailImageFilePath) { m_thumbnailImageFilePath = thumbnailImageFilePath; }

    const QString & notebookName() const { return m_notebookName; }
    void setNotebookName(const QString & notebookName) { m_notebookName = notebookName; }

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
    QString     m_linkedNotebookGuid;
    QString     m_title;
    QString     m_previewText;
    QString     m_thumbnailImageFilePath;
    QString     m_notebookName;
    QStringList m_tagNameList;
    qint64      m_creationTimestamp;
    qint64      m_modificationTimestamp;
    quint64     m_sizeInBytes;
    bool        m_isSynchronizable;
    bool        m_isDirty;
};

} // namespace qute_note

#endif // __QUTE_NOTE__MODELS__NOTE_MODEL_ITEM_H
