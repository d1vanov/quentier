/*
 * Copyright 2017-2021 Dmitry Ivanov
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

#include "DeletedNoteItemDelegate.h"

#include <lib/model/note/NoteModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QDateTime>
#include <QPainter>
#include <QTextOption>

#include <cmath>

namespace quentier {

namespace {

constexpr int gTextHeightMargin = 4;
constexpr int gTextWidthMargin = 8;
constexpr int gFirstColumnPadding = 10;

} // namespace

DeletedNoteItemDelegate::DeletedNoteItemDelegate(QObject * parent) :
    AbstractStyledItemDelegate(parent)
{
    m_deletionDateTimeReplacementText =
        QStringLiteral("(") + tr("No deletion datetime") + QStringLiteral(")");
}

QWidget * DeletedNoteItemDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return nullptr;
}

void DeletedNoteItemDelegate::paint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    painter->save();

    painter->setRenderHints(
        QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    doPaint(painter, option, index);
    painter->restore();
}

void DeletedNoteItemDelegate::setEditorData(
    QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void DeletedNoteItemDelegate::setModelData(
    QWidget * editor, QAbstractItemModel * model,
    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize DeletedNoteItemDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const QSize size = doSizeHint(option, index);
    if (size.isValid()) {
        return size;
    }

    // Fallback to the base class' implementation
    return AbstractStyledItemDelegate::sizeHint(option, index);
}

void DeletedNoteItemDelegate::updateEditorGeometry(
    QWidget * editor, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

void DeletedNoteItemDelegate::doPaint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (!index.isValid()) {
        return;
    }

    const auto * pModel = index.model();
    if (Q_UNLIKELY(!pModel)) {
        QNDEBUG(
            "delegate",
            "DeletedNoteItemDelegate::doPaint: can't paint, "
                << "no model");
        return;
    }

    const auto * pNoteModel = qobject_cast<const NoteModel *>(pModel);
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(
            "delegate",
            "DeletedNoteItemDelegate::doPaint: can't paint, "
                << "can't cast the model to NoteModel");
        return;
    }

    const auto * pNoteItem = pNoteModel->itemForIndex(index);
    if (Q_UNLIKELY(!pNoteItem)) {
        QNDEBUG(
            "delegate",
            "DeletedNoteItemDelegate::doPaint: can't paint, "
                << "no note item for index: row = " << index.row()
                << ", column = " << index.column());
        return;
    }

    const int column = index.column();

    if (column == static_cast<int>(NoteModel::Column::Title)) {
        drawDeletedNoteTitleOrPreviewText(painter, option, *pNoteItem);
    }
    else if (column == static_cast<int>(NoteModel::Column::DeletionTimestamp)) {
        drawDeletionDateTime(painter, option, *pNoteItem);
    }
}

void DeletedNoteItemDelegate::drawDeletedNoteTitleOrPreviewText(
    QPainter * painter, const QStyleOptionViewItem & option,
    const NoteModelItem & item) const
{
    QString text = item.title();
    if (text.isEmpty()) {
        text = item.previewText();
    }

    text = text.simplified();

    if (text.isEmpty()) {
        painter->setPen(
            (option.state & QStyle::State_Selected)
                ? option.palette.color(QPalette::Active, QPalette::WindowText)
                : option.palette.color(QPalette::Active, QPalette::Highlight));

        text = QStringLiteral("(") + tr("Note without title or content") +
            QStringLiteral(")");
    }
    else {
        painter->setPen(
            option.state & QStyle::State_Selected
                ? option.palette.highlightedText().color()
                : option.palette.windowText().color());
    }

    adjustDisplayedText(text, option);

    painter->drawText(
        option.rect, text,
        QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
}

void DeletedNoteItemDelegate::drawDeletionDateTime(
    QPainter * painter, const QStyleOptionViewItem & option,
    const NoteModelItem & item) const
{
    const qint64 deletionTimestamp = item.deletionTimestamp();

    QString text;
    if (deletionTimestamp == static_cast<qint64>(0)) {
        painter->setPen(
            (option.state & QStyle::State_Selected)
                ? option.palette.color(QPalette::Active, QPalette::WindowText)
                : option.palette.color(QPalette::Active, QPalette::Highlight));

        text = m_deletionDateTimeReplacementText;
    }
    else {
        painter->setPen(
            option.state & QStyle::State_Selected
                ? option.palette.highlightedText().color()
                : option.palette.windowText().color());

        text = QDateTime::fromMSecsSinceEpoch(deletionTimestamp)
                   .toString(Qt::DefaultLocaleShortDate);

        text.prepend(QStringLiteral(" "));

        adjustDisplayedText(text, option);
    }

    QRect rect = option.rect;
    rect.translate(gFirstColumnPadding, 0);

    painter->drawText(
        rect, text,
        QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
}

QSize DeletedNoteItemDelegate::doSizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (!index.isValid()) {
        return {};
    }

    const auto * pModel = index.model();
    if (Q_UNLIKELY(!pModel)) {
        QNDEBUG(
            "delegate",
            "DeletedNoteItemDelegate::doSizeHint: "
                << "can't compute size hint, no model");
        return {};
    }

    const auto * pNoteModel = qobject_cast<const NoteModel *>(pModel);
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(
            "delegate",
            "DeletedNoteItemDelegate::doSizeHint: can't "
                << "compute size hint, can't cast the model to NoteModel");
        return {};
    }

    const auto * pNoteItem = pNoteModel->itemForIndex(index);
    if (Q_UNLIKELY(!pNoteItem)) {
        QNDEBUG(
            "delegate",
            "DeletedNoteItemDelegate::doSizeHint: can't "
                << "compute size hint, no note item for index: row = "
                << index.row() << ", column = " << index.column());
        return {};
    }

    const int column = index.column();

    const QFontMetrics fontMetrics{option.font};
    const int height = fontMetrics.height() + gTextHeightMargin;

    if (column == static_cast<int>(NoteModel::Column::Title)) {
        QString text = pNoteItem->title();
        if (text.isEmpty()) {
            text = pNoteItem->previewText();
        }

        text = text.simplified();

        const int width =
            fontMetricsWidth(fontMetrics, text) + gTextWidthMargin;

        return QSize{width, height};
    }

    if (column == static_cast<int>(NoteModel::Column::DeletionTimestamp)) {
        QString text;

        const qint64 deletionTimestamp = pNoteItem->deletionTimestamp();
        if (deletionTimestamp == static_cast<qint64>(0)) {
            text = m_deletionDateTimeReplacementText;
        }
        else {
            text = QDateTime::fromMSecsSinceEpoch(deletionTimestamp)
                       .toString(Qt::DefaultLocaleShortDate);
        }

        text.prepend(QStringLiteral(" "));

        const int width =
            gFirstColumnPadding + fontMetricsWidth(fontMetrics, text) +
            gTextWidthMargin;

        return QSize{width, height};
    }

    return {};
}

} // namespace quentier
