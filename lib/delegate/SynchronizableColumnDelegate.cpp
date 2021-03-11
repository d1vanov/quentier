/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#include "SynchronizableColumnDelegate.h"

#include <QCheckBox>
#include <QPainter>

namespace quentier {

namespace {

constexpr int gSideSize = 8;

} // namespace

SynchronizableColumnDelegate::SynchronizableColumnDelegate(QObject * parent) :
    QStyledItemDelegate(parent), m_iconSize(gSideSize, gSideSize)
{
    m_icon.addFile(
        QStringLiteral(":/sync_icons/stat_notify_sync.png"), m_iconSize);
}

int SynchronizableColumnDelegate::sideSize() const
{
    return qRound(gSideSize * 1.25);
}

QString SynchronizableColumnDelegate::displayText(
    const QVariant & value, const QLocale & locale) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)
    return {};
}

QWidget * SynchronizableColumnDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(option)

    if (Q_UNLIKELY(!index.isValid())) {
        return nullptr;
    }

    const auto * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return nullptr;
    }

    const bool synchronizable = model->data(index).toBool();
    if (synchronizable) {
        // The item which is already synchronizable cannot be made
        // non-synchronizable
        return nullptr;
    }

    auto * checkbox = new QCheckBox(parent);
    checkbox->setCheckState(Qt::Unchecked);

    return checkbox;
}

void SynchronizableColumnDelegate::paint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    bool synchronizable = false;

    const auto * model = index.model();
    if (model) {
        synchronizable = model->data(index).toBool();
    }

    if (synchronizable) {
        m_icon.paint(painter, option.rect);
    }
    else {
        painter->setBrush(QBrush(Qt::magenta));
        const int side = std::min(option.rect.width(), option.rect.height());
        constexpr int nonSynchronizableCircleRadius = 2;
        const int radius = std::min(side, nonSynchronizableCircleRadius);
        const int diameter = 2 * radius;
        const auto center = option.rect.center();

        painter->drawEllipse(QRectF(
            center.x() - radius, center.y() - radius, diameter, diameter));
    }

    painter->restore();
}

void SynchronizableColumnDelegate::setEditorData(
    QWidget * editor, const QModelIndex & index) const
{
    auto * checkbox = qobject_cast<QCheckBox *>(editor);
    if (Q_UNLIKELY(!checkbox)) {
        return;
    }

    const auto * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return;
    }

    const bool synchronizable = model->data(index).toBool();
    checkbox->setCheckState(synchronizable ? Qt::Checked : Qt::Unchecked);
}

void SynchronizableColumnDelegate::setModelData(
    QWidget * editor, QAbstractItemModel * model,
    const QModelIndex & index) const
{
    if (Q_UNLIKELY(!model)) {
        return;
    }

    if (Q_UNLIKELY(!editor)) {
        return;
    }

    auto * checkbox = qobject_cast<QCheckBox *>(editor);
    if (Q_UNLIKELY(!checkbox)) {
        return;
    }

    const bool synchronizable = (checkbox->checkState() == Qt::Checked);
    model->setData(index, synchronizable, Qt::EditRole);
}

QSize SynchronizableColumnDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option)

    if (Q_UNLIKELY(!index.isValid())) {
        return {};
    }

    return m_iconSize;
}

void SynchronizableColumnDelegate::updateEditorGeometry(
    QWidget * editor, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

} // namespace quentier
