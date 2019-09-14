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

#ifndef QUENTIER_LIB_DELEGATE_LIMITED_FONTS_DELEGATE_H
#define QUENTIER_LIB_DELEGATE_LIMITED_FONTS_DELEGATE_H

#include <quentier/utility/Macros.h>

#include <QStyledItemDelegate>

namespace quentier {

class LimitedFontsDelegate: public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit LimitedFontsDelegate(QObject * parent = Q_NULLPTR);

    virtual void paint(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const Q_DECL_OVERRIDE;

private:
    bool doPaint(
        QPainter * painter, const QStyleOptionViewItem & option,
        const QModelIndex & index) const;
};

} // namespace quentier

#endif // QUENTIER_LIB_DELEGATE_LIMITED_FONTS_DELEGATE_H
