/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include "TagItemDelegate.h"

#include <lib/model/tag/TagModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QFontMetrics>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace quentier {

namespace {

constexpr int gIconSideSize = 16;

} // namespace

TagItemDelegate::TagItemDelegate(QObject * parent) :
    AbstractStyledItemDelegate{parent},
    m_userIconSize{gIconSideSize, gIconSideSize}
{
    m_userIcon.addFile(QStringLiteral(":/user/user.png"), m_userIconSize);
}

QString TagItemDelegate::displayText(
    const QVariant & value, const QLocale & locale) const
{
    return AbstractStyledItemDelegate::displayText(value, locale);
}

QWidget * TagItemDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    const auto * tagModel = qobject_cast<const TagModel *>(index.model());
    if (!tagModel) {
        return nullptr;
    }

    const auto * modelItem = tagModel->itemForIndex(index);
    if (!modelItem) {
        return nullptr;
    }

    if (modelItem->type() == ITagModelItem::Type::LinkedNotebook) {
        return nullptr;
    }

    if (modelItem->type() == ITagModelItem::Type::AllTagsRoot) {
        return nullptr;
    }

    if (index.column() != static_cast<int>(TagModel::Column::Name)) {
        return nullptr;
    }

    return AbstractStyledItemDelegate::createEditor(parent, option, index);
}

void TagItemDelegate::paint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    painter->save();

    painter->setRenderHints(
        QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (index.isValid() &&
        (index.column() == static_cast<int>(TagModel::Column::Name)))
    {
        drawTagName(painter, index, option);
    }

    painter->restore();
}

void TagItemDelegate::setEditorData(
    QWidget * editor, const QModelIndex & index) const
{
    if (index.isValid() &&
        (index.column() == static_cast<int>(TagModel::Column::Name)))
    {
        AbstractStyledItemDelegate::setEditorData(editor, index);
    }
}

void TagItemDelegate::setModelData(
    QWidget * editor, QAbstractItemModel * model,
    const QModelIndex & index) const
{
    if (index.isValid() &&
        (index.column() == static_cast<int>(TagModel::Column::Name)))
    {
        AbstractStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize TagItemDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return {};
    }

    if (index.column() == static_cast<int>(TagModel::Column::Name)) {
        return tagNameSizeHint(option, index);
    }

    return AbstractStyledItemDelegate::sizeHint(option, index);
}

void TagItemDelegate::updateEditorGeometry(
    QWidget * editor, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (index.isValid() &&
        (index.column() == static_cast<int>(TagModel::Column::Name)))
    {
        AbstractStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}

void TagItemDelegate::drawTagName(
    QPainter * painter, const QModelIndex & index,
    const QStyleOptionViewItem & option) const
{
    const auto * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNDEBUG(
            "delegate::TagItemDelegate",
            "TagItemDelegate::drawTagName: can't draw, no model");
        return;
    }

    const auto * tagModel = qobject_cast<const TagModel *>(model);
    if (Q_UNLIKELY(!tagModel)) {
        QNDEBUG(
            "delegate::TagItemDelegate",
            "TagItemDelegate::drawTagName: non-tag model is set");
        return;
    }

    const auto * modelItem = tagModel->itemForIndex(index);
    if (Q_UNLIKELY(!modelItem)) {
        QNDEBUG(
            "delegate::TagItemDelegate",
            "TagItemDelegate::drawTagName: no model item for given index");
        return;
    }

    QStyleOptionViewItem adjustedOption{option};
    QString name;

    const auto * linkedNotebookItem =
        modelItem->cast<TagLinkedNotebookRootItem>();

    if (linkedNotebookItem) {
        QRect iconRect = adjustedOption.rect;
        iconRect.setRight(iconRect.left() + gIconSideSize);
        m_userIcon.paint(painter, iconRect);

        adjustedOption.rect.setLeft(
            adjustedOption.rect.left() + iconRect.width() + 2);

        name = linkedNotebookItem->username();
    }

    const auto * tagItem = modelItem->cast<TagItem>();
    if (tagItem) {
        name = tagItem->name();
    }

    if (name.isEmpty()) {
        name = index.data().toString().simplified();
    }
    else {
        name = name.simplified();
    }

    if (name.isEmpty()) {
        QNDEBUG(
            "delegate::TagItemDelegate",
            "TagItemDelegate::drawTagName: tag model item name is empty");
        return;
    }

    if (adjustedOption.state & QStyle::State_Selected) {
        painter->fillRect(
            adjustedOption.rect, adjustedOption.palette.highlight());
    }

    QString nameSuffix;

    if (tagItem) {
        const auto noteCountIndex = model->index(
            index.row(), static_cast<int>(TagModel::Column::NoteCount),
            index.parent());

        const QVariant noteCount = model->data(noteCountIndex);
        bool conversionResult = false;
        const int noteCountInt = noteCount.toInt(&conversionResult);
        if (conversionResult && (noteCountInt > 0)) {
            nameSuffix = QStringLiteral(" (");
            nameSuffix += QString::number(noteCountInt);
            nameSuffix += QStringLiteral(")");
        }

        adjustDisplayedText(name, adjustedOption, nameSuffix);
    }

    painter->setPen(
        adjustedOption.state & QStyle::State_Selected
            ? adjustedOption.palette.highlightedText().color()
            : adjustedOption.palette.windowText().color());

    painter->drawText(
        adjustedOption.rect, name,
        QTextOption{Qt::Alignment{Qt::AlignLeft | Qt::AlignVCenter}});

    if (nameSuffix.isEmpty()) {
        return;
    }

    const QFontMetrics fontMetrics{adjustedOption.font};
    const int nameWidth = fontMetrics.horizontalAdvance(name);

    painter->setPen(
        adjustedOption.state & QStyle::State_Selected
            ? adjustedOption.palette.color(
                  QPalette::Active, QPalette::WindowText)
            : adjustedOption.palette.color(
                  QPalette::Active, QPalette::Highlight));

    painter->drawText(
        adjustedOption.rect.translated(nameWidth, 0), nameSuffix,
        QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
}

QSize TagItemDelegate::tagNameSizeHint(
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

    const auto noteCountIndex = model->index(
        index.row(), static_cast<int>(TagModel::Column::NoteCount),
        index.parent());

    const QVariant noteCount = model->data(noteCountIndex);
    bool conversionResult = false;
    const int noteCountInt = noteCount.toInt(&conversionResult);
    if (conversionResult && (noteCountInt > 0)) {
        nameSuffix = QStringLiteral(" (");
        nameSuffix += QString::number(noteCountInt);
        nameSuffix += QStringLiteral(")");
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

} // namespace quentier
