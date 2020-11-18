/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_DELEGATE_SYNCHRONIZABLE_COLUMN_DELEGATE_H
#define QUENTIER_LIB_DELEGATE_SYNCHRONIZABLE_COLUMN_DELEGATE_H

#include <QIcon>
#include <QStyledItemDelegate>

class SynchronizableColumnDelegate final : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit SynchronizableColumnDelegate(QObject * parent = nullptr);

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
    QIcon m_icon;
    QSize m_iconSize;
};

#endif // QUENTIER_LIB_DELEGATE_SYNCHRONIZABLE_COLUMN_DELEGATE_H
