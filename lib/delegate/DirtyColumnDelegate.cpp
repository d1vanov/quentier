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

#include "DirtyColumnDelegate.h"

#include <QPainter>

#include <algorithm>
#include <cmath>

namespace quentier {

namespace {

constexpr int gDirtyCircleRadius = 2;

} // namespace

DirtyColumnDelegate::DirtyColumnDelegate(QObject * parent) :
    AbstractStyledItemDelegate{parent}
{}

int DirtyColumnDelegate::sideSize() const noexcept
{
    return qRound(gDirtyCircleRadius * 2.1125);
}

QString DirtyColumnDelegate::displayText(
    const QVariant & value, const QLocale & locale) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)
    return QString{};
}

QWidget * DirtyColumnDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return nullptr;
}

void DirtyColumnDelegate::paint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    const auto data = index.data();
    if (!data.isValid()) {
        return;
    }

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    const bool dirty = data.toBool();
    if (dirty) {
        painter->setBrush(QBrush(Qt::red));
    }
    else {
        painter->setBrush(QBrush(Qt::green));
    }

    const int colNameWidth = columnNameWidth(option, index);
    const int side = std::min(option.rect.width(), option.rect.height());
    const int radius = std::min(side, gDirtyCircleRadius);
    const double diameter = 2.0 * radius;

    QPoint center = option.rect.center();
    center.setX(std::min(
        center.x(),
        (option.rect.left() + std::max(colNameWidth, side) / 2 + 1)));

    painter->setPen(QColor());

    painter->drawEllipse(QRectF{
        static_cast<double>(center.x() - radius),
        static_cast<double>(center.y() - radius), diameter, diameter});

    painter->restore();
}

void DirtyColumnDelegate::setEditorData(
    QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void DirtyColumnDelegate::setModelData(
    QWidget * editor, QAbstractItemModel * model,
    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize DirtyColumnDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option)

    if (Q_UNLIKELY(!index.isValid())) {
        return {};
    }

    const int column = index.column();

    QString columnName;
    const auto * model = index.model();
    if (Q_LIKELY(model && (model->columnCount(index.parent()) > column))) {
        // NOTE: assuming the delegate would only be used in horizontal layouts
        columnName = model->headerData(column, Qt::Horizontal).toString();
    }

    QFontMetrics fontMetrics{option.font};
    double margin = 0.1;

    const int columnNameWidth = static_cast<int>(std::floor(
        fontMetrics.horizontalAdvance(columnName) * (1.0 + margin) + 0.5));

    int side = gDirtyCircleRadius;
    side += 1;
    side *= 2;

    const int width = std::max(side, columnNameWidth);
    return QSize{width, side};
}

void DirtyColumnDelegate::updateEditorGeometry(
    QWidget * editor, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

} // namespace quentier
