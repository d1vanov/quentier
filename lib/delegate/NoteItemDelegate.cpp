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

#include "NoteItemDelegate.h"

#include <lib/model/NoteModel.h>
#include <lib/view/NoteListView.h>
#include <lib/preferences/DefaultSettings.h>

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
    m_showThumbnailsForAllNotes(DEFAULT_SHOW_NOTE_THUMBNAILS),
    m_minWidth(220),
    m_minHeight(120),
    m_leftMargin(6),
    m_rightMargin(6),
    m_topMargin(6),
    m_bottomMargin(6)
{}

QWidget * NoteItemDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return nullptr;
}

void NoteItemDelegate::paint(
    QPainter * pPainter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    const NoteModel * pNoteModel =
        qobject_cast<const NoteModel*>(index.model());
    if (Q_UNLIKELY(!pNoteModel))
    {
        ErrorString error(QT_TR_NOOP("Wrong model is connected to the note item "
                                     "delegate"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    const NoteModelItem * pItem = pNoteModel->itemForIndex(index);
    if (Q_UNLIKELY(!pItem))
    {
        ErrorString error(QT_TR_NOOP("Can't retrieve the item to paint for note "
                                     "model index"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    pPainter->save();
    pPainter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        pPainter->fillRect(option.rect, option.palette.highlight());
        pPainter->setPen(option.palette.highlightedText().color());
    }
    else {
        pPainter->fillRect(option.rect, option.palette.window());
        pPainter->setPen(option.palette.windowText().color());
    }

    QListView * pView = qobject_cast<QListView*>(parent());
    if (pView)
    {
        QModelIndex currentIndex = pView->currentIndex();
        if (currentIndex.isValid() && (index.row() == currentIndex.row()))
        {
            QPen pen(Qt::SolidLine);
            pen.setWidth(2);

            if (option.state & QStyle::State_Selected) {
                pen.setColor(option.palette.highlightedText().color());
            }
            else {
                pen.setColor(option.palette.highlight().color());
            }

            pPainter->setPen(pen);

            int dx1 = 1;
            int dy1 = 1;
            int dx2 = -1;
            int dy2 = -1;
            pPainter->drawRoundedRect(
                QRectF(option.rect.adjusted(dx1, dy1, dx2, dy2)), 2, 2);
        }
    }

    // Draw the bottom border line for all notes
    QPen pen(Qt::SolidLine);
    pen.setWidth(1);

    if (option.state & QStyle::State_Selected) {
        pen.setColor(option.palette.highlightedText().color());
    }
    else {
        pen.setColor(option.palette.windowText().color());
    }

    pPainter->setPen(pen);

    QLine bottomBoundaryLine(option.rect.bottomLeft(), option.rect.bottomRight());
    pPainter->drawLine(bottomBoundaryLine);

    // Painting the text parts of note item
    QFont boldFont = option.font;
    boldFont.setBold(true);
    QFontMetrics boldFontMetrics(boldFont);

    QFont smallerFont = option.font;
    smallerFont.setPointSizeF(smallerFont.pointSize() - 1.0);
    QFontMetrics smallerFontMetrics(smallerFont);

    const QByteArray & thumbnailData = pItem->thumbnailData();

    int left = option.rect.left() + m_leftMargin;
    int width = option.rect.width() - m_leftMargin - m_rightMargin;

    const QString & noteLocalUid = pItem->localUid();
    bool showThumbnail =
        m_showThumbnailsForAllNotes &&
        (!m_hideThumbnailsLocalUids.contains(noteLocalUid));

    if (showThumbnail && !thumbnailData.isEmpty()) {
        width -= 104; // 100 is the width of the thumbnail and 4 is a little margin
    }

    // Painting the title (or a piece of preview text if there's no title)
    pPainter->setFont(boldFont);

    QRect titleRect(left, option.rect.top() + m_topMargin,
                    width, boldFontMetrics.height());

    QString title = pItem->title();
    if (title.isEmpty()) {
        title = pItem->previewText();
    }

    title = title.simplified();

    if (title.isEmpty())
    {
        if (option.state & QStyle::State_Selected) {
            pPainter->setPen(option.palette.color(QPalette::Active,
                                                  QPalette::HighlightedText));
        }
        else {
            pPainter->setPen(option.palette.color(QPalette::Active,
                                                  QPalette::Highlight));
        }

        title = tr("Empty note");
    }
    else if (option.state & QStyle::State_Selected)
    {
        pPainter->setPen(option.palette.highlightedText().color());
    }
    else
    {
        pPainter->setPen(option.palette.windowText().color());
    }

    title = boldFontMetrics.elidedText(title, Qt::ElideRight, titleRect.width());
    pPainter->drawText(QRectF(titleRect), title,
                       QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));

    // Painting the created/modified datetime
    qint64 creationTimestamp = pItem->creationTimestamp();
    qint64 modificationTimestamp = pItem->modificationTimestamp();

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

    QString createdDisplayText, modifiedDisplayText;
    if (creationTimestamp >= 0) {
        qint64 msecsSinceCreation = currentTimestamp - creationTimestamp;
        createdDisplayText = timestampToString(creationTimestamp,
                                               msecsSinceCreation);
    }

    if (modificationTimestamp >= 0) {
        qint64 msecsSinceModification = currentTimestamp - modificationTimestamp;
        modifiedDisplayText = timestampToString(modificationTimestamp,
                                                msecsSinceModification);
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

    QRect dateTimeRect(left, titleRect.bottom() + m_topMargin,
                       width, smallerFontMetrics.height());
    displayText = smallerFontMetrics.elidedText(displayText, Qt::ElideRight,
                                                dateTimeRect.width());

    pPainter->setFont(smallerFont);

    if (option.state & QStyle::State_Selected) {
        pPainter->setPen(option.palette.color(QPalette::Active,
                                              QPalette::HighlightedText));
    }
    else {
        pPainter->setPen(option.palette.color(QPalette::Active,
                                              QPalette::Highlight));
    }

    pPainter->drawText(QRectF(dateTimeRect), displayText,
                       QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));

    // Painting the preview text
    int previewTextTop = dateTimeRect.bottom() + m_topMargin;
    QRect previewTextRect(left, previewTextTop, width,
                          option.rect.bottom() - m_bottomMargin - previewTextTop);

    QNTRACE("Preview text rect: top = " << previewTextRect.top()
            << ", bottom = " << previewTextRect.bottom()
            << ", left = " << previewTextRect.left()
            << ", right = " << previewTextRect.right()
            << "; height = " << previewTextRect.height()
            << ", width = " << previewTextRect.width());

    QString text = pItem->previewText().simplified();
    QFontMetrics originalFontMetrics(option.font);

    QNTRACE("Preview text: " << text);

    int linesForText = static_cast<int>(
        std::floor(originalFontMetrics.width(text) /
                   previewTextRect.width() + 0.5));
    int linesAvailable = static_cast<int>(
        std::floor(static_cast<double>(previewTextRect.height()) /
                                       originalFontMetrics.lineSpacing()));

    QNTRACE("Lines for text = " << linesForText
            << ", lines available = " << linesAvailable
            << ", line spacing = " << smallerFontMetrics.lineSpacing());

    if ((linesForText > linesAvailable) && (linesAvailable > 0))
    {
        double multiple = static_cast<double>(linesForText) / linesAvailable;
        int textSize = text.size();
        int newTextSize = static_cast<int>(textSize / multiple);
        int redundantSize = textSize - newTextSize - 3;
        if (redundantSize > 0)
        {
            QNTRACE("Chopping " << redundantSize
                    << " off the text: original size = "
                    << textSize << ", more appropriate text size = "
                    << newTextSize << ", lines for text without eliding = "
                    << linesForText << ", lines available = " << linesAvailable);
            text.chop(redundantSize);
            QNTRACE("Text after chopping: " << text);
        }
    }

    pPainter->setFont(option.font);

    if (option.state & QStyle::State_Selected) {
        pPainter->setPen(option.palette.base().color());
    }
    else {
        pPainter->setPen(option.palette.windowText().color());
    }

    bool textIsEmpty = text.isEmpty();
    if (textIsEmpty)
    {
        text = QStringLiteral("(") + tr("Note without content") +
               QStringLiteral(")");

        QPen pen = pPainter->pen();
        if (option.state & QStyle::State_Selected) {
            pen.setColor(option.palette.highlightedText().color());
        }
        else {
            pen.setColor(option.palette.highlight().color());
        }
        pPainter->setPen(pen);
    }

    QTextOption textOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop));
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    pPainter->drawText(QRectF(previewTextRect), text, textOption);

    // Painting the thumbnail (if any)
    if (showThumbnail && !thumbnailData.isEmpty())
    {
        int top = option.rect.top() + m_topMargin;
        int bottom = option.rect.bottom() - m_bottomMargin;
        QRect thumbnailRect(option.rect.right() - m_rightMargin - 100,
                            top, 100, bottom - top);

        QNTRACE("Thumbnail rect: top = " << thumbnailRect.top()
                << ", bottom = " << thumbnailRect.bottom()
                << ", left = " << thumbnailRect.left()
                << ", right = " << thumbnailRect.right());

        if (option.state & QStyle::State_Selected) {
            pPainter->setPen(option.palette.highlightedText().color());
        }
        else {
            pPainter->setPen(option.palette.windowText().color());
        }

        QImage thumbnail;
        Q_UNUSED(thumbnail.loadFromData(thumbnailData, "PNG"))
        pPainter->drawImage(thumbnailRect, thumbnail);
    }

    NoteListView * pNoteListView = qobject_cast<NoteListView*>(pView);
    if (!pNoteListView) {
        pPainter->restore();
        return;
    }

    const Account & currentAccount = pNoteListView->currentAccount();
    if (currentAccount.type() == Account::Type::Local) {
        pPainter->restore();
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

        pPainter->setPen(Qt::NoPen);
        pPainter->fillPath(modifiedTrianglePath,
                           ((option.state & QStyle::State_Selected)
                            ? option.palette.highlightedText()
                            : option.palette.highlight()));

        // Also paint a little arrow here as an indication of the fact that
        // the note is about to be sent to the remote server
        QPainterPath arrowVerticalLinePath;
        int arrowVerticalLineHorizontalOffset =
            static_cast<int>(std::floor(triangleSide * 0.7) + 0.5);
        int arrowVerticalLineVerticalOffset =
            static_cast<int>(std::floor(triangleSide * 0.5) + 0.5);
        QPoint arrowBottomPoint(topLeft.x() + arrowVerticalLineHorizontalOffset,
                                topLeft.y() + arrowVerticalLineVerticalOffset);
        arrowVerticalLinePath.moveTo(arrowBottomPoint);
        QPoint arrowTopPoint(
            arrowBottomPoint.x(),
            arrowBottomPoint.y() -
            static_cast<int>(std::floor(arrowVerticalLineVerticalOffset * 0.8) + 0.5));
        arrowVerticalLinePath.lineTo(arrowTopPoint);

        int arrowPointerOffset =
            static_cast<int>(
                std::floor((arrowBottomPoint.y() - arrowTopPoint.y()) * 0.3) + 0.5);

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

        pPainter->setPen(arrowPen);
        pPainter->drawPath(arrowVerticalLinePath);
        pPainter->drawPath(arrowPointerLeftPartPath);
        pPainter->drawPath(arrowPointerRightPartPath);
    }

    pPainter->restore();
}

void NoteItemDelegate::setEditorData(
    QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void NoteItemDelegate::setModelData(
    QWidget * editor, QAbstractItemModel * model,
    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize NoteItemDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(index)

    int width = std::max(m_minWidth, option.rect.width());

    /**
     * Computing height: need to compute all its components as if we were
     * painting and ensure the integer number of preview text lines would
     * correspond to the preview text's part of the overall height - this way
     * note items look better
     */

    QFont boldFont = option.font;
    boldFont.setBold(true);
    QFontMetrics boldFontMetrics(boldFont);

    int titleHeight = boldFontMetrics.height();

    QFont smallerFont = option.font;
    smallerFont.setPointSizeF(smallerFont.pointSize() - 1.0);
    QFontMetrics smallerFontMetrics(smallerFont);

    int dateTimeHeight = smallerFontMetrics.height();

    int titleAndDateTimeHeightPart = 3 * m_topMargin + titleHeight + dateTimeHeight;

    int remainingTextHeight =
        static_cast<int>(
            std::max(static_cast<double>(m_minHeight -
                                         titleAndDateTimeHeightPart), 0.0));

    QFontMetrics fontMetrics(option.font);
    int numFontHeights =
        static_cast<int>(
            std::ceil(remainingTextHeight / fontMetrics.lineSpacing()));

    // NOTE: the last item is an ad-hoc constant which in some cases seems
    // to make a good difference
    int height = titleAndDateTimeHeightPart +
                 numFontHeights * fontMetrics.lineSpacing() +
                 fontMetrics.leading() + m_bottomMargin + 2;

    return QSize(width, height);
}

void NoteItemDelegate::updateEditorGeometry(
    QWidget * editor, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

QString NoteItemDelegate::timestampToString(
    const qint64 timestamp, const qint64 timePassed) const
{
    QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(timestamp);
    QDateTime today = QDateTime::currentDateTime();

    QDate targetDate = dateTime.date();
    QDate todayDate = today.date();
    QDate yesterdayDate = todayDate.addDays(-1);

    QString text;
    if (timePassed > MSEC_PER_WEEK)
    {
        int pastWeeks =
            static_cast<int>(std::floor(timePassed / MSEC_PER_WEEK + 0.5));

        if (pastWeeks == 1) {
            text = tr("last week");
        }
        else if (pastWeeks < 5) {
            text = tr("%n weeks ago", nullptr, pastWeeks);
        }
    }
    else if (targetDate == yesterdayDate)
    {
        text = tr("yesterday");
    }
    else if (targetDate == todayDate)
    {
        QTime time = dateTime.time();
        text = tr("today at") + QStringLiteral(" ") +
               time.toString(Qt::DefaultLocaleShortDate);
    }
    else if (timePassed > MSEC_PER_DAY)
    {
        int pastDays = static_cast<int>(std::floor(timePassed / MSEC_PER_DAY + 0.5));
        if (pastDays < 6) {
            text = tr("%n days ago", nullptr, pastDays);
        }
    }

    if (text.isEmpty()) {
        text = QDateTime::fromMSecsSinceEpoch(timestamp)
               .toString(Qt::DefaultLocaleShortDate);
    }

    return text;
}

void NoteItemDelegate::setShowNoteThumbnailsState(
    bool showThumbnailsForAllNotes, const QSet<QString> & hideThumbnailsLocalUids)
{
    QNDEBUG("NoteItemDelegate::setShowNoteThumbnailsState");
    m_showThumbnailsForAllNotes = showThumbnailsForAllNotes;
    m_hideThumbnailsLocalUids = hideThumbnailsLocalUids;
}

} // namespace quentier
