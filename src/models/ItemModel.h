/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_MODELS_ITEM_MODEL_H
#define QUENTIER_MODELS_ITEM_MODEL_H

#include <quentier/utility/Macros.h>
#include <QAbstractItemModel>
#include <QStringList>

namespace quentier {

/**
 * @brief The ItemModel class is the abstract base class for models representing notebooks, tags,
 * and saved searches.
 *
 * It contains some pure virtual methods required for the models managing the items
 * of all the mentioned types.
 */
class ItemModel: public QAbstractItemModel
{
    Q_OBJECT
protected:
    explicit ItemModel(QObject * parent = Q_NULLPTR);

public:
    virtual ~ItemModel();

    /**
     * @brief localUidForItemName - finds local uid for item name
     * @param itemName - the name of the item for which the local uid is required
     * @return the local uid corresponding to the item name; empty string if no item
     * with such name exists
     */
    virtual QString localUidForItemName(const QString & itemName) const = 0;

    /**
     * @brief itemNameForLocalUid - finds item name for local uid
     * @param localUid - the local uid of the item for which the name is required
     * @return the name of the item corresponding to the local uid; empty string
     * if no item with such local uid exists
     */
    virtual QString itemNameForLocalUid(const QString & localUid) const = 0;

    /**
     * @brief itemNames
     * @return the sorted list of names of the items stored within the model
     */
    virtual QStringList itemNames() const = 0;

    /**
     * @brief nameColumn
     * @return the column containing the names of items stored within the model
     */
    virtual int nameColumn() const = 0;

    /**
     * @brief sortingColumn
     * @return the column with respect to which the model is sorted
     */
    virtual int sortingColumn() const = 0;

    /**
     * @brief sortOrder
     * @return the order with respect to which the model is sorted
     */
    virtual Qt::SortOrder sortOrder() const = 0;

    /**
     * @brief allItemsListed
     * @return true if the model has received all items from the local storage,
     * false otherwise
     */
    virtual bool allItemsListed() const = 0;

Q_SIGNALS:
    /**
     * @brief allItemsListed - this signal should be emitted when the model has
     * received all items from the local storage
     */
    void notifyAllItemsListed();
};

} // namespace quentier

#endif // QUENTIER_MODELS_ITEM_MODEL_H
