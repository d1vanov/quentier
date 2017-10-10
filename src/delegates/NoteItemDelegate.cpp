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
#include "../views/NoteListView.h"
#include "../DefaultSettings.h"
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>
#include <QDateTime>
#include <QTextOption>
#include <QListView>
#include <cmath>

#define MSEC_PER_DAY (864e5)
#define MSEC_PER_WEEK (6048e5)

namespace quentier {

NoteItemDelegate::NoteItemDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_showNoteThumbnails(DEFAULT_SHOW_NOTE_THUMBNAILS),
    m_minWidth(220),
    m_height(120),
    m_leftMargin(2),
    m_rightMargin(4),
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
        ErrorString error(QT_TR_NOOP("Wrong model is connected to the note item delegate"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pNoteFilterModel->sourceModel());
    if (Q_UNLIKELY(!pNoteModel)) {
        ErrorString error(QT_TR_NOOP("Can't get the source model from the note filter model connected to the note item delegate"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    QModelIndex sourceIndex = pNoteFilterModel->mapToSource(index);
    const NoteModelItem * pItem = pNoteModel->itemForIndex(sourceIndex);
    if (Q_UNLIKELY(!pItem)) {
        ErrorString error(QT_TR_NOOP("Can't retrieve the item to paint for note model index"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
    }
    else {
        painter->fillRect(option.rect, option.palette.window());
        painter->setPen(option.palette.windowText().color());
    }

    bool isLast = (index.row() == pNoteFilterModel->rowCount(QModelIndex()) - 1);

    int leftMargin = m_leftMargin;
    int rightMargin = m_rightMargin;
    int topMargin = m_topMargin;
    int bottomMargin = m_bottomMargin;

    QListView * pView = qobject_cast<QListView*>(parent());
    if (pView)
    {
        QModelIndex currentIndex = pView->currentIndex();
        if (currentIndex.isValid() && (sourceIndex.row() == currentIndex.row()))
        {
            QPen originalPen = painter->pen();
            QPen pen = originalPen;
            pen.setWidth(2);

            if (option.state & QStyle::State_Selected) {
                pen.setColor(option.palette.highlightedText().color());
            }
            else {
                pen.setColor(option.palette.highlight().color());
            }

            painter->setPen(pen);

            int dx1 = 1;
            int dy1 = 1;
            int dx2 = -1;
            int dy2 = (isLast ? -2 : -3);
            painter->drawRoundedRect(QRectF(option.rect.adjusted(dx1, dy1, dx2, dy2)), 2, 2);

            painter->setPen(originalPen);

            leftMargin += 2;
            rightMargin += 2;
            topMargin += 2;
            bottomMargin += (isLast ? 3 : 2);
        }
    }

    // Draw the bottom border line for all notes but the last (lowest) one
    if (!isLast)
    {
        QPen originalPen = painter->pen();
        QPen pen = originalPen;
        pen.setWidth(1);

        if (option.state & QStyle::State_Selected) {
            pen.setColor(option.palette.highlightedText().color());
        }
        else {
            pen.setColor(option.palette.windowText().color());
        }

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

    int left = option.rect.left() + leftMargin;
    int width = option.rect.width() - leftMargin - rightMargin;
    if (m_showNoteThumbnails && !thumbnail.isNull()) {
        width -= 100;
    }

    // Painting the title (or a piece of preview text if there's no title)
    QFont originalFont = painter->font();
    painter->setFont(boldFont);

    QRect titleRect(left, option.rect.top() + topMargin, width, boldFontMetrics.height());

    QString title = pItem->title();
    if (title.isEmpty()) {
        title = pItem->previewText();
    }

    title = title.simplified();

    if (title.isEmpty())
    {
        if (option.state & QStyle::State_Selected) {
            painter->setPen(option.palette.color(QPalette::Active, QPalette::HighlightedText));
        }
        else {
            painter->setPen(option.palette.color(QPalette::Active, QPalette::Highlight));
        }

        title = tr("Empty note");
    }
    else if (option.state & QStyle::State_Selected)
    {
        painter->setPen(option.palette.highlightedText().color());
    }
    else
    {
        painter->setPen(option.palette.windowText().color());
    }

    title = boldFontMetrics.elidedText(title, Qt::ElideRight, titleRect.width());
    painter->drawText(QRectF(titleRect), title, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));

    // Painting the created/modified datetime
    qint64 creationTimestamp = pItem->creationTimestamp();
    qint64 modificationTimestamp = pItem->modificationTimestamp();

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

    QString createdDisplayText, modifiedDisplayText;
    if (creationTimestamp >= 0) {
        qint64 msecsSinceCreation = currentTimestamp - creationTimestamp;
        createdDisplayText = timestampToString(creationTimestamp, msecsSinceCreation);
    }

    if (modificationTimestamp >= 0) {
        qint64 msecsSinceModification = currentTimestamp - modificationTimestamp;
        modifiedDisplayText = timestampToString(modificationTimestamp, msecsSinceModification);
    }

    QString displayText;
    if (createdDisplayText.isEmpty() && modifiedDisplayText.isEmpty())
    {
        displayText = QStringLiteral("(") +
                      tr("No creation or modification datetime") +
                      QStringLiteral(")");
    }
    else
    {
        if (!createdDisplayText.isEmpty()) {
            createdDisplayText.prepend(tr("Created") + QStringLiteral(": "));
        }

        if (!modifiedDisplayText.isEmpty())
        {
            QString modificationTextPrefix = tr("modified") + QStringLiteral(": ");
            if (createdDisplayText.isEmpty()) {
                modificationTextPrefix = modificationTextPrefix.at(0).toUpper() +
                                         modificationTextPrefix.mid(1);
            }

            modifiedDisplayText.prepend(modificationTextPrefix);
        }

        displayText = createdDisplayText;
        if (!displayText.isEmpty()) {
            displayText += QStringLiteral(", ");
        }

        displayText += modifiedDisplayText;
    }

    QRect dateTimeRect(left, titleRect.bottom() + bottomMargin + topMargin,
                       width, smallerFontMetrics.height());
    displayText = smallerFontMetrics.elidedText(displayText, Qt::ElideRight,
                                                dateTimeRect.width());

    painter->setFont(smallerFont);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.color(QPalette::Active, QPalette::HighlightedText));
    }
    else {
        painter->setPen(option.palette.color(QPalette::Active, QPalette::Highlight));
    }

    painter->drawText(QRectF(dateTimeRect), displayText, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));

    // Painting the preview text
    int previewTextTop = dateTimeRect.bottom() + topMargin;
    QRect previewTextRect(left, previewTextTop, width, option.rect.bottom() - bottomMargin - previewTextTop);

    QNTRACE(QStringLiteral("Preview text rect: top = ") << previewTextRect.top() << QStringLiteral(", bottom = ")
            << previewTextRect.bottom() << QStringLiteral(", left = ") << previewTextRect.left()
            << QStringLiteral(", right = ") << previewTextRect.right() << QStringLiteral("; height = ")
            << previewTextRect.height() << QStringLiteral(", width = ") << previewTextRect.width());

    QString text = pItem->previewText().simplified();
    QFontMetrics originalFontMetrics(originalFont);

    QNTRACE(QStringLiteral("Preview text: ") << text);

    int linesForText = static_cast<int>(std::floor(originalFontMetrics.width(text) / previewTextRect.width() + 0.5));
    int linesAvailable = static_cast<int>(std::floor(static_cast<double>(previewTextRect.height()) / smallerFontMetrics.lineSpacing()));

    QNTRACE(QStringLiteral("Lines for text = ") << linesForText << QStringLiteral(", lines available = ")
            << linesAvailable << QStringLiteral(", line spacing = ") << smallerFontMetrics.lineSpacing());

    if ((linesForText > linesAvailable) && (linesAvailable > 0))
    {
        double multiple = static_cast<double>(linesForText) / linesAvailable;
        int textSize = text.size();
        int newTextSize = static_cast<int>(textSize / multiple);
        int redundantSize = textSize - newTextSize - 3;
        if (redundantSize > 0)
        {
            QNTRACE(QStringLiteral("Chopping ") << redundantSize << QStringLiteral(" off the text: original size = ")
                    << textSize << QStringLiteral(", more appropriate text size = ") << newTextSize
                    << QStringLiteral(", lines for text without eliding = ") << linesForText
                    << QStringLiteral(", lines available = ") << linesAvailable);
            text.chop(redundantSize);
            QNTRACE(QStringLiteral("Text after chopping: ") << text);
        }
    }

    painter->setFont(originalFont);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.base().color());
    }
    else {
        painter->setPen(option.palette.windowText().color());
    }

    bool textIsEmpty = text.isEmpty();
    if (textIsEmpty)
    {
        text = QStringLiteral("(") + tr("Note without content") + QStringLiteral(")");

        QPen pen = painter->pen();
        if (option.state & QStyle::State_Selected) {
            pen.setColor(option.palette.highlightedText().color());
        }
        else {
            pen.setColor(option.palette.highlight().color());
        }
        painter->setPen(pen);
    }

    QTextOption textOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop));
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    painter->drawText(QRectF(previewTextRect), text, textOption);

    // Painting the thumbnail (if any)
    if (m_showNoteThumbnails && !thumbnail.isNull())
    {
        int top = option.rect.top() + topMargin;
        int bottom = option.rect.bottom() - bottomMargin;
        QRect thumbnailRect(option.rect.right() - 100, top, 100, bottom - top);

        QNTRACE(QStringLiteral("Thumbnail rect: top = ") << thumbnailRect.top() << QStringLiteral(", bottom = ")
                << thumbnailRect.bottom() << QStringLiteral(", left = ") << thumbnailRect.left()
                << QStringLiteral(", right = ") << thumbnailRect.right());

        if (option.state & QStyle::State_Selected) {
            painter->setPen(option.palette.highlightedText().color());
        }
        else {
            painter->setPen(option.palette.windowText().color());
        }

        painter->drawImage(thumbnailRect, thumbnail);
    }

    NoteListView * pNoteListView = qobject_cast<NoteListView*>(pView);
    if (!pNoteListView) {
        return;
    }

    const Account & currentAccount = pNoteListView->currentAccount();
    if (currentAccount.type() == Account::Type::Local) {
        return;
    }

    // Fill the triangle in the right upper corner if the note is modified
    if (pItem->isDirty() && pItem->isSynchronizable())
    {
        QPainterPath modifiedTrianglePath;
        int triangleSide = 20;

        QPoint topRight = option.rect.topRight();
        modifiedTrianglePath.moveTo(topRight);

        QPoint bottomRight = topRight;
        bottomRight.setY(topRight.y() + triangleSide);
        modifiedTrianglePath.lineTo(bottomRight);

        QPoint topLeft = topRight;
        topLeft.setX(topRight.x() - triangleSide);
        modifiedTrianglePath.lineTo(topLeft);

        painter->setPen(Qt::NoPen);
        painter->fillPath(modifiedTrianglePath, ((option.state & QStyle::State_Selected)
                                                 ? option.palette.highlightedText()
                                                 : option.palette.highlight()));

        // Also paint a little arrow here as an indication of the fact that the note is about to be sent
        // to the remote server
        QPainterPath arrowVerticalLinePath;
        int arrowVerticalLineHorizontalOffset = static_cast<int>(std::floor(triangleSide * 0.7) + 0.5);
        int arrowVerticalLineVerticalOffset = static_cast<int>(std::floor(triangleSide * 0.5) + 0.5);
        QPoint arrowBottomPoint(topLeft.x() + arrowVerticalLineHorizontalOffset,
                                topLeft.y() + arrowVerticalLineVerticalOffset);
        arrowVerticalLinePath.moveTo(arrowBottomPoint);
        QPoint arrowTopPoint(arrowBottomPoint.x(), arrowBottomPoint.y() - static_cast<int>(std::floor(arrowVerticalLineVerticalOffset * 0.8) + 0.5));
        arrowVerticalLinePath.lineTo(arrowTopPoint);

        int arrowPointerOffset = static_cast<int>(std::floor((arrowBottomPoint.y() - arrowTopPoint.y()) * 0.3) + 0.5);

        // Adjustments accounting for the fact we'd be using pen of width 2
        arrowTopPoint.ry() += 1;
        arrowTopPoint.rx() -= 1;

        QPainterPath arrowPointerLeftPartPath;
        arrowPointerLeftPartPath.moveTo(arrowTopPoint);
        QPoint arrowPointerLeftPartEndPoint = arrowTopPoint;
        arrowPointerLeftPartEndPoint.rx() -= arrowPointerOffset;
        arrowPointerLeftPartEndPoint.ry() += arrowPointerOffset;
        arrowPointerLeftPartPath.lineTo(arrowPointerLeftPartEndPoint);

        // Adjustment accounting for the fact we'd be using pen of width 2
        arrowTopPoint.rx() += 2;

        QPainterPath arrowPointerRightPartPath;
        arrowPointerRightPartPath.moveTo(arrowTopPoint);
        QPoint arrowPointerRightPartEndPoint = arrowTopPoint;
        arrowPointerRightPartEndPoint.rx() += arrowPointerOffset;
        arrowPointerRightPartEndPoint.ry() += arrowPointerOffset;
        arrowPointerRightPartPath.lineTo(arrowPointerRightPartEndPoint);

        QPen arrowPen;
        arrowPen.setBrush((option.state & QStyle::State_Selected)
                          ? option.palette.highlight()
                          : option.palette.highlightedText());
        arrowPen.setWidth(2);
        arrowPen.setStyle(Qt::SolidLine);
        arrowPen.setCapStyle(Qt::SquareCap);
        arrowPen.setJoinStyle(Qt::BevelJoin);

        painter->setPen(arrowPen);
        painter->drawPath(arrowVerticalLinePath);
        painter->drawPath(arrowPointerLeftPartPath);
        painter->drawPath(arrowPointerRightPartPath);
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
    Q_UNUSED(index)
    int width = std::max(m_minWidth, option.rect.width());
    return QSize(width, m_height);
}

void NoteItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

QString NoteItemDelegate::timestampToString(const qint64 timestamp, const qint64 timePassed) const
{
    QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(timestamp);
    QDateTime today = QDateTime::currentDateTime();

    QDate targetDate = dateTime.date();
    QDate todayDate = today.date();
    QDate yesterdayDate = todayDate.addDays(-1);

    QString text;
    if (timePassed > MSEC_PER_WEEK)
    {
        int pastWeeks = static_cast<int>(std::floor(timePassed / MSEC_PER_WEEK + 0.5));

        if (pastWeeks == 1) {
            text = tr("last week");
        }
        else if (pastWeeks < 5) {
            text = tr("%n weeks ago", Q_NULLPTR, pastWeeks);
        }
    }
    else if (targetDate == yesterdayDate)
    {
        text = tr("yesterday");
    }
    else if (targetDate == todayDate)
    {
        QTime time = dateTime.time();
        text = tr("today at") + QStringLiteral(" ") + time.toString(Qt::DefaultLocaleShortDate);
    }
    else if (timePassed > MSEC_PER_DAY)
    {
        int pastDays = static_cast<int>(std::floor(timePassed / MSEC_PER_DAY + 0.5));
        if (pastDays < 6) {
            text = tr("%n days ago", Q_NULLPTR, pastDays);
        }
    }

    if (text.isEmpty()) {
        text = QDateTime::fromMSecsSinceEpoch(timestamp).toString(Qt::DefaultLocaleShortDate);
    }

    return text;
}

} // namespace quentier
