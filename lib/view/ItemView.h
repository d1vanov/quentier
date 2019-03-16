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

#ifndef QUENTIER_LIB_VIEW_ITEM_VIEW_H
#define QUENTIER_LIB_VIEW_ITEM_VIEW_H

#include <quentier/utility/Macros.h>

#include <QTreeView>

namespace quentier {

/**
 * @brief The ItemView class provides somewhat extended version of QTreeView
 * (can be used with tree or table models) which works around different quirks
 * of the default implementations like column sizing
 */
class ItemView: public QTreeView
{
    Q_OBJECT
public:
    explicit ItemView(QWidget * parent = Q_NULLPTR);

public Q_SLOTS:
    /**
     * @brief dataChanged slot redefines the QTreeView's implementation: it
     * forces the columns affected by the data change to be automatically resized
     * after the change was processed; the QTreeView's implementation does not
     * do so
     * @param topLeft       Top left model index of the changed data
     * @param bottomRight   Bottom right model index of the changed data
     * @param roles         The roles under which the data has been changed -
     *                      this parameter is present only if Qt5 is used, not Qt4
     *
     * @warning: don't attempt to use QT_VERSION_CHECK here to detect the Qt
     * version properly: Qt4 doesn't expand macro functions during the moc run
     * on headers which leads to the check not working and the code not building
     * with Qt4
     */
    virtual void dataChanged(const QModelIndex & topLeft,
                             const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                            )
#else
                            , const QVector<int> & roles = QVector<int>())
#endif
                            Q_DECL_OVERRIDE;

protected:
    /**
     * @return              The valid index if all indexes in the list point to
     *                      the same row or invalid model index otherwise
     */
    QModelIndex singleRow(const QModelIndexList & indexes,
                          const QAbstractItemModel & model,
                          const int column) const;
};

} // namespace quentier

#endif // QUENTIER_LIB_VIEW_ITEM_VIEW_H
