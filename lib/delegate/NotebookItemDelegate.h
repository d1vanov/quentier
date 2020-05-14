/*
 * Copyright 2016-2019 Dmitry Ivanov
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

QT_FORWARD_DECLARE_CLASS(INotebookModelItem)

class NotebookItemDelegate: public AbstractStyledItemDelegate
{
    Q_OBJECT
public:
    explicit NotebookItemDelegate(QObject * parent = nullptr);

    int sideSize() const;

private:
    // QStyledItemDelegate interface
    virtual QString displayText(
        const QVariant & value, const QLocale & locale) const override;

    virtual QWidget * createEditor(
        QWidget * parent, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    virtual void paint(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    virtual void setEditorData(
        QWidget * editor, const QModelIndex & index) const override;

    virtual void setModelData(
        QWidget * editor, QAbstractItemModel * model,
        const QModelIndex & index) const override;

    virtual QSize sizeHint(
        const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

    virtual void updateEditorGeometry(
        QWidget * editor, const QStyleOptionViewItem & option,
        const QModelIndex & index) const override;

private:
    const INotebookModelItem * notebookModelItem(
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

    QSize notebookNameSizeHint(
        const QStyleOptionViewItem & option, const QModelIndex & index,
        const int columnNameWidth) const;

private:
    QIcon   m_userIcon;
    QSize   m_userIconSize;
};

} // namespace quentier

#endif // QUENTIER_LIB_DELEGATE_NOTEBOOK_ITEM_DELEGATE_H
