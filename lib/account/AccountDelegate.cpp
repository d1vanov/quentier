/*
 * Copyright 2018-2024 Dmitry Ivanov
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

#include "AccountDelegate.h"
#include "AccountModel.h"

#include <QFontMetrics>
#include <QLineEdit>

namespace quentier {

AccountDelegate::AccountDelegate(QObject * parent) : QStyledItemDelegate{parent}
{}

QWidget * AccountDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (!index.isValid() ||
        index.column() != static_cast<int>(AccountModel::Column::DisplayName))
    {
        return nullptr;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void AccountDelegate::setEditorData(
    QWidget * editor, const QModelIndex & index) const
{
    if (!index.isValid() ||
        index.column() != static_cast<int>(AccountModel::Column::DisplayName))
    {
        return;
    }

    QLineEdit * lineEdit = qobject_cast<QLineEdit *>(editor);
    if (lineEdit) {
        QString displayName = index.data().toString();
        lineEdit->setText(displayName);
        lineEdit->selectAll();
    }
}

QSize AccountDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (!index.isValid()) {
        return QSize{};
    }

    const QString str = index.data().toString();
    QSize size = option.fontMetrics.size(Qt::TextSingleLine, str);
    size.rheight() += 2;
    size.rwidth() += 2;
    return size;
}

} // namespace quentier
