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

#include "FromLinkedNotebookColumnDelegate.h"
#include <QPainter>
#include <algorithm>
#include <cmath>

#define ICON_SIDE_SIZE (16)

namespace quentier {

FromLinkedNotebookColumnDelegate::FromLinkedNotebookColumnDelegate(
        QObject * parent) :
    AbstractStyledItemDelegate(parent),
    m_icon(),
    m_iconSize(ICON_SIDE_SIZE, ICON_SIDE_SIZE)
{
    m_icon.addFile(QStringLiteral(":/user/user.png"), m_iconSize);
}

int FromLinkedNotebookColumnDelegate::sideSize() const
{
    return qRound(ICON_SIDE_SIZE * 1.25);
}

QString FromLinkedNotebookColumnDelegate::displayText(
    const QVariant & value, const QLocale & locale) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)
    return QString();
}

QWidget * FromLinkedNotebookColumnDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void FromLinkedNotebookColumnDelegate::paint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    bool fromLinkedNotebook = false;

    const QAbstractItemModel * model = index.model();
    if (model)
    {
        QVariant data = model->data(index);

        // The data here might be a string - linked notebook guid - or just
        // a bool indicating whether the corresponding item is from linked
        // notebook or not
        if (data.type() == QVariant::String) {
            fromLinkedNotebook = !data.toString().isEmpty();
        }
        else {
            fromLinkedNotebook = data.toBool();
        }
    }

    if (fromLinkedNotebook) {
        m_icon.paint(painter, option.rect);
    }

    painter->restore();
}

void FromLinkedNotebookColumnDelegate::setEditorData(
    QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void FromLinkedNotebookColumnDelegate::setModelData(
    QWidget * editor, QAbstractItemModel * model,
    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize FromLinkedNotebookColumnDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option)

    if (Q_UNLIKELY(!index.isValid())) {
        return QSize();
    }

    int colNameWidth = columnNameWidth(option, index);
    int width = std::max(m_iconSize.width(), colNameWidth);
    return QSize(width, m_iconSize.height());
}

void FromLinkedNotebookColumnDelegate::updateEditorGeometry(
    QWidget * editor, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

} // namespace quentier
