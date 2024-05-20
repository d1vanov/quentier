/*
 * Copyright 2017-2024 Dmitry Ivanov
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
#include <QLocale>
#include <QPainter>
#include <QTime>

#include <cmath>
#include <limits>

namespace quentier {

namespace {

constexpr int gLogViewerModelMaxLogEntryLineSize = 150;

} // namespace

LogViewerDelegate::LogViewerDelegate(QObject * parent) :
    QStyledItemDelegate{parent},
    m_newlineChar{QChar::fromLatin1('\n')},
    m_whitespaceChar{QChar::fromLatin1(' ')},
    m_margin{0.4},
    m_widestLogLevelName{QStringLiteral("Warning")},
    m_sampleDateTimeString{QStringLiteral("26/09/2017 19:31:23:457")},
    m_sampleSourceFileLineNumberString{QStringLiteral("99999")}
{}

QWidget * LogViewerDelegate::createEditor(
    QWidget * parent, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return nullptr;
}

void LogViewerDelegate::paint(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (paintImpl(painter, option, index)) {
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
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

    const auto stringSizeHint = [&](const QString & str) {
        size.setWidth(static_cast<int>(std::floor(
            fontMetricsWidth(fontMetrics, str) * (1.0 + m_margin) + 0.5)));
        size.setHeight(static_cast<int>(
            std::floor(fontMetrics.lineSpacing() * (1.0 + m_margin) + 0.5)));
        return size;
    };

    switch (static_cast<LogViewerModel::Column>(index.column())) {
    case LogViewerModel::Column::Timestamp:
        return stringSizeHint(m_sampleDateTimeString);
    case LogViewerModel::Column::SourceFileLineNumber:
        return stringSizeHint(m_sampleSourceFileLineNumberString);
    case LogViewerModel::Column::LogLevel:
        return stringSizeHint(m_widestLogLevelName);
    default:
        break;
    }

    // If we haven't returned yet, either the index is invalid or we are dealing
    // with either log entry column or source file name column or component
    // column

    if (Q_UNLIKELY(!index.isValid())) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    const auto * model = qobject_cast<const LogViewerModel *>(index.model());
    if (Q_UNLIKELY(!model)) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    const int row = index.row();
    const LogViewerModel::Data * dataEntry = model->dataEntry(row);
    if (Q_UNLIKELY(!dataEntry)) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    const int column = index.column();
    if (column == static_cast<int>(LogViewerModel::Column::SourceFileName) ||
        column == static_cast<int>(LogViewerModel::Column::Component))
    {
        const auto & field =
            (column == static_cast<int>(LogViewerModel::Column::Component)
                 ? dataEntry->m_component
                 : dataEntry->m_sourceFileName);

        int numSubRows = 1;

        const int originalWidth = static_cast<int>(std::floor(
            fontMetrics.horizontalAdvance(field) * (1.0 + m_margin) + 0.5));

        int width = originalWidth;
        while (width > maxSourceFileNameColumnWidth()) {
            ++numSubRows;
            width -= maxSourceFileNameColumnWidth();
        }

        size.setWidth(
            std::min(originalWidth, maxSourceFileNameColumnWidth()));

        size.setHeight(static_cast<int>(std::floor(
            fontMetrics.lineSpacing() * (numSubRows + 1 + m_margin) + 0.5)));

        return size;
    }

    int numDisplayedLines = 0;
    Q_ASSERT(dataEntry->m_logEntry.size() <= std::numeric_limits<int>::max());
    const int logEntrySize = static_cast<int>(dataEntry->m_logEntry.size());
    int maxLineSize = 0;
    int lineStartPos = -1;
    QString logEntryLineBuffer;
    while (true) {
        int lineEndPos = -1;

        const int index = static_cast<int>(
            dataEntry->m_logEntry.indexOf(m_newlineChar, (lineStartPos + 1)));

        if (index < 0) {
            lineEndPos =
                (lineStartPos + gLogViewerModelMaxLogEntryLineSize);

            const auto previousWhitespaceIndex =
                static_cast<int>(dataEntry->m_logEntry.lastIndexOf(
                    m_whitespaceChar, (lineEndPos - 1)));

            if (previousWhitespaceIndex > lineStartPos) {
                lineEndPos = previousWhitespaceIndex;
            }
        }
        else {
            lineEndPos = index;

            if (lineEndPos - lineStartPos >
                gLogViewerModelMaxLogEntryLineSize) {
                lineEndPos =
                    lineStartPos + gLogViewerModelMaxLogEntryLineSize;

                const auto previousWhitespaceIndex =
                    static_cast<int>(dataEntry->m_logEntry.lastIndexOf(
                        m_whitespaceChar, (lineEndPos - 1)));

                if (previousWhitespaceIndex > lineStartPos) {
                    lineEndPos = previousWhitespaceIndex;
                }
            }
        }

        if (lineEndPos > logEntrySize) {
            lineEndPos = logEntrySize;
        }

        const bool lastIteration = (lineEndPos == logEntrySize);

        logEntryLineBuffer =
            dataEntry->m_logEntry.mid(lineStartPos, (lineEndPos - lineStartPos))
                .trimmed();

        const auto lineSize = static_cast<int>(logEntryLineBuffer.size());
        if (lineSize > maxLineSize) {
            maxLineSize = lineSize;
        }

        ++numDisplayedLines;

        if (lastIteration) {
            break;
        }

        lineStartPos = lineEndPos;
    }

    size.setWidth(static_cast<int>(std::floor(
        fontMetricsWidth(fontMetrics, QStringLiteral("w")) *
            (static_cast<double>(maxLineSize) + 2 + m_margin) +
        0.5)));

    size.setHeight(static_cast<int>(std::floor(
        (static_cast<double>(numDisplayedLines) + 1) *
            fontMetrics.lineSpacing() +
        m_margin)));

    return size;
}

bool LogViewerDelegate::paintImpl(
    QPainter * painter, const QStyleOptionViewItem & option,
    const QModelIndex & index) const
{
    if (Q_UNLIKELY(!painter)) {
        return false;
    }

    if (Q_UNLIKELY(!index.isValid())) {
        return false;
    }

    const auto * model = qobject_cast<const LogViewerModel *>(index.model());
    if (Q_UNLIKELY(!model)) {
        return false;
    }

    const auto row = index.row();
    const auto * dataEntry = model->dataEntry(row);
    if (Q_UNLIKELY(!dataEntry)) {
        return false;
    }

    painter->save();

    painter->setRenderHints(
        QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.highlightedText().color());
    }
    else {
        painter->fillRect(
            option.rect,
            QBrush{model->backgroundColorForLogLevel(dataEntry->m_logLevel)});

        painter->setPen(Qt::black);
    }

    QTextOption textOption{Qt::Alignment{Qt::AlignLeft | Qt::AlignTop}};
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    QRect adjustedRect = option.rect.adjusted(2, 2, -2, -2);

    const auto column = index.column();
    switch (column) {
    case static_cast<int>(LogViewerModel::Column::Timestamp):
    {
        const QDateTime & timestamp = dataEntry->m_timestamp;
        QDate date = timestamp.date();
        QTime time = timestamp.time();

        QString printedTimestamp =
            QLocale{}.toString(date, QLocale::ShortFormat);

        printedTimestamp += QStringLiteral(" ");
        printedTimestamp += time.toString(QStringLiteral("HH:mm:ss:zzz"));
        painter->drawText(adjustedRect, printedTimestamp, textOption);
    } break;
    case static_cast<int>(LogViewerModel::Column::SourceFileName):
    case static_cast<int>(LogViewerModel::Column::Component):
    {
        QTextOption textOption{Qt::Alignment{Qt::AlignLeft | Qt::AlignTop}};
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

        painter->drawText(
            adjustedRect,
            (column == static_cast<int>(LogViewerModel::Column::Component)
                 ? dataEntry->m_component
                 : dataEntry->m_sourceFileName),
            textOption);
    } break;
    case static_cast<int>(LogViewerModel::Column::SourceFileLineNumber):
        painter->drawText(
            adjustedRect, QString::number(dataEntry->m_sourceFileLineNumber),
            textOption);
        break;
    case static_cast<int>(LogViewerModel::Column::LogLevel):
        painter->drawText(
            adjustedRect,
            LogViewerModel::logLevelToString(dataEntry->m_logLevel),
            textOption);
        break;
    case static_cast<int>(LogViewerModel::Column::LogEntry):
    {
        const QFontMetrics fontMetrics{option.font};
        paintLogEntry(*painter, adjustedRect, *dataEntry, fontMetrics);
    } break;
    default:
        break;
    }

    painter->restore();
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
    logEntryLineBuffer.reserve(gLogViewerModelMaxLogEntryLineSize + 1);

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

    Q_ASSERT(dataEntry.m_logEntry.size() <= std::numeric_limits<int>::max());
    const auto logEntrySize = static_cast<int>(dataEntry.m_logEntry.size());
    while (true) {
        int lineEndPos = -1;

        const auto index = static_cast<int>(
            dataEntry.m_logEntry.indexOf(m_newlineChar, (lineStartPos + 1)));

        if (index < 0) {
            lineEndPos =
                (lineStartPos + gLogViewerModelMaxLogEntryLineSize);

            const auto previousWhitespaceIndex =
                static_cast<int>(dataEntry.m_logEntry.lastIndexOf(
                    m_whitespaceChar, (lineEndPos - 1)));

            if (previousWhitespaceIndex > lineStartPos) {
                lineEndPos = previousWhitespaceIndex;
            }
        }
        else {
            lineEndPos = index;

            if (lineEndPos - lineStartPos >
                gLogViewerModelMaxLogEntryLineSize) {
                lineEndPos =
                    lineStartPos + gLogViewerModelMaxLogEntryLineSize;

                const auto previousWhitespaceIndex =
                    static_cast<int>(dataEntry.m_logEntry.lastIndexOf(
                        m_whitespaceChar, (lineEndPos - 1)));

                if (previousWhitespaceIndex > lineStartPos) {
                    lineEndPos = previousWhitespaceIndex;
                }
            }
        }

        if (lineEndPos > logEntrySize) {
            lineEndPos = logEntrySize;
        }

        const bool lastIteration = (lineEndPos == logEntrySize);

        logEntryLineBuffer =
            dataEntry.m_logEntry.mid(lineStartPos, (lineEndPos - lineStartPos))
                .trimmed();

        painter.drawText(currentRect, logEntryLineBuffer, textOption);

        if (lastIteration) {
            break;
        }

        lineStartPos = lineEndPos;
        currentRect.moveTop(currentRect.top() + lineSpacing);
    }
}

} // namespace quentier
