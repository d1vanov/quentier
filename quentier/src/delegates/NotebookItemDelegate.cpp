/*
 * Copyright 2016 Dmitry Ivanov
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
#include "../models/NotebookModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>
#include <QFontMetrics>
#include <QTextOption>
#include <algorithm>
#include <cmath>

#define CIRCLE_RADIUS (2)

namespace quentier {

NotebookItemDelegate::NotebookItemDelegate(QObject * parent) :
    AbstractStyledItemDelegate(parent)
{}

QString NotebookItemDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    return QStyledItemDelegate::displayText(value, locale);
}

QWidget * NotebookItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                             const QModelIndex & index) const
{
    // Only allow to edit the notebook name but no other notebook model columns
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
    else {
        return Q_NULLPTR;
    }
}

void NotebookItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
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

void NotebookItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void NotebookItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                        const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize NotebookItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
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

    return QStyledItemDelegate::sizeHint(option, index);
}

void NotebookItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                                const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}

void NotebookItemDelegate::drawEllipse(QPainter * painter, const QStyleOptionViewItem & option,
                                       const QModelIndex & index) const
{
    int colNameWidth = columnNameWidth(option, index);
    int side = std::min(option.rect.width(), option.rect.height());
    int radius = std::min(side, CIRCLE_RADIUS);
    int diameter = 2 * radius;
    QPoint center = option.rect.center();
    center.setX(std::min(center.x(), (option.rect.left() + std::max(colNameWidth, side)/2 + 1)));
    painter->setPen(QColor());
    painter->drawEllipse(QRectF(center.x() - radius, center.y() - radius, diameter, diameter));
}

void NotebookItemDelegate::drawNotebookName(QPainter * painter, const QModelIndex & index,
                                            const QStyleOptionViewItem & option) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNDEBUG(QStringLiteral("NotebookItemDelegate::drawNotebookName: can't draw, no model"));
        return;
    }

    QString name = model->data(index).toString();
    if (name.isEmpty()) {
        QNDEBUG(QStringLiteral("NotebookItemDelegate::drawNotebookName: notebook name is empty"));
        return;
    }

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.highlightedText().color()
                    : option.palette.windowText().color());
    painter->drawText(QRectF(option.rect), name, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name);

    QModelIndex numNotesPerNotebookIndex = model->index(index.row(), NotebookModel::Columns::NumNotesPerNotebook, index.parent());
    QVariant numNotesPerNotebook = model->data(numNotesPerNotebookIndex);
    bool conversionResult = false;
    int numNotesPerNotebookInt = numNotesPerNotebook.toInt(&conversionResult);
    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the number of notes per notebook to int: ") << numNotesPerNotebook);
        return;
    }

    if (numNotesPerNotebookInt <= 0) {
        return;
    }

    QString nameSuffix = QStringLiteral(" (");
    nameSuffix += QString::number(numNotesPerNotebookInt);
    nameSuffix += QStringLiteral(")");

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.color(QPalette::Active, QPalette::WindowText)
                    : option.palette.color(QPalette::Active, QPalette::Highlight));
    painter->drawText(QRectF(option.rect.translated(nameWidth, 0)), nameSuffix,
                      QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
}

QSize NotebookItemDelegate::notebookNameSizeHint(const QStyleOptionViewItem & option,
                                                 const QModelIndex & index,
                                                 const int columnNameWidth) const
{
    QNTRACE(QStringLiteral("NotebookItemDelegate::notebookNameSizeHint: column name width = ")
            << columnNameWidth);

    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNDEBUG(QStringLiteral("No model, fallback to the default size hint"));
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QString name = model->data(index).toString();
    if (Q_UNLIKELY(name.isEmpty())) {
        QNDEBUG(QStringLiteral("Notebook name is empty, fallback to the default size hint"));
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QModelIndex numNotesPerNotebookIndex =
            model->index(index.row(), NotebookModel::Columns::NumNotesPerNotebook, index.parent());
    QVariant numNotesPerNotebook = model->data(numNotesPerNotebookIndex);
    bool conversionResult = false;
    int numNotesPerNotebookInt = numNotesPerNotebook.toInt(&conversionResult);

    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the number of notes per notebook to int: ")
                << numNotesPerNotebook);
    }
    else if (numNotesPerNotebookInt > 0) {
        QNTRACE(QStringLiteral("Appending num notes per notebook to the notebook name: ")
                << numNotesPerNotebookInt);
        name += QStringLiteral(" (");
        name += QString::number(numNotesPerNotebookInt);
        name += QStringLiteral(")");
    }

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name);
    int fontHeight = fontMetrics.height();

    double margin = 0.1;
    int width = std::max(static_cast<int>(std::floor(nameWidth * (1.0 + margin) + 0.5)),
                         option.rect.width());
    if (width < columnNameWidth) {
        width = columnNameWidth;
    }

    int height = std::max(static_cast<int>(std::floor(fontHeight * (1.0 + margin) + 0.5)),
                          option.rect.height());

    QSize size = QSize(width, height);

    QNTRACE(QStringLiteral("Computed size: width = ") << width
            << QStringLiteral(", height = ") << height);
    return size;
}

} // namespace quentier
