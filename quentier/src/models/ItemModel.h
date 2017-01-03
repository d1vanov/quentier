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

#include <quentier/utility/Qt4Helper.h>
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
};

} // namespace quentier

#endif // QUENTIER_MODELS_ITEM_MODEL_H
