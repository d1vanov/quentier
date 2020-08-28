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

#ifndef QUENTIER_LIB_MODEL_ITEM_MODEL_H
#define QUENTIER_LIB_MODEL_ITEM_MODEL_H

#include <quentier/utility/Macros.h>

#include <QAbstractItemModel>
#include <QStringList>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace quentier {

/**
 * @brief The ItemModel class is the abstract base class for models representing
 * notebooks, tags, and saved searches.
 *
 * It contains some pure virtual methods required for the models managing
 * the items of all the mentioned types.
 */
class ItemModel : public QAbstractItemModel
{
    Q_OBJECT
protected:
    explicit ItemModel(QObject * parent = nullptr);

public:
    virtual ~ItemModel();

    /**
     * @brief localUidForItemName - finds local uid for item name
     * @param itemName              The name of the item for which the local
     *                              uid is required
     * @param linkedNotebookGuid    The guid of a linked notebook to which
     *                              the item which local uid is returned belongs
     *                              (if any)
     * @return                      The local uid corresponding to the item
     *                              name; empty string if no item with such name
     *                              exists
     */
    virtual QString localUidForItemName(
        const QString & itemName, const QString & linkedNotebookGuid) const = 0;

    /**
     * @brief itemNameForLocalUid   Finds item name for local uid
     * @param localUid              The local uid of the item for which the name
     *                              is required
     * @return                      The name of the item corresponding to
     *                              the local uid; empty string if no item with
     *                              such local uid exists
     */
    virtual QString itemNameForLocalUid(const QString & localUid) const = 0;

    struct ItemInfo
    {
        QString m_name;
        QString m_localUid;
        QString m_linkedNotebookGuid;
        QString m_linkedNotebookUsername;
    };

    friend QDebug & operator<<(QDebug & dbg, const ItemInfo & itemInfo);

    /**
     * @brief itemInfoForLocalUid   Finds item info for local uid
     * @param localUid              The local uid of the item which info is
     *                              required
     * @return                      Model item info
     */
    virtual ItemInfo itemInfoForLocalUid(const QString & localUid) const = 0;

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
    virtual QStringList itemNames(const QString & linkedNotebookGuid) const = 0;

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

    /**
     * @brief linkedNotebooksInfo
     * @return                      Linked notebook guids and corresponding
     *                              usernames
     */
    virtual QVector<LinkedNotebookInfo> linkedNotebooksInfo() const = 0;

    /**
     * @brief linkedNotebookUsername
     * @param linkedNotebooGuid     The linked notebook guid which corresponding
     *                              username is requested
     * @return                      Username corresponding to the passed in
     *                              linked notebook guid or empty string
     */
    virtual QString linkedNotebookUsername(
        const QString & linkedNotebookGuid) const = 0;

    /**
     * @brief nameColumn
     * @return                      The column containing the names of items
     *                              stored within the model
     */
    virtual int nameColumn() const = 0;

    /**
     * @brief sortingColumn
     * @return                      The column with respect to which the model
     *                              is sorted
     */
    virtual int sortingColumn() const = 0;

    /**
     * @brief sortOrder
     * @return                      The order with respect to which the model
     *                              is sorted
     */
    virtual Qt::SortOrder sortOrder() const = 0;

    /**
     * @brief allItemsListed
     * @return                      True if the model has received all items
     *                              from the local storage, false otherwise
     */
    virtual bool allItemsListed() const = 0;

    /**
     * @brief allItemsRootItemIndex
     * @return                      The index of root item which conceptionally
     *                              corresponds to an item representing all real
     *                              model's items. Should return invalid model
     *                              index if the concept is not applicable
     *                              to the model which is the default
     *                              implementation
     */
    virtual QModelIndex allItemsRootItemIndex() const
    {
        return {};
    }

Q_SIGNALS:
    /**
     * @brief allItemsListed - this signal should be emitted when the model has
     * received all items from the local storage
     */
    void notifyAllItemsListed();
};

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_ITEM_MODEL_H
