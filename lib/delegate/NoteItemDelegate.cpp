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

#include "NoteItemDelegate.h"
#include "AbstractStyledItemDelegate.h"

#include <lib/model/note/NoteModel.h>
#include <lib/preferences/defaults/Appearance.h>
#include <lib/view/NoteListView.h>

#include <quentier/logging/QuentierLogger.h>

#include <QDateTime>
#include <QListView>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QTextOption>

#include <cmath>
#include <limits>

namespace quentier {

NoteItemDelegate::NoteItemDelegate(QObject * parent) :
    QStyledItemDelegate{parent},
    m_showThumbnailsForAllNotes{preferences::defaults::showNoteThumbnails},
    m_minWidth{220}, m_minHeight{120}, m_leftMargin{6}, m_rightMargin{6},
    m_topMargin{6}, m_bottomMargin{6}
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
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    const auto * noteModel = qobject_cast<const NoteModel *>(index.model());
    if (Q_UNLIKELY(!noteModel)) {
        ErrorString error{
            QT_TR_NOOP("Wrong model is connected to the note item delegate")};

        QNWARNING("delegate::NoteItemDelegate", error);
        Q_EMIT notifyError(error);
        return;
    }

    const auto * item = noteModel->itemForIndex(index);
    if (Q_UNLIKELY(!item)) {
        ErrorString error{QT_TR_NOOP(
            "Can't retrieve the item to paint for note model index")};

        QNWARNING("delegate::NoteItemDelegate", error);
        Q_EMIT notifyError(error);
        return;
    }

    painter->save();

    painter->setRenderHints(
        QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
    }
    else {
        painter->fillRect(option.rect, option.palette.window());
        painter->setPen(option.palette.windowText().color());
    }

    auto * view = qobject_cast<QListView *>(parent());
    if (view) {
        const auto currentIndex = view->currentIndex();
        if (currentIndex.isValid() && (index.row() == currentIndex.row())) {
            QPen pen{Qt::SolidLine};
            pen.setWidth(2);

            if (option.state & QStyle::State_Selected) {
                pen.setColor(option.palette.highlightedText().color());
            }
            else {
                pen.setColor(option.palette.highlight().color());
            }

            painter->setPen(pen);

            const int dx1 = 1;
            const int dy1 = 1;
            const int dx2 = -1;
            const int dy2 = -1;

            painter->drawRoundedRect(
                QRectF{option.rect.adjusted(dx1, dy1, dx2, dy2)}, 2, 2);
        }
    }

    // Draw the bottom border line for all notes
    QPen pen{Qt::SolidLine};
    pen.setWidth(1);

    if (option.state & QStyle::State_Selected) {
        pen.setColor(option.palette.highlightedText().color());
    }
    else {
        pen.setColor(option.palette.windowText().color());
    }

    painter->setPen(pen);

    QLine bottomBoundaryLine{
        option.rect.bottomLeft(), option.rect.bottomRight()};

    painter->drawLine(bottomBoundaryLine);

    // Painting the text parts of note item
    QFont boldFont = option.font;
    boldFont.setBold(true);
    const QFontMetrics boldFontMetrics{boldFont};

    QFont smallerFont = option.font;
    smallerFont.setPointSizeF(smallerFont.pointSize() - 1.0);
    QFontMetrics smallerFontMetrics(smallerFont);

    const QByteArray & thumbnailData = item->thumbnailData();

    const int left = option.rect.left() + m_leftMargin;
    int width = option.rect.width() - m_leftMargin - m_rightMargin;

    const QString & noteLocalId = item->localId();
    bool showThumbnail = m_showThumbnailsForAllNotes &&
        (!m_hideThumbnailsLocalIds.contains(noteLocalId));

    if (showThumbnail && !thumbnailData.isEmpty()) {
        // 100 is the width of the thumbnail and 4 is a little margin
        width -= 104;
    }

    // Painting the title (or a piece of preview text if there's no title)
    painter->setFont(boldFont);

    QRect titleRect{
        left, option.rect.top() + m_topMargin, width, boldFontMetrics.height()};

    QString title = item->title();
    if (title.isEmpty()) {
        title = item->previewText();
    }

    title = title.simplified();

    if (title.isEmpty()) {
        if (option.state & QStyle::State_Selected) {
            painter->setPen(option.palette.color(
                QPalette::Active, QPalette::HighlightedText));
        }
        else {
            painter->setPen(
                option.palette.color(QPalette::Active, QPalette::Highlight));
        }

        title = tr("Empty note");
    }
    else if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.highlightedText().color());
    }
    else {
        painter->setPen(option.palette.windowText().color());
    }

    title =
        boldFontMetrics.elidedText(title, Qt::ElideRight, titleRect.width());

    painter->drawText(
        QRectF{titleRect}, title,
        QTextOption{Qt::Alignment{Qt::AlignLeft | Qt::AlignVCenter}});

    // Painting the created/modified datetime
    const qint64 creationTimestamp = item->creationTimestamp();
    const qint64 modificationTimestamp = item->modificationTimestamp();
    const qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

    QString createdDisplayText, modifiedDisplayText;
    if (creationTimestamp >= 0) {
        const qint64 msecsSinceCreation = currentTimestamp - creationTimestamp;
        createdDisplayText =
            timestampToString(creationTimestamp, msecsSinceCreation);
    }

    if (modificationTimestamp >= 0) {
        const qint64 msecsSinceModification =
            currentTimestamp - modificationTimestamp;
        modifiedDisplayText =
            timestampToString(modificationTimestamp, msecsSinceModification);
    }

    QString displayText;
    if (createdDisplayText.isEmpty() && modifiedDisplayText.isEmpty()) {
        displayText = QStringLiteral("(") +
            tr("No creation or modification datetime") + QStringLiteral(")");
    }
    else {
        if (!createdDisplayText.isEmpty()) {
            createdDisplayText.prepend(tr("Created") + QStringLiteral(": "));
        }

        if (!modifiedDisplayText.isEmpty()) {
            QString modificationTextPrefix =
                tr("modified") + QStringLiteral(": ");

            if (createdDisplayText.isEmpty()) {
                modificationTextPrefix =
                    modificationTextPrefix.at(0).toUpper() +
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

    const QRect dateTimeRect{
        left, titleRect.bottom() + m_topMargin, width,
        smallerFontMetrics.height()};

    displayText = smallerFontMetrics.elidedText(
        displayText, Qt::ElideRight, dateTimeRect.width());

    painter->setFont(smallerFont);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(
            option.palette.color(QPalette::Active, QPalette::HighlightedText));
    }
    else {
        painter->setPen(
            option.palette.color(QPalette::Active, QPalette::Highlight));
    }

    painter->drawText(
        QRectF{dateTimeRect}, displayText,
        QTextOption{Qt::Alignment{Qt::AlignLeft | Qt::AlignVCenter}});

    // Painting the preview text
    const int previewTextTop = dateTimeRect.bottom() + m_topMargin;

    const QRect previewTextRect{
        left, previewTextTop, width,
        option.rect.bottom() - m_bottomMargin - previewTextTop};

    QNTRACE(
        "delegate::NoteItemDelegate",
        "Preview text rect: top = " << previewTextRect.top()
                                    << ", bottom = " << previewTextRect.bottom()
                                    << ", left = " << previewTextRect.left()
                                    << ", right = " << previewTextRect.right()
                                    << "; height = " << previewTextRect.height()
                                    << ", width = " << previewTextRect.width());

    QString text = item->previewText().simplified();
    const QFontMetrics originalFontMetrics{option.font};

    QNTRACE("delegate::NoteItemDelegate", "Preview text: " << text);

    const int linesForText = static_cast<int>(std::floor(
        fontMetricsWidth(originalFontMetrics, text) / previewTextRect.width() +
        0.5));

    const int linesAvailable = static_cast<int>(std::floor(
        static_cast<double>(previewTextRect.height()) /
        originalFontMetrics.lineSpacing()));

    QNTRACE(
        "delegate::NoteItemDelegate",
        "Lines for text = "
            << linesForText << ", lines available = " << linesAvailable
            << ", line spacing = " << smallerFontMetrics.lineSpacing());

    if ((linesForText > linesAvailable) && (linesAvailable > 0)) {
        const double multiple =
            static_cast<double>(linesForText) / linesAvailable;

        Q_ASSERT(text.size() <= std::numeric_limits<int>::max());
        const int textSize = static_cast<int>(text.size());
        const int newTextSize = static_cast<int>(textSize / multiple);

        const int redundantSize = textSize - newTextSize - 3;
        if (redundantSize > 0) {
            QNTRACE(
                "delegate::NoteItemDelegate",
                "Chopping "
                    << redundantSize
                    << " off the text: original size = " << textSize
                    << ", more appropriate text size = " << newTextSize
                    << ", lines for text without eliding = " << linesForText
                    << ", lines available = " << linesAvailable);

            text.chop(redundantSize);
            QNTRACE("delegate::NoteItemDelegate", "Text after chopping: " << text);
        }
    }

    painter->setFont(option.font);

    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.base().color());
    }
    else {
        painter->setPen(option.palette.windowText().color());
    }

    const bool textIsEmpty = text.isEmpty();
    if (textIsEmpty) {
        text = QStringLiteral("(") + tr("Note without content") +
            QStringLiteral(")");

        QPen pen = painter->pen();
        if (option.state & QStyle::State_Selected) {
            pen.setColor(option.palette.highlightedText().color());
        }
        else {
            pen.setColor(option.palette.highlight().color());
        }
        painter->setPen(pen);
    }

    QTextOption textOption{Qt::Alignment{Qt::AlignLeft | Qt::AlignTop}};
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    painter->drawText(QRectF{previewTextRect}, text, textOption);

    // Painting the thumbnail (if any)
    if (showThumbnail && !thumbnailData.isEmpty()) {
        const int top = option.rect.top() + m_topMargin;
        const int bottom = option.rect.bottom() - m_bottomMargin;

        QRect thumbnailRect{
            option.rect.right() - m_rightMargin - 100, top, 100, bottom - top};

        QNTRACE(
            "delegate::NoteItemDelegate",
            "Thumbnail rect: top = " << thumbnailRect.top()
                                     << ", bottom = " << thumbnailRect.bottom()
                                     << ", left = " << thumbnailRect.left()
                                     << ", right = " << thumbnailRect.right());

        if (option.state & QStyle::State_Selected) {
            painter->setPen(option.palette.highlightedText().color());
        }
        else {
            painter->setPen(option.palette.windowText().color());
        }

        QImage thumbnail;
        Q_UNUSED(thumbnail.loadFromData(thumbnailData, "PNG"))
        painter->drawImage(thumbnailRect, thumbnail);
    }

    auto * noteListView = qobject_cast<NoteListView *>(view);
    if (!noteListView) {
        painter->restore();
        return;
    }

    const auto & currentAccount = noteListView->currentAccount();
    if (currentAccount.type() == Account::Type::Local) {
        painter->restore();
        return;
    }

    // Fill the triangle in the right upper corner if the note is modified
    if (item->isDirty() && item->isSynchronizable()) {
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

        painter->fillPath(
            modifiedTrianglePath,
            ((option.state & QStyle::State_Selected)
                 ? option.palette.highlightedText()
                 : option.palette.highlight()));

        // Also paint a little arrow here as an indication of the fact that
        // the note is about to be sent to the remote server
        QPainterPath arrowVerticalLinePath;

        const int arrowVerticalLineHorizontalOffset =
            static_cast<int>(std::floor(triangleSide * 0.7) + 0.5);

        const int arrowVerticalLineVerticalOffset =
            static_cast<int>(std::floor(triangleSide * 0.5) + 0.5);

        QPoint arrowBottomPoint{
            topLeft.x() + arrowVerticalLineHorizontalOffset,
            topLeft.y() + arrowVerticalLineVerticalOffset};

        arrowVerticalLinePath.moveTo(arrowBottomPoint);

        QPoint arrowTopPoint(
            arrowBottomPoint.x(),
            arrowBottomPoint.y() -
                static_cast<int>(
                    std::floor(arrowVerticalLineVerticalOffset * 0.8) + 0.5));

        arrowVerticalLinePath.lineTo(arrowTopPoint);

        const int arrowPointerOffset = static_cast<int>(
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

        arrowPen.setBrush(
            (option.state & QStyle::State_Selected)
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

    const int width = std::max(m_minWidth, option.rect.width());

    /**
     * Computing height: need to compute all its components as if we were
     * painting and ensure the integer number of preview text lines would
     * correspond to the preview text's part of the overall height - this way
     * note items look better
     */

    QFont boldFont = option.font;
    boldFont.setBold(true);
    const QFontMetrics boldFontMetrics{boldFont};

    const int titleHeight = boldFontMetrics.height();

    QFont smallerFont = option.font;
    smallerFont.setPointSizeF(smallerFont.pointSize() - 1.0);
    const QFontMetrics smallerFontMetrics{smallerFont};

    const int dateTimeHeight = smallerFontMetrics.height();

    const int titleAndDateTimeHeightPart =
        3 * m_topMargin + titleHeight + dateTimeHeight;

    const int remainingTextHeight = static_cast<int>(std::max(
        static_cast<double>(m_minHeight - titleAndDateTimeHeightPart), 0.0));

    const QFontMetrics fontMetrics{option.font};
    const int numFontHeights = static_cast<int>(
        std::ceil(remainingTextHeight / fontMetrics.lineSpacing()));

    // NOTE: the last item is an ad-hoc constant which in some cases seems
    // to make a good difference
    const int height = titleAndDateTimeHeightPart +
        numFontHeights * fontMetrics.lineSpacing() + fontMetrics.leading() +
        m_bottomMargin + 2;

    return QSize{width, height};
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
    const QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(timestamp);
    const QDateTime today = QDateTime::currentDateTime();

    const QDate targetDate = dateTime.date();
    const QDate todayDate = today.date();
    const QDate yesterdayDate = todayDate.addDays(-1);

    const double timePassedDouble = static_cast<double>(timePassed);

    constexpr int millisecondsPerDay = 864e5;
    constexpr int millisecondsPerWeek = 6048e5;

    QString text;
    if (timePassedDouble > millisecondsPerWeek) {
        const int pastWeeks = static_cast<int>(
            std::floor(timePassedDouble / millisecondsPerWeek + 0.5));

        if (pastWeeks == 1) {
            text = tr("last week");
        }
        else if (pastWeeks < 5) {
            text = tr("%n weeks ago", nullptr, pastWeeks);
        }
    }
    else if (targetDate == yesterdayDate) {
        text = tr("yesterday");
    }
    else if (targetDate == todayDate) {
        QTime time = dateTime.time();
        text = tr("today at") + QStringLiteral(" ") +
            QLocale{}.toString(time, QLocale::ShortFormat);
    }
    else if (timePassedDouble > millisecondsPerDay) {
        int pastDays = static_cast<int>(
            std::floor(timePassedDouble / millisecondsPerDay + 0.5));

        if (pastDays < 6) {
            text = tr("%n days ago", nullptr, pastDays);
        }
    }

    if (text.isEmpty()) {
        text = QLocale{}.toString(
            QDateTime::fromMSecsSinceEpoch(timestamp), QLocale::ShortFormat);
    }

    return text;
}

void NoteItemDelegate::setShowNoteThumbnailsState(
    bool showThumbnailsForAllNotes,
    const QSet<QString> & hideThumbnailsLocalIds)
{
    QNDEBUG("delegate", "NoteItemDelegate::setShowNoteThumbnailsState");
    m_showThumbnailsForAllNotes = showThumbnailsForAllNotes;
    m_hideThumbnailsLocalIds = hideThumbnailsLocalIds;
}

} // namespace quentier
