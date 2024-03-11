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

#include "FavoriteItemDelegate.h"

#include <lib/model/favorites/FavoritesModel.h>
#include <lib/model/favorites/FavoritesModelItem.h>
#include <lib/model/notebook/NotebookModel.h>
#include <lib/model/tag/TagModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QPainter>

#include <algorithm>

namespace quentier {

namespace {

constexpr int gIconSideSize = 16;

} // namespace

FavoriteItemDelegate::FavoriteItemDelegate(QObject * parent) :
    AbstractStyledItemDelegate(parent),
    m_notebookIcon{QIcon::fromTheme(QStringLiteral("x-office-address-book"))},
    m_noteIcon{QIcon::fromTheme(QStringLiteral("x-office-document"))},
    m_savedSearchIcon{QIcon::fromTheme(QStringLiteral("edit-find"))},
    m_unknownTypeIcon{QIcon::fromTheme(QStringLiteral("image-missing"))},
    m_iconSize{gIconSideSize, gIconSideSize}
{
    m_tagIcon.addFile(QStringLiteral(":/tag/tag.png"), m_iconSize);
}

int FavoriteItemDelegate::sideSize() const noexcept
{
    return gIconSideSize;
}

QString FavoriteItemDelegate::displayText(
    const QVariant & value, const QLocale & locale) const
{
    return AbstractStyledItemDelegate::displayText(value, locale);
}

QWidget * FavoriteItemDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    // Only allow to edit the favorited item's display name but no other
    // favorites model columns
    if (index.isValid() &&
        (index.column() ==
         static_cast<int>(FavoritesModel::Column::DisplayName)))
    {
        return AbstractStyledItemDelegate::createEditor(parent, option, index);
    }
    else {
        return nullptr;
    }
}

void FavoriteItemDelegate::paint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    const auto * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return;
    }

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing);

    const int column = index.column();
    if (column == static_cast<int>(FavoritesModel::Column::Type)) {
        QVariant type = model->data(index);
        bool conversionResult = false;
        const int typeInt = type.toInt(&conversionResult);
        if (conversionResult) {
            if (option.state & QStyle::State_Selected) {
                painter->fillRect(option.rect, option.palette.highlight());
            }

            switch (typeInt) {
            case static_cast<int>(FavoritesModelItem::Type::Notebook):
                m_notebookIcon.paint(painter, option.rect);
                break;
            case static_cast<int>(FavoritesModelItem::Type::Tag):
                m_tagIcon.paint(painter, option.rect);
                break;
            case static_cast<int>(FavoritesModelItem::Type::Note):
                m_noteIcon.paint(painter, option.rect);
                break;
            case static_cast<int>(FavoritesModelItem::Type::SavedSearch):
                m_savedSearchIcon.paint(painter, option.rect);
                break;
            default:
                m_unknownTypeIcon.paint(painter, option.rect);
                break;
            }
        }
    }
    else if (column == static_cast<int>(FavoritesModel::Column::DisplayName)) {
        drawFavoriteItemName(painter, index, option);
    }

    painter->restore();
}

void FavoriteItemDelegate::setEditorData(
    QWidget * editor, const QModelIndex & index) const
{
    if (index.isValid() &&
        (index.column() ==
         static_cast<int>(FavoritesModel::Column::DisplayName)))
    {
        AbstractStyledItemDelegate::setEditorData(editor, index);
    }
}

void FavoriteItemDelegate::setModelData(
    QWidget * editor, QAbstractItemModel * model,
    const QModelIndex & index) const
{
    if (index.isValid() &&
        (index.column() ==
         static_cast<int>(FavoritesModel::Column::DisplayName)))
    {
        AbstractStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize FavoriteItemDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const int column = index.column();
    if (column == static_cast<int>(FavoritesModel::Column::Type)) {
        QSize size = m_iconSize;
        // Some margin so that the icon is not tightly near the text
        size.setWidth(size.width() + 15);
        return size;
    }

    return favoriteItemNameSizeHint(option, index);
}

void FavoriteItemDelegate::updateEditorGeometry(
    QWidget * editor, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (index.isValid() &&
        (index.column() ==
         static_cast<int>(FavoritesModel::Column::DisplayName)))
    {
        AbstractStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}

QSize FavoriteItemDelegate::favoriteItemNameSizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const auto * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    const QString name = model->data(index).toString().simplified();
    if (Q_UNLIKELY(name.isEmpty())) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    QString nameSuffix;

    const QModelIndex itemTypeIndex = model->index(
        index.row(), static_cast<int>(FavoritesModel::Column::Type),
        index.parent());

    const QVariant itemType = model->data(itemTypeIndex);
    bool conversionResult = false;

    const auto itemTypeInt = itemType.toInt(&conversionResult);

    if (conversionResult &&
        (itemTypeInt == static_cast<int>(FavoritesModelItem::Type::Notebook) ||
         itemTypeInt == static_cast<int>(FavoritesModelItem::Type::Tag)))
    {
        const QModelIndex noteCountIndex = model->index(
            index.row(), static_cast<int>(FavoritesModel::Column::NoteCount),
            index.parent());

        const QVariant noteCount = model->data(noteCountIndex);
        conversionResult = false;
        const int noteCountInt = noteCount.toInt(&conversionResult);
        if (conversionResult && (noteCountInt > 0)) {
            nameSuffix = QStringLiteral(" (");
            nameSuffix += QString::number(noteCountInt);
            nameSuffix += QStringLiteral(")");
        }
    }

    const QFontMetrics fontMetrics{option.font};
    const int nameWidth = fontMetrics.horizontalAdvance(name + nameSuffix);
    const int fontHeight = fontMetrics.height();

    constexpr double margin = 1.1;

    return QSize{
        std::max(
            static_cast<int>(std::floor(nameWidth * margin + 0.5)),
            option.rect.width()),
        std::max(
            static_cast<int>(std::floor(fontHeight * margin + 0.5)),
            option.rect.height())};
}

void FavoriteItemDelegate::drawFavoriteItemName(
    QPainter * painter, const QModelIndex & index,
    const QStyleOptionViewItem & option) const
{
    const auto * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return;
    }

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    QString name = model->data(index).toString().simplified();
    if (name.isEmpty()) {
        QNDEBUG(
            "delegate",
            "FavoriteItemDelegate::drawFavoriteItemName: item name is empty");
        return;
    }

    QString nameSuffix;

    const QModelIndex noteCountIndex = model->index(
        index.row(), static_cast<int>(FavoritesModel::Column::NoteCount),
        index.parent());

    const QVariant noteCount = model->data(noteCountIndex);
    bool conversionResult = false;
    const int noteCountInt = noteCount.toInt(&conversionResult);
    if (conversionResult && (noteCountInt > 0)) {
        nameSuffix = QStringLiteral(" (");
        nameSuffix += QString::number(noteCountInt);
        nameSuffix += QStringLiteral(")");
    }

    adjustDisplayedText(name, option, nameSuffix);

    painter->setPen(
        option.state & QStyle::State_Selected
            ? option.palette.highlightedText().color()
            : option.palette.windowText().color());

    painter->drawText(
        option.rect, name,
        QTextOption{Qt::Alignment{Qt::AlignLeft | Qt::AlignVCenter}});

    if (nameSuffix.isEmpty()) {
        return;
    }

    const QFontMetrics fontMetrics{option.font};
    const int nameWidth = fontMetrics.horizontalAdvance(name);

    painter->setPen(
        option.state & QStyle::State_Selected
            ? option.palette.color(QPalette::Active, QPalette::WindowText)
            : option.palette.color(QPalette::Active, QPalette::Highlight));

    painter->drawText(
        option.rect.translated(nameWidth, 0), nameSuffix,
        QTextOption{Qt::Alignment{Qt::AlignLeft | Qt::AlignVCenter}});
}

} // namespace quentier
