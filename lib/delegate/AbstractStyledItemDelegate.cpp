/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#include <QFontMetrics>

#include <algorithm>
#include <cmath>

namespace quentier {

namespace {

////////////////////////////////////////////////////////////////////////////////

int fontMetricsWidth(const QFontMetrics & metrics, const QString & text)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    return metrics.horizontalAdvance(text);
#else
    return metrics.width(text);
#endif
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

AbstractStyledItemDelegate::AbstractStyledItemDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

AbstractStyledItemDelegate::~AbstractStyledItemDelegate()
{}

int AbstractStyledItemDelegate::columnNameWidth(
    const QStyleOptionViewItem & option,
    const QModelIndex & index,
    const Qt::Orientation orientation) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNTRACE("Can't determine the column name width: the model is null");
        return -1;
    }

    if (Q_UNLIKELY(!index.isValid())) {
        QNTRACE("Can't determine the column name width for invalid model index");
        return -1;
    }

    int column = index.column();
    if (Q_UNLIKELY(model->columnCount(index.parent()) <= column)) {
        QNTRACE("Can't determine the column name width: index's "
                "column number is beyond the range of model's columns");
        return -1;
    }

    QString columnName =
        model->headerData(column, orientation, Qt::DisplayRole).toString();
    if (Q_UNLIKELY(columnName.isEmpty())) {
        QNTRACE("Can't determine the column name width: model "
                "returned empty header data");
        return -1;
    }

    QFontMetrics fontMetrics(option.font);
    double margin = 0.1;

    int columnNameWidth = static_cast<int>(
        std::floor(fontMetricsWidth(fontMetrics, columnName) * (1.0 + margin) + 0.5));

    return columnNameWidth;
}

void AbstractStyledItemDelegate::adjustDisplayedText(
    QString & displayedText, const QStyleOptionViewItem & option,
    const QString & nameSuffix) const
{
    QFontMetrics fontMetrics(option.font);

    int displayedTextWidth = fontMetricsWidth(fontMetrics, displayedText);

    int nameSuffixWidth = (nameSuffix.isEmpty()
        ? 0
        : fontMetricsWidth(fontMetrics, nameSuffix));

    int optionRectWidth = option.rect.width();

    // Shorten the available width a tiny bit to ensure there's some margin and
    // there're no weird rendering glitches
    optionRectWidth -= 2;
    optionRectWidth = std::max(optionRectWidth, 0);

    if ((displayedTextWidth + nameSuffixWidth) <= optionRectWidth) {
        return;
    }

    int idealDisplayedTextWidth = optionRectWidth - nameSuffixWidth;
    displayedText = fontMetrics.elidedText(displayedText, Qt::ElideRight,
                                           idealDisplayedTextWidth);
}

} // namespace quentier
