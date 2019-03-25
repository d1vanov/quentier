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

#ifndef QUENTIER_LIB_DELEGATE_DIRTY_COLUMN_DELEGATE_H
#define QUENTIER_LIB_DELEGATE_DIRTY_COLUMN_DELEGATE_H

#include "AbstractStyledItemDelegate.h"

namespace quentier {

class DirtyColumnDelegate: public AbstractStyledItemDelegate
{
    Q_OBJECT
public:
    explicit DirtyColumnDelegate(QObject * parent = Q_NULLPTR);

    int sideSize() const;

private:
    // QStyledItemDelegate interface
    virtual QString displayText(const QVariant & value,
                                const QLocale & locale) const Q_DECL_OVERRIDE;

    virtual QWidget * createEditor(QWidget * parent,
                                   const QStyleOptionViewItem & option,
                                   const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual void paint(QPainter * painter,
                       const QStyleOptionViewItem & option,
                       const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual void setEditorData(QWidget * editor,
                               const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual void setModelData(QWidget * editor,
                              QAbstractItemModel * model,
                              const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual QSize sizeHint(const QStyleOptionViewItem & option,
                           const QModelIndex & index) const Q_DECL_OVERRIDE;

    virtual void updateEditorGeometry(QWidget * editor,
                                      const QStyleOptionViewItem & option,
                                      const QModelIndex & index) const Q_DECL_OVERRIDE;
};

} // namespace quentier

#endif // QUENTIER_LIB_DELEGATE_DIRTY_COLUMN_DELEGATE_H
