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

#include <quentier/types/Account.h>

#include <QAbstractItemModel>
#include <QList>
#include <QStringList>

class QDebug;
class QTextStream;

namespace quentier {

/**
 * @brief The AbstractItemModel class is the abstract base class for models
 * representing items such as notebooks, tags, saved searches.
 */
class AbstractItemModel : public QAbstractItemModel
{
    Q_OBJECT
protected:
    explicit AbstractItemModel(Account account, QObject * parent = nullptr);

public:
    ~AbstractItemModel() override;

    [[nodiscard]] const Account & account() const noexcept
    {
        return m_account;
    }

    void setAccount(Account account)
    {
        m_account = std::move(account);
    }

    /**
     * @brief localIdForItemName    Finds local id for item name
     * @param itemName              Name of the item for which local id is
     *                              required
     * @param linkedNotebookGuid    Guid of a linked notebook to which the item
     *                              which local id is returned belongs (if any)
     * @return                      Local id corresponding to item name; empty
     *                              string if no item with such name exists
     */
    [[nodiscard]] virtual QString localIdForItemName(
        const QString & itemName, const QString & linkedNotebookGuid) const = 0;

    /**
     * @brief indexForLocalId
     * @param localId               Local id of the item which index is required
     * @return                      Model index of the item corresponding to
     *                              local id or invalid index if no item with
     *                              such local id exists
     */
    [[nodiscard]] virtual QModelIndex indexForLocalId(
        const QString & localId) const = 0;

    /**
     * @brief itemNameForLocalId    Finds item name for local id
     * @param localId               Local id of the item for which name is
     *                              required
     * @return                      Name of the item corresponding to local id;
     *                              empty string if no item with such local id
     *                              exists
     */
    [[nodiscard]] virtual QString itemNameForLocalId(
        const QString & localId) const = 0;

    struct ItemInfo
    {
        QString m_name;
        QString m_localId;
        QString m_linkedNotebookGuid;
        QString m_linkedNotebookUsername;
    };

    friend QDebug & operator<<(QDebug & dbg, const ItemInfo & itemInfo);
    friend QTextStream & operator<<(
        QTextStream & strm, const ItemInfo & itemInfo);

    /**
     * @brief itemInfoForLocalId    Finds item info for local id
     * @param localId               The local id of the item which info is
     *                              required
     * @return                      Model item info
     */
    [[nodiscard]] virtual ItemInfo itemInfoForLocalId(
        const QString & localId) const = 0;

    /**
     * @brief itemNames
     * @param linkedNotebookGuid    The optional guid of a linked notebook to
     *                              which the returned item names belong; if it
     *                              is null (i.e. linkedNotebookGuid.isNull()
     *                              returns true), the item names would be
     *                              returned ignoring their belonging to user's
     *                              own account or linked notebook; if it's not
     *                              null but empty (i.e.
     *                              linkedNotebookGuid.isEmpty() returns true),
     *                              only the names of tags from user's own
     *                              account would be returned. Otherwise only
     *                              the names of tags from the corresponding
     *                              linked notebook would be returned
     * @return                      The sorted list of names of the items stored
     *                              within the model
     */
    [[nodiscard]] virtual QStringList itemNames(
        const QString & linkedNotebookGuid) const = 0;

    struct LinkedNotebookInfo
    {
        LinkedNotebookInfo() = default;

        LinkedNotebookInfo(QString guid, QString username) :
            m_guid(std::move(guid)), m_username(std::move(username))
        {}

        QString m_guid;
        QString m_username;
    };

    friend QDebug & operator<<(QDebug & dbg, const LinkedNotebookInfo & info);
    friend QTextStream & operator<<(
        QTextStream & strm, const LinkedNotebookInfo & info);

    /**
     * @brief linkedNotebooksInfo
     * @return                      Linked notebook guids and corresponding
     *                              usernames
     */
    [[nodiscard]] virtual QList<LinkedNotebookInfo> linkedNotebooksInfo()
        const = 0;

    /**
     * @brief linkedNotebookUsername
     * @param linkedNotebooGuid     The linked notebook guid which corresponding
     *                              username is requested
     * @return                      Username corresponding to the passed in
     *                              linked notebook guid or empty string
     */
    [[nodiscard]] virtual QString linkedNotebookUsername(
        const QString & linkedNotebookGuid) const = 0;

    /**
     * @brief nameColumn
     * @return                      The column containing the names of items
     *                              stored within the model
     */
    [[nodiscard]] virtual int nameColumn() const noexcept = 0;

    /**
     * @brief sortingColumn
     * @return                      The column with respect to which the model
     *                              is sorted
     */
    [[nodiscard]] virtual int sortingColumn() const noexcept = 0;

    /**
     * @brief sortOrder
     * @return                      The order with respect to which the model
     *                              is sorted
     */
    [[nodiscard]] virtual Qt::SortOrder sortOrder() const noexcept = 0;

    /**
     * @brief allItemsListed
     * @return                      True if the model has received all items
     *                              from the local storage, false otherwise
     */
    [[nodiscard]] virtual bool allItemsListed() const noexcept = 0;

    /**
     * @brief allItemsRootItemIndex
     * @return                      The index of root item which conceptionally
     *                              corresponds to an item representing all real
     *                              model's items. Should return invalid model
     *                              index if the concept is not applicable
     *                              to the model which is the default
     *                              implementation
     */
    [[nodiscard]] virtual QModelIndex allItemsRootItemIndex() const
    {
        return {};
    }

    /**
     * @brief localIdForItemIndex
     * @return                      Local id of the item corresponding to the
     *                              passed in index, if the index represents
     *                              an item having a local id; empty string
     *                              otherwise
     */
    [[nodiscard]] virtual QString localIdForItemIndex(
        const QModelIndex & index) const = 0;

    /**
     * @brief linkedNotebookGuidForItemIndex
     * @return                      Linked notebook guid of the linked notebook
     *                              item corresponding to the passed in index;
     *                              empty string if the passed in index does not
     *                              correspond to a linked notebook item
     */
    [[nodiscard]] virtual QString linkedNotebookGuidForItemIndex(
        const QModelIndex & index) const = 0;

    /**
     * @brief persistentIndexes method is simply a public accessor for
     * QAbstractItemModel's protected persistentIndexList method
     */
    [[nodiscard]] QModelIndexList persistentIndexes() const
    {
        return persistentIndexList();
    }

Q_SIGNALS:
    /**
     * @brief allItemsListed - this signal should be emitted when the model has
     * received all items from the local storage
     */
    void notifyAllItemsListed();

protected:
    Account m_account;
};

} // namespace quentier
