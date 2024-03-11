/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#pragma once

#include "AbstractStyledItemDelegate.h"

#include <QIcon>

namespace quentier {

class FavoriteItemDelegate : public AbstractStyledItemDelegate
{
    Q_OBJECT
public:
    explicit FavoriteItemDelegate(QObject * parent = nullptr);

    [[nodiscard]] int sideSize() const noexcept;

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
    [[nodiscard]] QSize favoriteItemNameSizeHint(
        const QStyleOptionViewItem & option, const QModelIndex & index) const;

    void drawFavoriteItemName(
        QPainter * painter, const QModelIndex & index,
        const QStyleOptionViewItem & option) const;

private:
    const QIcon m_notebookIcon;
    const QIcon m_noteIcon;
    const QIcon m_savedSearchIcon;
    const QIcon m_unknownTypeIcon;
    const QSize m_iconSize;
    QIcon m_tagIcon;
};

} // namespace quentier
