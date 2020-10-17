/*
 * Copyright 2016-2020 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUENTIER_LIB_MODEL_NOTE_MODEL_ITEM_H
#define QUENTIER_LIB_MODEL_NOTE_MODEL_ITEM_H

#include <quentier/utility/Printable.h>

#include <QByteArray>
#include <QStringList>

namespace quentier {

class NoteModelItem final: public Printable
{
public:
    NoteModelItem() = default;

    virtual ~NoteModelItem() override = default;

    const QString & localUid() const
    {
        return m_localUid;
    }

    void setLocalUid(QString localUid)
    {
        m_localUid = std::move(localUid);
    }

    const QString & guid() const
    {
        return m_guid;
    }

    void setGuid(QString guid)
    {
        m_guid = std::move(guid);
    }

    const QString & notebookLocalUid() const
    {
        return m_notebookLocalUid;
    }

    void setNotebookLocalUid(QString notebookLocalUid)
    {
        m_notebookLocalUid = std::move(notebookLocalUid);
    }

    const QString & notebookGuid() const
    {
        return m_notebookGuid;
    }

    void setNotebookGuid(QString notebookGuid)
    {
        m_notebookGuid = std::move(notebookGuid);
    }

    const QString & title() const
    {
        return m_title;
    }

    void setTitle(QString title)
    {
        m_title = std::move(title);
    }

    const QString & previewText() const
    {
        return m_previewText;
    }

    void setPreviewText(QString previewText)
    {
        m_previewText = std::move(previewText);
    }

    const QByteArray & thumbnailData() const
    {
        return m_thumbnailData;
    }

    void setThumbnailData(QByteArray thumbnailData)
    {
        m_thumbnailData = std::move(thumbnailData);
    }

    const QString & notebookName() const
    {
        return m_notebookName;
    }

    void setNotebookName(QString notebookName)
    {
        m_notebookName = std::move(notebookName);
    }

    const QStringList & tagLocalUids() const
    {
        return m_tagLocalUids;
    }

    void setTagLocalUids(QStringList tagLocalUids)
    {
        m_tagLocalUids = std::move(tagLocalUids);
    }

    void addTagLocalUid(const QString & tagLocalUid);

    void removeTagLocalUid(const QString & tagLocalUid);

    bool hasTagLocalUid(const QString & tagLocalUid) const;

    int numTagLocalUids() const;

    const QStringList & tagGuids() const
    {
        return m_tagGuids;
    }

    void setTagGuids(QStringList tagGuids)
    {
        m_tagGuids = std::move(tagGuids);
    }

    void addTagGuid(const QString & tagGuid);

    void removeTagGuid(const QString & tagGuid);

    bool hasTagGuid(const QString & tagGuid) const;

    int numTagGuids() const;

    const QStringList & tagNameList() const
    {
        return m_tagNameList;
    }

    void setTagNameList(QStringList tagNameList)
    {
        m_tagNameList = std::move(tagNameList);
    }

    void addTagName(const QString & tagName);

    void removeTagName(const QString & tagName);

    bool hasTagName(const QString & tagName) const;

    int numTagNames() const;

    qint64 creationTimestamp() const
    {
        return m_creationTimestamp;
    }

    void setCreationTimestamp(const qint64 creationTimestamp)
    {
        m_creationTimestamp = creationTimestamp;
    }

    qint64 modificationTimestamp() const
    {
        return m_modificationTimestamp;
    }

    void setModificationTimestamp(const qint64 modificationTimestamp)
    {
        m_modificationTimestamp = modificationTimestamp;
    }

    qint64 deletionTimestamp() const
    {
        return m_deletionTimestamp;
    }

    void setDeletionTimestamp(const qint64 deletionTimestamp)
    {
        m_deletionTimestamp = deletionTimestamp;
    }

    quint64 sizeInBytes() const
    {
        return m_sizeInBytes;
    }

    void setSizeInBytes(const quint64 sizeInBytes)
    {
        m_sizeInBytes = sizeInBytes;
    }

    bool isSynchronizable() const
    {
        return m_isSynchronizable;
    }

    void setSynchronizable(const bool synchronizable)
    {
        m_isSynchronizable = synchronizable;
    }

    bool isDirty() const
    {
        return m_isDirty;
    }

    void setDirty(const bool dirty)
    {
        m_isDirty = dirty;
    }

    bool isFavorited() const
    {
        return m_isFavorited;
    }

    void setFavorited(const bool favorited)
    {
        m_isFavorited = favorited;
    }

    bool isActive() const
    {
        return m_isActive;
    }

    void setActive(const bool active)
    {
        m_isActive = active;
    }

    bool hasResources() const
    {
        return m_hasResources;
    }

    void setHasResources(const bool hasResources)
    {
        m_hasResources = hasResources;
    }

    bool canUpdateTitle() const
    {
        return m_canUpdateTitle;
    }

    void setCanUpdateTitle(const bool canUpdateTitle)
    {
        m_canUpdateTitle = canUpdateTitle;
    }

    bool canUpdateContent() const
    {
        return m_canUpdateContent;
    }

    void setCanUpdateContent(const bool canUpdateContent)
    {
        m_canUpdateContent = canUpdateContent;
    }

    bool canEmail() const
    {
        return m_canEmail;
    }

    void setCanEmail(const bool canEmail)
    {
        m_canEmail = canEmail;
    }

    bool canShare() const
    {
        return m_canShare;
    }

    void setCanShare(const bool canShare)
    {
        m_canShare = canShare;
    }

    bool canSharePublicly() const
    {
        return m_canSharePublicly;
    }

    void setCanSharePublicly(const bool canSharePublicly)
    {
        m_canSharePublicly = canSharePublicly;
    }

    virtual QTextStream & print(QTextStream & strm) const override;

private:
    QString m_localUid;
    QString m_guid;
    QString m_notebookLocalUid;
    QString m_notebookGuid;
    QString m_title;
    QString m_previewText;
    QByteArray m_thumbnailData;
    QString m_notebookName;
    QStringList m_tagLocalUids;
    QStringList m_tagGuids;
    QStringList m_tagNameList;

    qint64 m_creationTimestamp = -1;
    qint64 m_modificationTimestamp = -1;
    qint64 m_deletionTimestamp = -1;
    quint64 m_sizeInBytes = 0;

    bool m_isSynchronizable = false;
    bool m_isDirty = true;
    bool m_isFavorited = false;
    bool m_isActive = true;
    bool m_hasResources = false;
    bool m_canUpdateTitle = true;
    bool m_canUpdateContent = true;
    bool m_canEmail = true;
    bool m_canShare = true;
    bool m_canSharePublicly = true;
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_NOTE_MODEL_ITEM_H
