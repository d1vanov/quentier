/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_DELEGATE_NOTEBOOK_ITEM_DELEGATE_H
#define QUENTIER_LIB_DELEGATE_NOTEBOOK_ITEM_DELEGATE_H

#include "AbstractStyledItemDelegate.h"

#include <QIcon>
#include <QSize>

namespace quentier {

class INotebookModelItem;

class NotebookItemDelegate final: public AbstractStyledItemDelegate
{
    Q_OBJECT
public:
    explicit NotebookItemDelegate(QObject * parent = nullptr);

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
    [[nodiscard]] const INotebookModelItem * notebookModelItem(
        const QModelIndex & index) const;

    void paintItem(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index, const INotebookModelItem & item) const;

    void drawEllipse(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const;

    void drawNotebookName(
        QPainter * painter, const QModelIndex & index,
        const QStyleOptionViewItem & option) const;

    [[nodiscard]] QSize notebookNameSizeHint(
        const QStyleOptionViewItem & option, const QModelIndex & index,
        int columnNameWidth) const;

private:
    QIcon m_userIcon;
    QSize m_userIconSize;
};

} // namespace quentier

#endif // QUENTIER_LIB_DELEGATE_NOTEBOOK_ITEM_DELEGATE_H
