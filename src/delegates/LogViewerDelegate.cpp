#include "LogViewerDelegate.h"
#include "../models/LogViewerModel.h"
#include "../models/LogViewerFilterModel.h"
#include <QFontMetrics>
#include <QPainter>
#include <cmath>

namespace quentier {

LogViewerDelegate::LogViewerDelegate(QObject * parent) :
    AbstractStyledItemDelegate(parent),
    m_margin(0.1)
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

    AbstractStyledItemDelegate::paint(pPainter, option, index);
}

QSize LogViewerDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    const LogViewerFilterModel * pFilterModel = qobject_cast<const LogViewerFilterModel*>(index.model());
    if (Q_UNLIKELY(!pFilterModel)) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    const LogViewerModel * pModel = qobject_cast<const LogViewerModel*>(pFilterModel->sourceModel());
    if (Q_UNLIKELY(!pModel)) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    int row = index.row();
    const LogViewerModel::Data * pDataEntry = pModel->dataEntry(row);
    if (Q_UNLIKELY(!pDataEntry)) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    QSize size;

    int column = index.column();
    switch(column)
    {
    case LogViewerModel::Columns::Timestamp:
        {
            const QDateTime & timestamp = pDataEntry->m_timestamp;
            QString printedTimestamp = timestamp.toString(Qt::DefaultLocaleShortDate);
            size = stringSizeHint(option, index, printedTimestamp);
        }
        break;
    case LogViewerModel::Columns::SourceFileName:
        size = stringSizeHint(option, index, pDataEntry->m_sourceFileName);
        break;
    case LogViewerModel::Columns::SourceFileLineNumber:
        size = stringSizeHint(option, index, QString::number(pDataEntry->m_sourceFileLineNumber));
        break;
    case LogViewerModel::Columns::LogLevel:
        size = stringSizeHint(option, index, LogViewerModel::logLevelToString(pDataEntry->m_logLevel));
        break;
    case LogViewerModel::Columns::LogEntry:
        {
            QFontMetrics fontMetrics(option.font);
            int width = static_cast<int>(std::floor(fontMetrics.width(QStringLiteral("w")) *
                                                    (pDataEntry->m_logEntryMaxNumCharsPerLine + m_margin) + 0.5));
            int height = static_cast<int>(std::floor(fontMetrics.height() *
                                                     (pDataEntry->m_numLogEntryLines + 1 + m_margin) + 0.5));
            size.setWidth(width);
            size.setHeight(height);
        }
        break;
    default:
        size = AbstractStyledItemDelegate::sizeHint(option, index);
    }

    if (Q_UNLIKELY(!size.isValid())) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    return size;
}

QSize LogViewerDelegate::stringSizeHint(const QStyleOptionViewItem & option, const QModelIndex & index, const QString & content) const
{
    QSize result;

    QFontMetrics fontMetrics(option.font);
    int contentWidth = static_cast<int>(std::floor(fontMetrics.width(content) * (1.0 + m_margin) + 0.5));
    int headerWidth = columnNameWidth(option, index);
    result.setWidth(std::max(contentWidth, headerWidth));
    result.setHeight(static_cast<int>(std::floor(fontMetrics.height() * (1.0 + m_margin) + 0.5)));

    return result;
}

bool LogViewerDelegate::paintImpl(QPainter * pPainter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!pPainter)) {
        return false;
    }

    if (Q_UNLIKELY(!index.isValid())) {
        return false;
    }

    const LogViewerFilterModel * pFilterModel = qobject_cast<const LogViewerFilterModel*>(index.model());
    if (Q_UNLIKELY(!pFilterModel)) {
        return false;
    }

    const LogViewerModel * pModel = qobject_cast<const LogViewerModel*>(pFilterModel->sourceModel());
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
        // TODO: fill background with color specific to the log level
        pPainter->setPen(option.palette.windowText().color());
    }

    int column = index.column();
    switch(column)
    {
    case LogViewerModel::Columns::Timestamp:
        {
            const QDateTime & timestamp = pDataEntry->m_timestamp;
            QString printedTimestamp = timestamp.toString(Qt::DefaultLocaleShortDate);
            pPainter->drawText(option.rect, printedTimestamp, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop)));
        }
        break;
    case LogViewerModel::Columns::SourceFileName:
        pPainter->drawText(option.rect, pDataEntry->m_sourceFileName, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop)));
        break;
    case LogViewerModel::Columns::SourceFileLineNumber:
        pPainter->drawText(option.rect, QString::number(pDataEntry->m_sourceFileLineNumber),
                           QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop)));
        break;
    case LogViewerModel::Columns::LogLevel:
        pPainter->drawText(option.rect, LogViewerModel::logLevelToString(pDataEntry->m_logLevel),
                           QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignTop)));
        break;
    case LogViewerModel::Columns::LogEntry:
        {
            QTextOption textOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter));
            textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            pPainter->drawText(option.rect, pDataEntry->m_logEntry, textOption);
        }
        break;
    default:
        break;
    }

    pPainter->restore();
    return true;
}

} // namespace quentier
