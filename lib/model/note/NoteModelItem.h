/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/utility/Printable.h>

#include <QByteArray>
#include <QStringList>

namespace quentier {

class NoteModelItem final : public Printable
{
public:
    NoteModelItem() = default;

    ~NoteModelItem() override = default;

    [[nodiscard]] const QString & localId() const noexcept
    {
        return m_localId;
    }

    void setLocalId(QString localId) noexcept
    {
        m_localId = std::move(localId);
    }

    [[nodiscard]] const QString & guid() const noexcept
    {
        return m_guid;
    }

    void setGuid(QString guid) noexcept
    {
        m_guid = std::move(guid);
    }

    [[nodiscard]] const QString & notebookLocalId() const noexcept
    {
        return m_notebookLocalId;
    }

    void setNotebookLocalId(QString notebookLocalId) noexcept
    {
        m_notebookLocalId = std::move(notebookLocalId);
    }

    [[nodiscard]] const QString & notebookGuid() const noexcept
    {
        return m_notebookGuid;
    }

    void setNotebookGuid(QString notebookGuid) noexcept
    {
        m_notebookGuid = std::move(notebookGuid);
    }

    [[nodiscard]] const QString & title() const noexcept
    {
        return m_title;
    }

    void setTitle(QString title) noexcept
    {
        m_title = std::move(title);
    }

    [[nodiscard]] const QString & previewText() const noexcept
    {
        return m_previewText;
    }

    void setPreviewText(QString previewText) noexcept
    {
        m_previewText = std::move(previewText);
    }

    [[nodiscard]] const QByteArray & thumbnailData() const noexcept
    {
        return m_thumbnailData;
    }

    void setThumbnailData(QByteArray thumbnailData) noexcept
    {
        m_thumbnailData = std::move(thumbnailData);
    }

    [[nodiscard]] const QString & notebookName() const noexcept
    {
        return m_notebookName;
    }

    void setNotebookName(QString notebookName) noexcept
    {
        m_notebookName = std::move(notebookName);
    }

    [[nodiscard]] const QStringList & tagLocalIds() const noexcept
    {
        return m_tagLocalIds;
    }

    void setTagLocalIds(QStringList tagLocalIds) noexcept
    {
        m_tagLocalIds = std::move(tagLocalIds);
    }

    void addTagLocalId(const QString & tagLocalId);
    void removeTagLocalId(const QString & tagLocalId);
    [[nodiscard]] bool hasTagLocalId(const QString & tagLocalId) const noexcept;
    [[nodiscard]] int numTagLocalIds() const noexcept;

    [[nodiscard]] const QStringList & tagGuids() const noexcept
    {
        return m_tagGuids;
    }

    void setTagGuids(QStringList tagGuids) noexcept
    {
        m_tagGuids = std::move(tagGuids);
    }

    void addTagGuid(const QString & tagGuid);
    void removeTagGuid(const QString & tagGuid);
    [[nodiscard]] bool hasTagGuid(const QString & tagGuid) const noexcept;
    [[nodiscard]] int numTagGuids() const noexcept;

    [[nodiscard]] const QStringList & tagNameList() const noexcept
    {
        return m_tagNameList;
    }

    void setTagNameList(QStringList tagNameList) noexcept
    {
        m_tagNameList = std::move(tagNameList);
    }

    void addTagName(const QString & tagName);
    void removeTagName(const QString & tagName);
    [[nodiscard]] bool hasTagName(const QString & tagName) const noexcept;
    [[nodiscard]] int numTagNames() const noexcept;

    [[nodiscard]] qint64 creationTimestamp() const noexcept
    {
        return m_creationTimestamp;
    }

    void setCreationTimestamp(const qint64 creationTimestamp) noexcept
    {
        m_creationTimestamp = creationTimestamp;
    }

    [[nodiscard]] qint64 modificationTimestamp() const noexcept
    {
        return m_modificationTimestamp;
    }

    void setModificationTimestamp(const qint64 modificationTimestamp) noexcept
    {
        m_modificationTimestamp = modificationTimestamp;
    }

    [[nodiscard]] qint64 deletionTimestamp() const noexcept
    {
        return m_deletionTimestamp;
    }

    void setDeletionTimestamp(const qint64 deletionTimestamp) noexcept
    {
        m_deletionTimestamp = deletionTimestamp;
    }

    [[nodiscard]] quint64 sizeInBytes() const noexcept
    {
        return m_sizeInBytes;
    }

    void setSizeInBytes(const quint64 sizeInBytes) noexcept
    {
        m_sizeInBytes = sizeInBytes;
    }

    [[nodiscard]] bool isSynchronizable() const noexcept
    {
        return m_isSynchronizable;
    }

    void setSynchronizable(const bool synchronizable) noexcept
    {
        m_isSynchronizable = synchronizable;
    }

    [[nodiscard]] bool isDirty() const noexcept
    {
        return m_isDirty;
    }

    void setDirty(const bool dirty) noexcept
    {
        m_isDirty = dirty;
    }

    [[nodiscard]] bool isFavorited() const noexcept
    {
        return m_isFavorited;
    }

    void setFavorited(const bool favorited) noexcept
    {
        m_isFavorited = favorited;
    }

    [[nodiscard]] bool isActive() const noexcept
    {
        return m_isActive;
    }

    void setActive(const bool active) noexcept
    {
        m_isActive = active;
    }

    [[nodiscard]] bool hasResources() const noexcept
    {
        return m_hasResources;
    }

    void setHasResources(const bool hasResources) noexcept
    {
        m_hasResources = hasResources;
    }

    [[nodiscard]] bool canUpdateTitle() const noexcept
    {
        return m_canUpdateTitle;
    }

    void setCanUpdateTitle(const bool canUpdateTitle) noexcept
    {
        m_canUpdateTitle = canUpdateTitle;
    }

    [[nodiscard]] bool canUpdateContent() const noexcept
    {
        return m_canUpdateContent;
    }

    void setCanUpdateContent(const bool canUpdateContent) noexcept
    {
        m_canUpdateContent = canUpdateContent;
    }

    [[nodiscard]] bool canEmail() const noexcept
    {
        return m_canEmail;
    }

    void setCanEmail(const bool canEmail) noexcept
    {
        m_canEmail = canEmail;
    }

    [[nodiscard]] bool canShare() const noexcept
    {
        return m_canShare;
    }

    void setCanShare(const bool canShare) noexcept
    {
        m_canShare = canShare;
    }

    [[nodiscard]] bool canSharePublicly() const noexcept
    {
        return m_canSharePublicly;
    }

    void setCanSharePublicly(const bool canSharePublicly) noexcept
    {
        m_canSharePublicly = canSharePublicly;
    }

    QTextStream & print(QTextStream & strm) const override;

private:
    QString m_localId;
    QString m_guid;
    QString m_notebookLocalId;
    QString m_notebookGuid;
    QString m_title;
    QString m_previewText;
    QByteArray m_thumbnailData;
    QString m_notebookName;
    QStringList m_tagLocalIds;
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
