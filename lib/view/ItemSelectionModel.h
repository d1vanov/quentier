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

#ifndef QUENTIER_LIB_VIEW_ITEM_SELECTION_MODEL_H
#define QUENTIER_LIB_VIEW_ITEM_SELECTION_MODEL_H

#include <QItemSelectionModel>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AbstractItemModel)

/**
 * @brief The ItemSelectionModel class represents a small customization
 * of the standard QItemSelectionModel
 */
class ItemSelectionModel final : public QItemSelectionModel
{
    Q_OBJECT
public:
    explicit ItemSelectionModel(
        AbstractItemModel * pModel, QObject * parent = nullptr);

    virtual ~ItemSelectionModel() override = default;

public Q_SLOTS:
    /**
     * @brief select method filters out the input selection in order to ensure
     * that for extended selection either solely the root item representing
     * all model items is selected or actual model items but not both
     *
     * NOTE: the filtering occurs only if model() returns an instance of
     * AbstractItemModel, otherwise the call is forwarded to
     * QItemSelectionModel's method.
     */
    virtual void select(
        const QItemSelection & selection,
        QItemSelectionModel::SelectionFlags command) override;

private:
    bool selectImpl(
        const QItemSelection & selection,
        QItemSelectionModel::SelectionFlags command);
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_ITEM_SELECTION_MODEL_H
