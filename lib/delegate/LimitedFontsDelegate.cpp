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

#include "LimitedFontsDelegate.h"

#include <QFont>
#include <QPainter>

namespace quentier {

LimitedFontsDelegate::LimitedFontsDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

void LimitedFontsDelegate::paint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }
    else {
        painter->fillRect(option.rect, option.palette.base());
    }

    if (!doPaint(painter, option, index)) {
        QStyledItemDelegate::paint(painter, option, index);
    }

    painter->restore();
}

bool LimitedFontsDelegate::doPaint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (!index.isValid()) {
        return false;
    }

    QVariant data = index.data(Qt::DisplayRole);
    if (data.isNull() || !data.isValid()) {
        return false;
    }

    QString fontFamily = data.toString();
    if (Q_UNLIKELY(fontFamily.isEmpty())) {
        return false;
    }

    QFont font;
    font.setFamily(fontFamily);
    painter->setFont(font);

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.highlightedText().color()
                    : option.palette.windowText().color());

    painter->drawText(option.rect, fontFamily,
                      QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
    return true;
}

} // namespace quentier
