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

#include "LogViewerDelegate.h"
#include "AbstractStyledItemDelegate.h"

#include <QDate>
#include <QFontMetrics>
#include <QPainter>
#include <QStringRef>
#include <QTime>

#include <cmath>

namespace quentier {

namespace {

constexpr int gMaxLogEntryLineSize = 150;
constexpr double gMargin = 0.4;

const QChar gNewlineChar = QChar::fromLatin1('\n');
const QChar gWhitespaceChar = QChar::fromLatin1(' ');

const QString gSampleSourceFileLineNumberString = QStringLiteral("99999");
const QString gSampleDateTimeString = QStringLiteral("26/09/2017 19:31:23:457");
const QString gWidestLogLevelName = QStringLiteral("Warning");

} // namespace

LogViewerDelegate::LogViewerDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

QWidget * LogViewerDelegate::createEditor(
    QWidget * pParent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(pParent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return nullptr;
}

void LogViewerDelegate::paint(
    QPainter * pPainter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (paintImpl(pPainter, option, index)) {
        return;
    }

    QStyledItemDelegate::paint(pPainter, option, index);
}

QSize LogViewerDelegate::sizeHint(
    const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // NOTE: this method is called many times when
    // QHeaderView::resizeSections(QHeaderView::ResizeToContents) is called.
    // It has to be very fast, otherwise the performance is complete crap
    // so there are some shortcuts and missing checks which should normally
    // be here

    const QFontMetrics fontMetrics{option.font};
    QSize size;

    const auto stringSizeHint = [&size, &fontMetrics](const QString & str)
    {
        size.setWidth(static_cast<int>(std::floor(
            fontMetricsWidth(fontMetrics, str) * (1.0 + gMargin) + 0.5)));
        size.setHeight(static_cast<int>(
            std::floor(fontMetrics.lineSpacing() * (1.0 + gMargin) + 0.5)));
        return size;
    };

    switch (static_cast<LogViewerModel::Column>(index.column())) {
    case LogViewerModel::Column::Timestamp:
        return stringSizeHint(gSampleDateTimeString);
    case LogViewerModel::Column::SourceFileLineNumber:
        return stringSizeHint(gSampleSourceFileLineNumberString);
    case LogViewerModel::Column::LogLevel:
        return stringSizeHint(gWidestLogLevelName);
    default:
        break;
    }

    // If we haven't returned yet, either the index is invalid or we are dealing
    // with either log entry column or source file name column or component
    // column

    if (Q_UNLIKELY(!index.isValid())) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    const auto * pModel = qobject_cast<const LogViewerModel *>(index.model());
    if (Q_UNLIKELY(!pModel)) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    const int row = index.row();
    const auto * pDataEntry = pModel->dataEntry(row);
    if (Q_UNLIKELY(!pDataEntry)) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    const auto column = static_cast<LogViewerModel::Column>(index.column());
    if ((column == LogViewerModel::Column::SourceFileName) ||
        (column == LogViewerModel::Column::Component))
    {
        const auto & field =
            (column == LogViewerModel::Column::Component
                 ? pDataEntry->m_component
                 : pDataEntry->m_sourceFileName);

        int numSubRows = 1;

        const int originalWidth = static_cast<int>(std::floor(
            fontMetricsWidth(fontMetrics, field) * (1.0 + gMargin) + 0.5));

        int width = originalWidth;
        while (width > MAX_SOURCE_FILE_NAME_COLUMN_WIDTH) {
            ++numSubRows;
            width -= MAX_SOURCE_FILE_NAME_COLUMN_WIDTH;
        }

        size.setWidth(
            std::min(originalWidth, MAX_SOURCE_FILE_NAME_COLUMN_WIDTH));

        size.setHeight(static_cast<int>(std::floor(
            fontMetrics.lineSpacing() * (numSubRows + 1 + gMargin) + 0.5)));

        return size;
    }

    int numDisplayedLines = 0;
    const int logEntrySize = pDataEntry->m_logEntry.size();
    int maxLineSize = 0;
    int lineStartPos = -1;
    QString logEntryLineBuffer;
    while (true) {
        int lineEndPos = -1;

        const int index =
            pDataEntry->m_logEntry.indexOf(gNewlineChar, (lineStartPos + 1));

        if (index < 0) {
            lineEndPos = (lineStartPos + gMaxLogEntryLineSize);

            const int previousWhitespaceIndex =
                pDataEntry->m_logEntry.lastIndexOf(
                    gWhitespaceChar, (lineEndPos - 1));

            if (previousWhitespaceIndex > lineStartPos) {
                lineEndPos = previousWhitespaceIndex;
            }
        }
        else {
            lineEndPos = index;

            if (lineEndPos - lineStartPos > gMaxLogEntryLineSize) {
                lineEndPos = lineStartPos + gMaxLogEntryLineSize;

                const int previousWhitespaceIndex =
                    pDataEntry->m_logEntry.lastIndexOf(
                        gWhitespaceChar, (lineEndPos - 1));

                if (previousWhitespaceIndex > lineStartPos) {
                    lineEndPos = previousWhitespaceIndex;
                }
            }
        }

        if (lineEndPos > logEntrySize) {
            lineEndPos = logEntrySize;
        }

        logEntryLineBuffer = pDataEntry->m_logEntry
                                 .mid(lineStartPos, (lineEndPos - lineStartPos))
                                 .trimmed();

        const int lineSize = logEntryLineBuffer.size();
        if (lineSize > maxLineSize) {
            maxLineSize = lineSize;
        }

        ++numDisplayedLines;

        if (lineEndPos == logEntrySize) {
            break;
        }

        lineStartPos = lineEndPos;
    }

    size.setWidth(static_cast<int>(std::floor(
        fontMetricsWidth(fontMetrics, QStringLiteral("w")) *
            (maxLineSize + 2 + gMargin) +
        0.5)));

    size.setHeight(static_cast<int>(std::floor(
        (numDisplayedLines + 1) * fontMetrics.lineSpacing() + gMargin)));

    return size;
}

bool LogViewerDelegate::paintImpl(
    QPainter * pPainter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (Q_UNLIKELY(!pPainter)) {
        return false;
    }

    if (Q_UNLIKELY(!index.isValid())) {
        return false;
    }

    const auto * pModel = qobject_cast<const LogViewerModel *>(index.model());
    if (Q_UNLIKELY(!pModel)) {
        return false;
    }

    const int row = index.row();
    const auto * pDataEntry = pModel->dataEntry(row);
    if (Q_UNLIKELY(!pDataEntry)) {
        return false;
    }

    pPainter->save();

    pPainter->setRenderHints(
        QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        pPainter->fillRect(option.rect, option.palette.highlight());
        pPainter->setPen(option.palette.highlightedText().color());
    }
    else {
        pPainter->fillRect(
            option.rect,
            QBrush(pModel->backgroundColorForLogLevel(pDataEntry->m_logLevel)));

        pPainter->setPen(Qt::black);
    }

    QTextOption textOption{Qt::Alignment(Qt::AlignLeft | Qt::AlignTop)};
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    const auto adjustedRect = option.rect.adjusted(2, 2, -2, -2);

    const auto column = static_cast<LogViewerModel::Column>(index.column());
    switch (column) {
    case LogViewerModel::Column::Timestamp:
    {
        const QDateTime & timestamp = pDataEntry->m_timestamp;
        const QDate date = timestamp.date();
        const QTime time = timestamp.time();

        QString printedTimestamp = date.toString(Qt::DefaultLocaleShortDate);
        printedTimestamp += QStringLiteral(" ");
        printedTimestamp += time.toString(QStringLiteral("HH:mm:ss:zzz"));

        pPainter->drawText(adjustedRect, printedTimestamp, textOption);
    } break;
    case LogViewerModel::Column::SourceFileName:
    case LogViewerModel::Column::Component:
    {
        QTextOption textOption{Qt::Alignment(Qt::AlignLeft | Qt::AlignTop)};
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

        pPainter->drawText(
            adjustedRect,
            (column == LogViewerModel::Column::Component
                 ? pDataEntry->m_component
                 : pDataEntry->m_sourceFileName),
            textOption);
    } break;
    case LogViewerModel::Column::SourceFileLineNumber:
        pPainter->drawText(
            adjustedRect, QString::number(pDataEntry->m_sourceFileLineNumber),
            textOption);
        break;
    case LogViewerModel::Column::LogLevel:
        pPainter->drawText(
            adjustedRect,
            LogViewerModel::logLevelToString(pDataEntry->m_logLevel),
            textOption);
        break;
    case LogViewerModel::Column::LogEntry:
    {
        const QFontMetrics fontMetrics{option.font};
        paintLogEntry(*pPainter, adjustedRect, *pDataEntry, fontMetrics);
    } break;
    default:
        break;
    }

    pPainter->restore();
    return true;
}

void LogViewerDelegate::paintLogEntry(
    QPainter & painter, const QRect & adjustedRect,
    const LogViewerModel::Data & dataEntry,
    const QFontMetrics & fontMetrics) const
{
    if (Q_UNLIKELY(dataEntry.m_logEntry.isEmpty())) {
        return;
    }

    QString logEntryLineBuffer;
    logEntryLineBuffer.reserve(gMaxLogEntryLineSize + 1);

    const int lineSpacing = fontMetrics.height();

    QRect currentRect;
    currentRect.setTopLeft(adjustedRect.topLeft());
    currentRect.setTopRight(adjustedRect.topRight());

    QPoint bottomLeft = adjustedRect.topLeft();
    bottomLeft.setY(currentRect.top() + lineSpacing);

    QPoint bottomRight = adjustedRect.topRight();
    bottomRight.setY(bottomLeft.y());

    currentRect.setBottomLeft(bottomLeft);
    currentRect.setBottomRight(bottomRight);

    QTextOption textOption{Qt::Alignment(Qt::AlignLeft | Qt::AlignTop)};
    textOption.setWrapMode(QTextOption::NoWrap);

    int lineStartPos = -1;
    const int logEntrySize = dataEntry.m_logEntry.size();
    while (true) {
        int lineEndPos = -1;

        const int index =
            dataEntry.m_logEntry.indexOf(gNewlineChar, (lineStartPos + 1));

        if (index < 0) {
            lineEndPos = (lineStartPos + gMaxLogEntryLineSize);

            const int previousWhitespaceIndex =
                dataEntry.m_logEntry.lastIndexOf(
                    gWhitespaceChar, (lineEndPos - 1));

            if (previousWhitespaceIndex > lineStartPos) {
                lineEndPos = previousWhitespaceIndex;
            }
        }
        else {
            lineEndPos = index;

            if (lineEndPos - lineStartPos > gMaxLogEntryLineSize) {
                lineEndPos = lineStartPos + gMaxLogEntryLineSize;

                const int previousWhitespaceIndex =
                    dataEntry.m_logEntry.lastIndexOf(
                        gWhitespaceChar, (lineEndPos - 1));

                if (previousWhitespaceIndex > lineStartPos) {
                    lineEndPos = previousWhitespaceIndex;
                }
            }
        }

        if (lineEndPos > logEntrySize) {
            lineEndPos = logEntrySize;
        }

        logEntryLineBuffer =
            dataEntry.m_logEntry.mid(lineStartPos, (lineEndPos - lineStartPos))
                .trimmed();

        painter.drawText(currentRect, logEntryLineBuffer, textOption);

        if (lineEndPos == logEntrySize) {
            break;
        }

        lineStartPos = lineEndPos;
        currentRect.moveTop(currentRect.top() + lineSpacing);
    }
}

} // namespace quentier
