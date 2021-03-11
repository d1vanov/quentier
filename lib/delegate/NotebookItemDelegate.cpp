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

#include "NotebookItemDelegate.h"

#include <lib/model/notebook/NotebookModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QFontMetrics>
#include <QPainter>
#include <QTextOption>

#include <algorithm>
#include <cmath>

namespace quentier {

namespace {

constexpr int gCircleRadius = 2;
constexpr int gIconSideSize = 16;

} // namespace

NotebookItemDelegate::NotebookItemDelegate(QObject * parent) :
    AbstractStyledItemDelegate(parent),
    m_userIconSize(gIconSideSize, gIconSideSize)
{
    m_userIcon.addFile(QStringLiteral(":/user/user.png"), m_userIconSize);
}

QString NotebookItemDelegate::displayText(
    const QVariant & value, const QLocale & locale) const
{
    return AbstractStyledItemDelegate::displayText(value, locale);
}

QWidget * NotebookItemDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    const auto * pModelItem = notebookModelItem(index);
    if (!pModelItem) {
        return nullptr;
    }

    const auto type = pModelItem->type();
    if ((type != INotebookModelItem::Type::Notebook) &&
        (type != INotebookModelItem::Type::Stack))
    {
        return nullptr;
    }

    if (index.column() != static_cast<int>(NotebookModel::Column::Name)) {
        return nullptr;
    }

    return AbstractStyledItemDelegate::createEditor(parent, option, index);
}

void NotebookItemDelegate::paint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    painter->save();

    painter->setRenderHints(
        QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    const auto * pModelItem = notebookModelItem(index);
    if (pModelItem) {
        paintItem(painter, option, index, *pModelItem);
    }

    painter->restore();
}

void NotebookItemDelegate::setEditorData(
    QWidget * editor, const QModelIndex & index) const
{
    if (index.isValid() &&
        (index.column() == static_cast<int>(NotebookModel::Column::Name)))
    {
        AbstractStyledItemDelegate::setEditorData(editor, index);
    }
}

void NotebookItemDelegate::setModelData(
    QWidget * editor, QAbstractItemModel * model,
    const QModelIndex & index) const
{
    if (index.isValid() &&
        (index.column() == static_cast<int>(NotebookModel::Column::Name)))
    {
        AbstractStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize NotebookItemDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return {};
    }

    const int colNameWidth = columnNameWidth(option, index);

    const int column = index.column();
    if ((column == static_cast<int>(NotebookModel::Column::Default)) ||
        (column == static_cast<int>(NotebookModel::Column::Published)))
    {
        int side = gCircleRadius;
        side += 1;
        side *= 2;

        const int width = std::max(colNameWidth, side);
        return QSize{width, side};
    }

    if (column == static_cast<int>(NotebookModel::Column::Name)) {
        return notebookNameSizeHint(option, index, colNameWidth);
    }

    return AbstractStyledItemDelegate::sizeHint(option, index);
}

void NotebookItemDelegate::updateEditorGeometry(
    QWidget * editor, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (index.isValid() &&
        (index.column() == static_cast<int>(NotebookModel::Column::Name)))
    {
        AbstractStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}

const INotebookModelItem * NotebookItemDelegate::notebookModelItem(
    const QModelIndex & index) const
{
    const auto * pNotebookModel =
        qobject_cast<const NotebookModel *>(index.model());

    if (Q_UNLIKELY(!pNotebookModel)) {
        return nullptr;
    }

    return pNotebookModel->itemForIndex(index);
}

void NotebookItemDelegate::paintItem(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index, const INotebookModelItem & item) const
{
    if (item.type() == INotebookModelItem::Type::InvisibleRoot) {
        return;
    }

    const int column = index.column();
    if (column == static_cast<int>(NotebookModel::Column::Name)) {
        drawNotebookName(painter, index, option);
        return;
    }

    if (column == static_cast<int>(NotebookModel::Column::Default)) {
        bool isDefault = index.model()->data(index).toBool();
        if (isDefault) {
            painter->setBrush(QBrush(Qt::green));
            drawEllipse(painter, option, index);
        }
    }
    else if (column == static_cast<int>(NotebookModel::Column::Published)) {
        bool published = index.model()->data(index).toBool();
        if (published) {
            painter->setBrush(QBrush(Qt::blue));
            drawEllipse(painter, option, index);
        }
    }
}

void NotebookItemDelegate::drawEllipse(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    const int colNameWidth = columnNameWidth(option, index);
    const int side = std::min(option.rect.width(), option.rect.height());
    const int radius = std::min(side, gCircleRadius);
    const int diameter = 2 * radius;

    auto center = option.rect.center();

    center.setX(std::min(
        center.x(),
        (option.rect.left() + std::max(colNameWidth, side) / 2 + 1)));

    painter->setPen(QColor());

    painter->drawEllipse(
        QRectF(center.x() - radius, center.y() - radius, diameter, diameter));
}

void NotebookItemDelegate::drawNotebookName(
    QPainter * painter, const QModelIndex & index,
    const QStyleOptionViewItem & option) const
{
    const auto * pModelItem = notebookModelItem(index);
    if (Q_UNLIKELY(!pModelItem)) {
        return;
    }

    QStyleOptionViewItem adjustedOption{option};

    const auto * pLinkedNotebookItem =
        pModelItem->cast<LinkedNotebookRootItem>();

    if (pLinkedNotebookItem) {
        auto iconRect = adjustedOption.rect;
        iconRect.setRight(iconRect.left() + gIconSideSize);
        m_userIcon.paint(painter, iconRect);

        adjustedOption.rect.setLeft(
            adjustedOption.rect.left() + iconRect.width() + 2);
    }

    QString name;

    const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
    if (pNotebookItem) {
        name = pNotebookItem->name();
    }

    if (name.isEmpty()) {
        const auto * pStackItem = pModelItem->cast<StackItem>();
        if (pStackItem) {
            name = pStackItem->name();
        }
    }

    if (name.isEmpty()) {
        const auto * pLinkedNotebookItem =
            pModelItem->cast<LinkedNotebookRootItem>();

        if (pLinkedNotebookItem) {
            name = pLinkedNotebookItem->username();
        }
    }

    if (name.isEmpty()) {
        name = index.data().toString().simplified();
    }

    if (name.isEmpty()) {
        QNDEBUG(
            "delegate",
            "NotebookItemDelegate::drawNotebookName: "
                << "notebook name is empty");
        return;
    }

    QString nameSuffix;
    if (pNotebookItem) {
        const int noteCount = pNotebookItem->noteCount();
        if (noteCount > 0) {
            nameSuffix = QStringLiteral(" (");
            nameSuffix += QString::number(noteCount);
            nameSuffix += QStringLiteral(")");
            adjustDisplayedText(name, adjustedOption, nameSuffix);
        }
    }

    painter->setPen(
        adjustedOption.state & QStyle::State_Selected
            ? adjustedOption.palette.highlightedText().color()
            : adjustedOption.palette.windowText().color());

    painter->drawText(
        QRectF(adjustedOption.rect), name,
        QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));

    if (nameSuffix.isEmpty()) {
        return;
    }

    const QFontMetrics fontMetrics{adjustedOption.font};
    const int nameWidth = fontMetricsWidth(fontMetrics, name);

    painter->setPen(
        adjustedOption.state & QStyle::State_Selected
            ? adjustedOption.palette.color(
                  QPalette::Active, QPalette::WindowText)
            : adjustedOption.palette.color(
                  QPalette::Active, QPalette::Highlight));

    painter->drawText(
        QRectF(adjustedOption.rect.translated(nameWidth, 0)), nameSuffix,
        QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
}

QSize NotebookItemDelegate::notebookNameSizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index,
    const int columnNameWidth) const
{
    QNTRACE(
        "delegate",
        "NotebookItemDelegate::notebookNameSizeHint: "
            << "column name width = " << columnNameWidth);

    const auto * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNDEBUG("delegate", "No model, fallback to the default size hint");
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    const auto name = model->data(index).toString().simplified();
    if (Q_UNLIKELY(name.isEmpty())) {
        QNDEBUG(
            "delegate",
            "Notebook name is empty, fallback to the default "
                << "size hint");
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    const auto noteCountIndex = model->index(
        index.row(), static_cast<int>(NotebookModel::Column::NoteCount),
        index.parent());

    const auto noteCount = model->data(noteCountIndex);
    bool conversionResult = false;
    const int noteCountInt = noteCount.toInt(&conversionResult);

    QString nameSuffix;
    if (!conversionResult) {
        QNTRACE(
            "delegate",
            "Failed to convert the number of notes per "
                << "notebook to int: " << noteCount);
    }
    else if (noteCountInt > 0) {
        QNTRACE(
            "delegate",
            "Appending num notes per notebook to the notebook "
                << "name: " << noteCountInt);

        nameSuffix = QStringLiteral(" (");
        nameSuffix += QString::number(noteCountInt);
        nameSuffix += QStringLiteral(")");
    }

    const QFontMetrics fontMetrics{option.font};
    const int nameWidth = fontMetricsWidth(fontMetrics, name + nameSuffix);
    const int fontHeight = fontMetrics.height();

    constexpr double margin = 0.1;

    int width = std::max(
        static_cast<int>(std::floor(nameWidth * (1.0 + margin) + 0.5)),
        option.rect.width());

    if (width < columnNameWidth) {
        width = columnNameWidth;
    }

    const int height = std::max(
        static_cast<int>(std::floor(fontHeight * (1.0 + margin) + 0.5)),
        option.rect.height());

    QNTRACE(
        "delegate",
        "Computed size: width = " << width << ", height = " << height);

    return QSize{width, height};
}

} // namespace quentier
