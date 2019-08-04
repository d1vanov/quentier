/*
 * Copyright 2018-2019 Dmitry Ivanov
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

#include <QLineEdit>
#include <QFontMetrics>

namespace quentier {

AccountDelegate::AccountDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

QWidget * AccountDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (!index.isValid() ||
        (index.column() != AccountModel::Columns::DisplayName))
    {
        return Q_NULLPTR;
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

void AccountDelegate::setEditorData(
    QWidget * pEditor, const QModelIndex & index) const
{
    if (!index.isValid() ||
        (index.column() != AccountModel::Columns::DisplayName))
    {
        return;
    }

    QLineEdit * pLineEdit = qobject_cast<QLineEdit*>(pEditor);
    if (pLineEdit) {
        QString displayName = index.data().toString();
        pLineEdit->setText(displayName);
        pLineEdit->selectAll();
    }
}

QSize AccountDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (!index.isValid()) {
        return QSize();
    }

    QString str = index.data().toString();
    QSize size = option.fontMetrics.size(Qt::TextSingleLine, str);
    size.rheight() += 2;
    size.rwidth() += 2;
    return size;
}

} // namespace quentier
