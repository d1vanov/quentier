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

#ifndef QUENTIER_LIB_DELEGATE_FROM_LINKED_NOTEBOOK_COLUMN_DELEGATE_H
#define QUENTIER_LIB_DELEGATE_FROM_LINKED_NOTEBOOK_COLUMN_DELEGATE_H

#include "AbstractStyledItemDelegate.h"

#include <QIcon>

namespace quentier {

/**
 * @brief The FromLinkedNotebookColumnDelegate class, as its name suggests,
 * is a custom delegate for the model column providing a boolean indicating
 * whether the model item belongs to the linked notebook
 */
class FromLinkedNotebookColumnDelegate final: public AbstractStyledItemDelegate
{
    Q_OBJECT
public:
    explicit FromLinkedNotebookColumnDelegate(QObject * parent = nullptr);

    [[nodiscard]] int sideSize() const;

private:
    // QStyledItemDelegate interface
    [[nodiscard]] QString displayText(
        const QVariant & value, const QLocale & locale) const override;

    [[nodiscard]] QWidget * createEditor(
        QWidget * parent, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    void paint(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    void setEditorData(
        QWidget * editor, const QModelIndex & index) const override;

    void setModelData(
        QWidget * editor, QAbstractItemModel * model,
        const QModelIndex & index) const override;

    [[nodiscard]] QSize sizeHint(
        const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    void updateEditorGeometry(
        QWidget * editor, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

private:
    QIcon m_icon;
    QSize m_iconSize;
};

} // namespace quentier

#endif // QUENTIER_LIB_DELEGATE_FROM_LINKED_NOTEBOOK_COLUMN_DELEGATE_H
