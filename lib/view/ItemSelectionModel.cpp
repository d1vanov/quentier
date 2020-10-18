/*
 * Copyright 2020 Dmitry Ivanov
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

#include <lib/model/common/ItemModel.h>

#include <quentier/logging/QuentierLogger.h>

namespace quentier {

ItemSelectionModel::ItemSelectionModel(ItemModel * pModel, QObject * parent) :
    QItemSelectionModel(pModel, parent)
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
        "view:item_selection_model",
        "ItemSelectionModel::selectImpl: "
            << "command = " << command);

    auto * pItemModel = qobject_cast<ItemModel *>(model());
    if (!pItemModel) {
        QNDEBUG(
            "view:item_selection_model", "Not an ItemModel subclass is used");
        return false;
    }

    const auto allItemsRootItemIndex = pItemModel->allItemsRootItemIndex();
    if (!allItemsRootItemIndex.isValid()) {
        QNDEBUG(
            "view:item_selection_model", "No valid all items root item index");
        return false;
    }

    auto currentIndexes = selectedIndexes();
    auto newIndexes = selection.indexes();

    if (currentIndexes.contains(allItemsRootItemIndex)) {
        if (newIndexes.size() > 1 ||
            (!command.testFlag(QItemSelectionModel::Clear) &&
             !command.testFlag(QItemSelectionModel::ClearAndSelect)))
        {
            QNDEBUG(
                "view:item_selection_model",
                "Filtering out all items root item index");
            // Need to filter out all items root item index from the new
            // selection
            auto i = newIndexes.indexOf(allItemsRootItemIndex);
            if (i >= 0) {
                newIndexes.removeAt(i);
            }

            QItemSelection newSelection;
            for (const auto & newIndex: qAsConst(newIndexes)) {
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
            "view:item_selection_model", "Selecting only all items root item");
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
