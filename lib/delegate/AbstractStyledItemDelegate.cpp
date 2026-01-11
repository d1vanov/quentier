/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include "AbstractStyledItemDelegate.h"

#include <quentier/logging/QuentierLogger.h>

#include <algorithm>
#include <cmath>

namespace quentier {

AbstractStyledItemDelegate::AbstractStyledItemDelegate(QObject * parent) :
    QStyledItemDelegate{parent}
{}

AbstractStyledItemDelegate::~AbstractStyledItemDelegate() = default;

int AbstractStyledItemDelegate::columnNameWidth(
    const QStyleOptionViewItem & option, const QModelIndex & index,
    const Qt::Orientation orientation) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNTRACE(
            "delegate::AbstractStyleItemDelegate",
            "Can't determine the column name width: the model is null");
        return -1;
    }

    if (Q_UNLIKELY(!index.isValid())) {
        QNTRACE(
            "delegate::AbstractStyleItemDelegate",
            "Can't determine the column name width for invalid "
                << "model index");
        return -1;
    }

    const int column = index.column();
    if (Q_UNLIKELY(model->columnCount(index.parent()) <= column)) {
        QNTRACE(
            "delegate::AbstractStyleItemDelegate",
            "Can't determine the column name width: index's "
                << "column number is beyond the range of model's columns");
        return -1;
    }

    const QString columnName =
        model->headerData(column, orientation, Qt::DisplayRole).toString();

    if (Q_UNLIKELY(columnName.isEmpty())) {
        QNTRACE(
            "delegate::AbstractStyleItemDelegate",
            "Can't determine the column name width: model "
                << "returned empty header data");
        return -1;
    }

    const QFontMetrics fontMetrics{option.font};
    double margin = 0.1;

    return static_cast<int>(std::floor(
        fontMetrics.horizontalAdvance(columnName) * (1.0 + margin) + 0.5));
}

void AbstractStyledItemDelegate::adjustDisplayedText(
    QString & displayedText, const QStyleOptionViewItem & option,
    const QString & nameSuffix) const
{
    const QFontMetrics fontMetrics{option.font};
    const int displayedTextWidth = fontMetrics.horizontalAdvance(displayedText);

    const int nameSuffixWidth =
        (nameSuffix.isEmpty() ? 0 : fontMetrics.horizontalAdvance(nameSuffix));

    int optionRectWidth = option.rect.width();

    // Shorten the available width a tiny bit to ensure there's some margin and
    // there're no weird rendering glitches
    optionRectWidth -= 2;
    optionRectWidth = std::max(optionRectWidth, 0);

    if ((displayedTextWidth + nameSuffixWidth) <= optionRectWidth) {
        return;
    }

    const int idealDisplayedTextWidth = optionRectWidth - nameSuffixWidth;

    displayedText = fontMetrics.elidedText(
        displayedText, Qt::ElideRight, idealDisplayedTextWidth);
}

} // namespace quentier
