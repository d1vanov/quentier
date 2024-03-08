/*
 * Copyright 2020-2024 Dmitry Ivanov
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

#include "ItemSelectionModel.h"

#include <lib/model/common/AbstractItemModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <utility>

namespace quentier {

ItemSelectionModel::ItemSelectionModel(
    AbstractItemModel * model, QObject * parent) :
    QItemSelectionModel{model, parent}
{}

void ItemSelectionModel::select(
    const QItemSelection & selection,
    QItemSelectionModel::SelectionFlags command)
{
    if (!selectImpl(selection, command)) {
        QItemSelectionModel::select(selection, command);
    }
}

bool ItemSelectionModel::selectImpl(
    const QItemSelection & selection,
    QItemSelectionModel::SelectionFlags command)
{
    QNDEBUG(
        "view::ItemSelectionModel",
        "ItemSelectionModel::selectImpl: "
            << "command = " << command);

    auto * itemModel = qobject_cast<AbstractItemModel *>(model());
    if (!itemModel) {
        QNDEBUG(
            "view::ItemSelectionModel",
            "Not an AbstractItemModel subclass is used");
        return false;
    }

    const auto allItemsRootItemIndex = itemModel->allItemsRootItemIndex();
    if (!allItemsRootItemIndex.isValid()) {
        QNDEBUG(
            "view::ItemSelectionModel", "No valid all items root item index");
        return false;
    }

    const auto currentIndexes = selectedIndexes();
    auto newIndexes = selection.indexes();

    if (currentIndexes.contains(allItemsRootItemIndex)) {
        if (newIndexes.size() > 1 ||
            (!command.testFlag(QItemSelectionModel::Clear) &&
             !command.testFlag(QItemSelectionModel::ClearAndSelect)))
        {
            QNDEBUG(
                "view::ItemSelectionModel",
                "Filtering out all items root item index");
            // Need to filter out all items root item index from the new
            // selection
            auto i = newIndexes.indexOf(allItemsRootItemIndex);
            if (i >= 0) {
                newIndexes.removeAt(i);
            }

            QItemSelection newSelection;
            for (const auto & newIndex: std::as_const(newIndexes)) {
                newSelection.select(newIndex, newIndex);
            }

            QItemSelectionModel::select(
                newSelection,
                QItemSelectionModel::ClearAndSelect |
                    QItemSelectionModel::Current | QItemSelectionModel::Rows);
            return true;
        }

        return false;
    }

    if (newIndexes.contains(allItemsRootItemIndex)) {
        QNDEBUG(
            "view::ItemSelectionModel", "Selecting only all items root item");
        // All items root item was selected, need to remove everything else
        // from the new selection
        QItemSelection newSelection;
        newSelection.select(allItemsRootItemIndex, allItemsRootItemIndex);
        QItemSelectionModel::select(
            newSelection,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current |
                QItemSelectionModel::Rows);
        return true;
    }

    return false;
}

} // namespace quentier
