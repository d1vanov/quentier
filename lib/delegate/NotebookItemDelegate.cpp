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

#include "NotebookItemDelegate.h"

#include <lib/model/NotebookModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QPainter>
#include <QFontMetrics>
#include <QTextOption>

#include <algorithm>
#include <cmath>

#define CIRCLE_RADIUS (2)
#define ICON_SIDE_SIZE (16)

namespace quentier {

NotebookItemDelegate::NotebookItemDelegate(QObject * parent) :
    AbstractStyledItemDelegate(parent),
    m_userIcon(),
    m_userIconSize(ICON_SIDE_SIZE, ICON_SIDE_SIZE)
{
    m_userIcon.addFile(QStringLiteral(":/user/user.png"), m_userIconSize);
}

QString NotebookItemDelegate::displayText(const QVariant & value,
                                          const QLocale & locale) const
{
    return AbstractStyledItemDelegate::displayText(value, locale);
}

QWidget * NotebookItemDelegate::createEditor(QWidget * parent,
                                             const QStyleOptionViewItem & option,
                                             const QModelIndex & index) const
{
    const NotebookModel * pNotebookModel =
        qobject_cast<const NotebookModel*>(index.model());
    if (!pNotebookModel) {
        return Q_NULLPTR;
    }

    const NotebookModelItem * pModelItem = pNotebookModel->itemForIndex(index);
    if (!pModelItem) {
        return Q_NULLPTR;
    }

    if (pModelItem->type() == NotebookModelItem::Type::LinkedNotebook) {
        return Q_NULLPTR;
    }

    if (index.column() != NotebookModel::Columns::Name) {
        return Q_NULLPTR;
    }

    return AbstractStyledItemDelegate::createEditor(parent, option, index);
}

void NotebookItemDelegate::paint(QPainter * painter,
                                 const QStyleOptionViewItem & option,
                                 const QModelIndex & index) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    if (index.isValid())
    {
        int column = index.column();
        if (column == NotebookModel::Columns::Name)
        {
            drawNotebookName(painter, index, option);
        }
        else if (column == NotebookModel::Columns::Default)
        {
            bool isDefault = index.model()->data(index).toBool();
            if (isDefault) {
                painter->setBrush(QBrush(Qt::green));
                drawEllipse(painter, option, index);
            }
        }
        else if (column == NotebookModel::Columns::Published)
        {
            bool published = index.model()->data(index).toBool();
            if (published) {
                painter->setBrush(QBrush(Qt::blue));
                drawEllipse(painter, option, index);
            }
        }
    }

    painter->restore();
}

void NotebookItemDelegate::setEditorData(QWidget * editor,
                                         const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        AbstractStyledItemDelegate::setEditorData(editor, index);
    }
}

void NotebookItemDelegate::setModelData(QWidget * editor,
                                        QAbstractItemModel * model,
                                        const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        AbstractStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize NotebookItemDelegate::sizeHint(const QStyleOptionViewItem & option,
                                     const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return QSize();
    }

    int colNameWidth = columnNameWidth(option, index);

    int column = index.column();
    if ((column == NotebookModel::Columns::Default) ||
        (column == NotebookModel::Columns::Published))
    {
        int side = CIRCLE_RADIUS;
        side += 1;
        side *= 2;
        int width = std::max(colNameWidth, side);
        return QSize(width, side);
    }
    else if (column == NotebookModel::Columns::Name)
    {
        return notebookNameSizeHint(option, index, colNameWidth);
    }

    return AbstractStyledItemDelegate::sizeHint(option, index);
}

void NotebookItemDelegate::updateEditorGeometry(QWidget * editor,
                                                const QStyleOptionViewItem & option,
                                                const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        AbstractStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}

void NotebookItemDelegate::drawEllipse(QPainter * painter,
                                       const QStyleOptionViewItem & option,
                                       const QModelIndex & index) const
{
    int colNameWidth = columnNameWidth(option, index);
    int side = std::min(option.rect.width(), option.rect.height());
    int radius = std::min(side, CIRCLE_RADIUS);
    int diameter = 2 * radius;
    QPoint center = option.rect.center();
    center.setX(std::min(center.x(),
                         (option.rect.left() +
                          std::max(colNameWidth, side)/2 + 1)));
    painter->setPen(QColor());
    painter->drawEllipse(QRectF(center.x() - radius,
                                center.y() - radius,
                                diameter, diameter));
}

void NotebookItemDelegate::drawNotebookName(QPainter * painter,
                                            const QModelIndex & index,
                                            const QStyleOptionViewItem & option) const
{
    const NotebookModel * pNotebookModel =
        qobject_cast<const NotebookModel*>(index.model());
    if (Q_UNLIKELY(!pNotebookModel)) {
        QNDEBUG(QStringLiteral("NotebookItemDelegate::drawNotebookName: "
                               "can't draw, no notebook model"));
        return;
    }

    const NotebookModelItem * pModelItem = pNotebookModel->itemForIndex(index);
    if (Q_UNLIKELY(!pModelItem)) {
        QNDEBUG(QStringLiteral("NotebookItemDelegate::drawNotebookName: can't "
                               "draw, no notebook model item corresponding to "
                               "index"));
        return;
    }

    QStyleOptionViewItem adjustedOption(option);
    if ((pModelItem->type() == NotebookModelItem::Type::LinkedNotebook) &&
        pModelItem->notebookLinkedNotebookItem())
    {
        QRect iconRect = adjustedOption.rect;
        iconRect.setRight(iconRect.left() + ICON_SIDE_SIZE);
        m_userIcon.paint(painter, iconRect);
        adjustedOption.rect.setLeft(adjustedOption.rect.left() +
                                    iconRect.width() + 2);
    }

    QString name;
    if ((pModelItem->type() == NotebookModelItem::Type::Notebook) &&
        pModelItem->notebookItem())
    {
        name = pModelItem->notebookItem()->name();
    }
    else if ((pModelItem->type() == NotebookModelItem::Type::Stack) &&
             pModelItem->notebookStackItem())
    {
        name = pModelItem->notebookStackItem()->name();
    }
    else if ((pModelItem->type() == NotebookModelItem::Type::LinkedNotebook) &&
             pModelItem->notebookLinkedNotebookItem())
    {
        name = pModelItem->notebookLinkedNotebookItem()->username();
    }

    if (name.isEmpty()) {
        QNDEBUG(QStringLiteral("NotebookItemDelegate::drawNotebookName: "
                               "notebook name is empty"));
        return;
    }

    QString nameSuffix;
    if ((pModelItem->type() == NotebookModelItem::Type::Notebook) &&
        pModelItem->notebookItem())
    {
        int numNotesPerNotebook = pModelItem->notebookItem()->numNotesPerNotebook();
        if (numNotesPerNotebook > 0) {
            nameSuffix = QStringLiteral(" (");
            nameSuffix += QString::number(numNotesPerNotebook);
            nameSuffix += QStringLiteral(")");
            adjustDisplayedText(name, adjustedOption, nameSuffix);
        }
    }

    painter->setPen(adjustedOption.state & QStyle::State_Selected
                    ? adjustedOption.palette.highlightedText().color()
                    : adjustedOption.palette.windowText().color());
    painter->drawText(QRectF(adjustedOption.rect), name,
                      QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));

    if (nameSuffix.isEmpty()) {
        return;
    }

    QFontMetrics fontMetrics(adjustedOption.font);
    int nameWidth = fontMetrics.width(name);

    painter->setPen(adjustedOption.state & QStyle::State_Selected
                    ? adjustedOption.palette.color(QPalette::Active,
                                                   QPalette::WindowText)
                    : adjustedOption.palette.color(QPalette::Active,
                                                   QPalette::Highlight));
    painter->drawText(QRectF(adjustedOption.rect.translated(nameWidth, 0)),
                      nameSuffix,
                      QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
}

QSize NotebookItemDelegate::notebookNameSizeHint(const QStyleOptionViewItem & option,
                                                 const QModelIndex & index,
                                                 const int columnNameWidth) const
{
    QNTRACE(QStringLiteral("NotebookItemDelegate::notebookNameSizeHint: ")
            << QStringLiteral("column name width = ") << columnNameWidth);

    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNDEBUG(QStringLiteral("No model, fallback to the default size hint"));
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    QString name = model->data(index).toString().simplified();
    if (Q_UNLIKELY(name.isEmpty())) {
        QNDEBUG(QStringLiteral("Notebook name is empty, fallback to the default "
                               "size hint"));
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    QModelIndex numNotesPerNotebookIndex =
            model->index(index.row(), NotebookModel::Columns::NumNotesPerNotebook,
                         index.parent());
    QVariant numNotesPerNotebook = model->data(numNotesPerNotebookIndex);
    bool conversionResult = false;
    int numNotesPerNotebookInt = numNotesPerNotebook.toInt(&conversionResult);

    QString nameSuffix;
    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the number of notes per ")
                << QStringLiteral("notebook to int: ") << numNotesPerNotebook);
    }
    else if (numNotesPerNotebookInt > 0) {
        QNTRACE(QStringLiteral("Appending num notes per notebook to the notebook ")
                << QStringLiteral("name: ") << numNotesPerNotebookInt);
        nameSuffix = QStringLiteral(" (");
        nameSuffix += QString::number(numNotesPerNotebookInt);
        nameSuffix += QStringLiteral(")");
    }

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name + nameSuffix);
    int fontHeight = fontMetrics.height();

    double margin = 0.1;
    int width =
        std::max(static_cast<int>(std::floor(nameWidth * (1.0 + margin) + 0.5)),
                 option.rect.width());
    if (width < columnNameWidth) {
        width = columnNameWidth;
    }

    int height =
        std::max(static_cast<int>(std::floor(fontHeight * (1.0 + margin) + 0.5)),
                 option.rect.height());

    QSize size = QSize(width, height);

    QNTRACE(QStringLiteral("Computed size: width = ") << width
            << QStringLiteral(", height = ") << height);
    return size;
}

} // namespace quentier
