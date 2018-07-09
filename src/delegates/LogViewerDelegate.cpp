/*
 * Copyright 2017 Dmitry Ivanov
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
#include <QFontMetrics>
#include <QPainter>
#include <QStringRef>
#include <cmath>

#define LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE (150)

namespace quentier {

LogViewerDelegate::LogViewerDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_margin(0.4),
    m_widestLogLevelName(QStringLiteral("Warning")),
    m_sampleDateTimeString(QStringLiteral("26/09/2017 19:31:23:457")),
    m_sampleSourceFileLineNumberString(QStringLiteral("99999")),
    m_newlineChar(QChar::fromLatin1('\n')),
    m_whitespaceChar(QChar::fromLatin1(' '))
{}

QWidget * LogViewerDelegate::createEditor(QWidget * pParent,
                                          const QStyleOptionViewItem & option,
                                          const QModelIndex & index) const
{
    Q_UNUSED(pParent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void LogViewerDelegate::paint(QPainter * pPainter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (paintImpl(pPainter, option, index)) {
        return;
    }

    QStyledItemDelegate::paint(pPainter, option, index);
}

QSize LogViewerDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // NOTE: this method is called many times when QHeaderView::resizeSections(QHeaderView::ResizeToContents)
    // it has to be very fast, otherwise the performance is complete crap
    // so there are some shortcuts and missing checks which should normally be here

    QFontMetrics fontMetrics(option.font);
    QSize size;

#define STRING_SIZE_HINT(str) \
    { \
        size.setWidth(static_cast<int>(std::floor(fontMetrics.width(str) * (1.0 + m_margin) + 0.5))); \
        size.setHeight(static_cast<int>(std::floor(fontMetrics.lineSpacing() * (1.0 + m_margin) + 0.5))); \
        return size; \
    }

    switch(index.column())
    {
    case LogViewerModel::Columns::Timestamp:
        STRING_SIZE_HINT(m_sampleDateTimeString)
    case LogViewerModel::Columns::SourceFileLineNumber:
        STRING_SIZE_HINT(m_sampleSourceFileLineNumberString)
    case LogViewerModel::Columns::LogLevel:
        STRING_SIZE_HINT(m_widestLogLevelName)
    }

#undef STRING_SIZE_HINT

    // If we haven't returned yet, either the index is invalid or we are dealing
    // with either log entry column or source file name column

    if (Q_UNLIKELY(!index.isValid())) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    const LogViewerModel * pModel = qobject_cast<const LogViewerModel*>(index.model());
    if (Q_UNLIKELY(!pModel)) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    int row = index.row();
    const LogViewerModel::Data * pDataEntry = pModel->dataEntry(row);
    if (Q_UNLIKELY(!pDataEntry)) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    if (index.column() == LogViewerModel::Columns::SourceFileName)
    {
        int numSubRows = 1;
        int originalWidth = static_cast<int>(std::floor(fontMetrics.width(pDataEntry->m_sourceFileName) * (1.0 + m_margin) + 0.5));
        int width = originalWidth;
        while(width > MAX_SOURCE_FILE_NAME_COLUMN_WIDTH) {
            ++numSubRows;
            width -= MAX_SOURCE_FILE_NAME_COLUMN_WIDTH;
        }

        size.setWidth(std::min(originalWidth, MAX_SOURCE_FILE_NAME_COLUMN_WIDTH));
        size.setHeight(static_cast<int>(std::floor(fontMetrics.lineSpacing() * (numSubRows + 1 + m_margin) + 0.5)));
        return size;
    }

    int numDisplayedLines = 0;
    const int logEntrySize = pDataEntry->m_logEntry.size();
    int maxLineSize = 0;
    int lineStartPos = -1;
    QString logEntryLineBuffer;
    while(true)
    {
        int lineEndPos = -1;
        int index = pDataEntry->m_logEntry.indexOf(m_newlineChar, (lineStartPos + 1));
        if (index < 0)
        {
            lineEndPos = (lineStartPos + LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE);

            int previousWhitespaceIndex = pDataEntry->m_logEntry.lastIndexOf(m_whitespaceChar, (lineEndPos - 1));
            if (previousWhitespaceIndex > lineStartPos) {
                lineEndPos = previousWhitespaceIndex;
            }
        }
        else
        {
            lineEndPos = index;

            if (lineEndPos - lineStartPos > LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE)
            {
                lineEndPos = lineStartPos + LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE;

                int previousWhitespaceIndex = pDataEntry->m_logEntry.lastIndexOf(m_whitespaceChar, (lineEndPos - 1));
                if (previousWhitespaceIndex > lineStartPos) {
                    lineEndPos = previousWhitespaceIndex;
                }
            }
        }

        if (lineEndPos > logEntrySize) {
            lineEndPos = logEntrySize;
        }

        bool lastIteration = (lineEndPos == logEntrySize);

        logEntryLineBuffer = pDataEntry->m_logEntry.mid(lineStartPos, (lineEndPos - lineStartPos)).trimmed();
        int lineSize = logEntryLineBuffer.size();
        if (lineSize > maxLineSize) {
            maxLineSize = lineSize;
        }

        ++numDisplayedLines;

        if (lastIteration) {
            break;
        }

        lineStartPos = lineEndPos;
    }

    size.setWidth(static_cast<int>(std::floor(fontMetrics.width(QStringLiteral("w")) * (maxLineSize + 2 + m_margin) + 0.5)));
    size.setHeight(static_cast<int>(std::floor((numDisplayedLines + 1) * fontMetrics.lineSpacing() + m_margin)));
    return size;
}

bool LogViewerDelegate::paintImpl(QPainter * pPainter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!pPainter)) {
        return false;
    }

    if (Q_UNLIKELY(!index.isValid())) {
        return false;
    }

    const LogViewerModel * pModel = qobject_cast<const LogViewerModel*>(index.model());
    if (Q_UNLIKELY(!pModel)) {
        return false;
    }

    int row = index.row();
    const LogViewerModel::Data * pDataEntry = pModel->dataEntry(row);
    if (Q_UNLIKELY(!pDataEntry)) {
        return false;
    }

    pPainter->save();
    pPainter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        pPainter->fillRect(option.rect, option.palette.highlight());
        pPainter->setPen(option.palette.highlightedText().color());
    }
    else {
        pPainter->fillRect(option.rect, QBrush(pModel->backgroundColorForLogLevel(pDataEntry->m_logLevel)));
        pPainter->setPen(Qt::black);
    }

    QTextOption textOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop));
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    QRect adjustedRect = option.rect.adjusted(2, 2, -2, -2);

    int column = index.column();
    switch(column)
    {
    case LogViewerModel::Columns::Timestamp:
        {
            const QDateTime & timestamp = pDataEntry->m_timestamp;
            QDate date = timestamp.date();
            QTime time = timestamp.time();
            QString printedTimestamp = date.toString(Qt::DefaultLocaleShortDate);
            printedTimestamp += QStringLiteral(" ");
            printedTimestamp += time.toString(QStringLiteral("HH:mm:ss:zzz"));
            pPainter->drawText(adjustedRect, printedTimestamp, textOption);
        }
        break;
    case LogViewerModel::Columns::SourceFileName:
        {
            QTextOption textOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop));
            textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            pPainter->drawText(adjustedRect, pDataEntry->m_sourceFileName, textOption);
        }
        break;
    case LogViewerModel::Columns::SourceFileLineNumber:
        pPainter->drawText(adjustedRect, QString::number(pDataEntry->m_sourceFileLineNumber),
                           textOption);
        break;
    case LogViewerModel::Columns::LogLevel:
        pPainter->drawText(adjustedRect, LogViewerModel::logLevelToString(pDataEntry->m_logLevel),
                           textOption);
        break;
    case LogViewerModel::Columns::LogEntry:
        // pPainter->drawText(adjustedRect, pDataEntry->m_logEntry, textOption);
        {
            QFontMetrics fontMetrics(option.font);
            paintLogEntry(*pPainter, adjustedRect, *pDataEntry, fontMetrics);
        }
        break;
    default:
        break;
    }

    pPainter->restore();
    return true;
}

void LogViewerDelegate::paintLogEntry(QPainter & painter, const QRect & adjustedRect,
                                      const LogViewerModel::Data & dataEntry, const QFontMetrics & fontMetrics) const
{
    if (Q_UNLIKELY(dataEntry.m_logEntry.isEmpty())) {
        return;
    }

    QString logEntryLineBuffer;
    logEntryLineBuffer.reserve(LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE + 1);

    int lineSpacing = fontMetrics.height();

    QRect currentRect;
    currentRect.setTopLeft(adjustedRect.topLeft());
    currentRect.setTopRight(adjustedRect.topRight());
    QPoint bottomLeft = adjustedRect.topLeft();
    bottomLeft.setY(currentRect.top() + lineSpacing);
    QPoint bottomRight = adjustedRect.topRight();
    bottomRight.setY(bottomLeft.y());
    currentRect.setBottomLeft(bottomLeft);
    currentRect.setBottomRight(bottomRight);

    QTextOption textOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop));
    textOption.setWrapMode(QTextOption::NoWrap);

    int lineStartPos = -1;
    const int logEntrySize = dataEntry.m_logEntry.size();
    while(true)
    {
        int lineEndPos = -1;
        int index = dataEntry.m_logEntry.indexOf(m_newlineChar, (lineStartPos + 1));
        if (index < 0)
        {
            lineEndPos = (lineStartPos + LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE);

            int previousWhitespaceIndex = dataEntry.m_logEntry.lastIndexOf(m_whitespaceChar, (lineEndPos - 1));
            if (previousWhitespaceIndex > lineStartPos) {
                lineEndPos = previousWhitespaceIndex;
            }
        }
        else
        {
            lineEndPos = index;

            if (lineEndPos - lineStartPos > LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE)
            {
                lineEndPos = lineStartPos + LOG_VIEWER_MODEL_MAX_LOG_ENTRY_LINE_SIZE;

                int previousWhitespaceIndex = dataEntry.m_logEntry.lastIndexOf(m_whitespaceChar, (lineEndPos - 1));
                if (previousWhitespaceIndex > lineStartPos) {
                    lineEndPos = previousWhitespaceIndex;
                }
            }
        }

        if (lineEndPos > logEntrySize) {
            lineEndPos = logEntrySize;
        }

        bool lastIteration = (lineEndPos == logEntrySize);

        logEntryLineBuffer = dataEntry.m_logEntry.mid(lineStartPos, (lineEndPos - lineStartPos)).trimmed();
        painter.drawText(currentRect, logEntryLineBuffer, textOption);

        if (lastIteration) {
            break;
        }

        lineStartPos = lineEndPos;
        currentRect.moveTop(currentRect.top() + lineSpacing);
    }
}

} // namespace quentier
