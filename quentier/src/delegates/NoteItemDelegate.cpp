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

#include "NoteItemDelegate.h"
#include "../models/NoteModel.h"
#include "../models/NoteFilterModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>
#include <QDateTime>
#include <QTextOption>
#include <cmath>

#define MSEC_PER_DAY (864e5)
#define MSEC_PER_WEEK (6048e5)

namespace quentier {

NoteItemDelegate::NoteItemDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_defaultSize(350, 120),
    m_leftMargin(2),
    m_rightMargin(2),
    m_topMargin(2),
    m_bottomMargin(2)
{}

QWidget * NoteItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                         const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void NoteItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                             const QModelIndex & index) const
{
    const NoteFilterModel * pNoteFilterModel = qobject_cast<const NoteFilterModel*>(index.model());
    if (Q_UNLIKELY(!pNoteFilterModel)) {
        QNLocalizedString error = QT_TR_NOOP("wrong model connected to the note item delegate");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNLocalizedString error = QT_TR_NOOP("can't get the source model from the note filter model connected to the note item delegate");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QModelIndex sourceIndex = pNoteFilterModel->mapToSource(index);
    const NoteModelItem * pItem = pNoteModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pItem)) {
        QNLocalizedString error = QT_TR_NOOP("can't retrieve item to paint for note model index");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    painter->fillRect(option.rect, option.palette.window());
    painter->setPen(option.palette.windowText().color());

    bool isLast = (index.row() == pNoteFilterModel->rowCount(QModelIndex()) - 1);

    if (option.state & QStyle::State_Selected)
    {
        QPen originalPen = painter->pen();
        QPen pen = originalPen;
        pen.setWidth(1);
        pen.setColor(option.palette.highlight().color());
        painter->setPen(pen);

        int dx1 = 1;
        int dy1 = 1;
        int dx2 = -1;
        int dy2 = (isLast ? -1 : -2);

        painter->drawRoundedRect(QRectF(option.rect.adjusted(dx1, dy1, dx2, dy2)), 2, 2);
        painter->setPen(originalPen);
    }

    if (!isLast)
    {
        QPen originalPen = painter->pen();
        QPen pen = originalPen;
        pen.setWidth(1);
        pen.setColor(option.palette.windowText().color());
        painter->setPen(pen);

        QLine bottomBoundaryLine(option.rect.bottomLeft(), option.rect.bottomRight());
        painter->drawLine(bottomBoundaryLine);

        painter->setPen(originalPen);
    }

    QFont boldFont = painter->font();
    boldFont.setBold(true);
    QFontMetrics boldFontMetrics(boldFont);

    QFont smallerFont = painter->font();
    smallerFont.setPointSize(smallerFont.pointSize() - 1);
    QFontMetrics smallerFontMetrics(smallerFont);

    const QImage & thumbnail = pItem->thumbnail();

    int left = option.rect.left() + m_leftMargin;
    int width = option.rect.width() - m_leftMargin - m_rightMargin;
    if (!thumbnail.isNull()) {
        width -= 120;
    }

    // 1) Painting the title (or a piece of preview text if there's no title)
    QFont originalFont = painter->font();
    painter->setFont(boldFont);

    QRect titleRect(left, option.rect.top() + m_topMargin, width, boldFontMetrics.height());

    QString title = pItem->title();
    if (title.isEmpty()) {
        title = pItem->previewText();
    }

    title = title.simplified();

    if (title.isEmpty()) {
        title = tr("No title");
    }
    else {
        title = boldFontMetrics.elidedText(title, Qt::ElideRight, titleRect.width());
    }

    painter->drawText(QRectF(titleRect), title, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));

    // 2) Painting the created/modified/deleted datetime
    qint64 targetTimestamp = 0;
    bool deleted = false;

    qint64 deletionTimestamp = pItem->deletionTimestamp();
    if (deletionTimestamp != 0) {
        targetTimestamp = deletionTimestamp;
        deleted = true;
    }
    else {
        qint64 modificationTimestamp = pItem->modificationTimestamp();
        qint64 creationTimestamp = pItem->creationTimestamp();
        targetTimestamp = std::max(creationTimestamp, modificationTimestamp);
    }

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    qint64 timeSpan = currentTimestamp - targetTimestamp;

#define CHECK_AND_SET_DELETED_PREFIX(text) \
    if (deleted) { \
        text.prepend(tr(QT_TR_NOOP("Deleted")) + QStringLiteral(" ")); \
    }

    QString text;
    if (timeSpan > MSEC_PER_WEEK)
    {
        int pastWeeks = static_cast<int>(std::floor(timeSpan / MSEC_PER_WEEK + 0.5));

        if (pastWeeks == 1) {
            text = (deleted ? tr("Deleted last week") : tr("Last week"));
        }
        else if (pastWeeks < 5) {
            text = tr("%n weeks ago", Q_NULLPTR, pastWeeks);
            CHECK_AND_SET_DELETED_PREFIX(text)
        }
    }
    else if (timeSpan > MSEC_PER_DAY)
    {
        int pastDays = static_cast<int>(std::floor(timeSpan / MSEC_PER_DAY + 0.5));

        if (pastDays == 1) {
            text = (deleted ? tr("Deleted yesterday") : tr("Yesterday"));
        }
        else if (pastDays < 6) {
            text = tr("%n days ago", Q_NULLPTR, pastDays);
            if (deleted) {
                CHECK_AND_SET_DELETED_PREFIX(text)
            }
        }
    }

    if (text.isEmpty())
    {
        if (targetTimestamp == 0) {
            text = tr("No created/modified datetime");
        }
        else {
            text = QDateTime::fromMSecsSinceEpoch(targetTimestamp).toString(Qt::DefaultLocaleShortDate);
            CHECK_AND_SET_DELETED_PREFIX(text)
        }
    }

#undef CHECK_AND_SET_DELETED_PREFIX

    QRect dateTimeRect(left, titleRect.bottom() + m_bottomMargin + m_topMargin, width, smallerFontMetrics.height());

    if (targetTimestamp != 0) {
        text = smallerFontMetrics.elidedText(text, Qt::ElideRight, dateTimeRect.width());
    }

    painter->setFont(smallerFont);

    QPen originalPen = painter->pen();
    QPen pen = originalPen;
    pen.setColor(option.palette.highlight().color());
    painter->setPen(pen);
    painter->drawText(QRectF(dateTimeRect), text, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
    painter->setPen(originalPen);

    // 3) Painting the preview text
    int previewTextTop = dateTimeRect.bottom() + m_bottomMargin + m_topMargin;
    QRect previewTextRect(left, previewTextTop, width, option.rect.bottom() - previewTextTop);

    QNTRACE(QStringLiteral("Preview text rect: top = ") << previewTextRect.top() << QStringLiteral(", bottom = ")
            << previewTextRect.bottom() << QStringLiteral(", left = ") << previewTextRect.left()
            << QStringLiteral(", right = ") << previewTextRect.right() << QStringLiteral("; height = ")
            << previewTextRect.height() << QStringLiteral(", width = ") << previewTextRect.width());

    text = pItem->previewText().simplified();
    QFontMetrics originalFontMetrics(originalFont);

    QNTRACE(QStringLiteral("Preview text: ") << text);

    int linesForText = static_cast<int>(std::floor(originalFontMetrics.width(text) / previewTextRect.width() + 0.5));
    int linesAvailable = static_cast<int>(std::floor(previewTextRect.height() / smallerFontMetrics.lineSpacing()));

    QNTRACE(QStringLiteral("Lines for text = ") << linesForText << QStringLiteral(", lines available = ")
            << linesAvailable << QStringLiteral(", line spacing = ") << smallerFontMetrics.lineSpacing());

    if ((linesForText > linesAvailable) && (linesAvailable > 0))
    {
        double multiple = static_cast<double>(linesForText) / linesAvailable;
        int textSize = text.size();
        int newTextSize = static_cast<int>(textSize / multiple);
        int redundantSize = textSize - newTextSize - 3;
        if (redundantSize > 0) {
            QNTRACE(QStringLiteral("Chopping ") << redundantSize << QStringLiteral(" off the text: original size = ")
                    << textSize << QStringLiteral(", more appropriate text size = ") << newTextSize
                    << QStringLiteral(", lines for text without eliding = ") << linesForText
                    << QStringLiteral(", lines available = ") << linesAvailable);
            text.chop(redundantSize);
            QNTRACE(QStringLiteral("Text after chopping: ") << text);
        }
    }

    painter->setFont(originalFont);

    originalPen = painter->pen();

    bool textIsEmpty = text.isEmpty();
    if (textIsEmpty)
    {
        text = tr("Empty note");

        QPen pen = originalPen;
        pen.setColor(option.palette.highlight().color());
        painter->setPen(pen);
    }

    QTextOption textOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop));
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    painter->drawText(QRectF(previewTextRect), text, textOption);

    if (textIsEmpty) {
        painter->setPen(originalPen);
    }

    // 4) Painting the thumbnail (if any)
    if (!thumbnail.isNull())
    {
        int top = option.rect.top() + m_topMargin;
        int bottom = option.rect.bottom() - m_bottomMargin;
        QRect thumbnailRect(option.rect.width() - 110, top, 100, bottom);

        QNTRACE(QStringLiteral("Thumbnail rect: top = ") << thumbnailRect.top() << QStringLiteral(", bottom = ")
                << thumbnailRect.bottom() << QStringLiteral(", left = ") << thumbnailRect.left()
                << QStringLiteral(", right = ") << thumbnailRect.right());
        painter->drawImage(thumbnailRect, thumbnail, thumbnail.rect());
    }

    painter->restore();
}

void NoteItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void NoteItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize NoteItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return m_defaultSize;
}

void NoteItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

} // namespace quentier
