#include "AbstractStyledItemDelegate.h"
#include <quentier/logging/QuentierLogger.h>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

namespace quentier {

AbstractStyledItemDelegate::AbstractStyledItemDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

AbstractStyledItemDelegate::~AbstractStyledItemDelegate()
{}

int AbstractStyledItemDelegate::columnNameWidth(const QStyleOptionViewItem & option, const QModelIndex & index,
                                                const Qt::Orientation orientation) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNTRACE(QStringLiteral("Can't determine the column name width: the model is null"));
        return -1;
    }

    if (Q_UNLIKELY(!index.isValid())) {
        QNTRACE(QStringLiteral("Can't determine the column name width for invalid model index"));
        return -1;
    }

    int column = index.column();
    if (Q_UNLIKELY(model->columnCount(index.parent()) <= column)) {
        QNTRACE(QStringLiteral("Can't determine the column name width: index's column number is beyond the range of model's columns"));
        return -1;
    }

    QString columnName = model->headerData(column, orientation, Qt::DisplayRole).toString();
    if (Q_UNLIKELY(columnName.isEmpty())) {
        QNTRACE(QStringLiteral("Can't determine the column name width: model returned empty header data"));
        return -1;
    }

    QFontMetrics fontMetrics(option.font);
    double margin = 0.1;

    int columnNameWidth = static_cast<int>(std::floor(fontMetrics.width(columnName) * (1.0 + margin) + 0.5));
    return columnNameWidth;
}

void AbstractStyledItemDelegate::adjustDisplayedText(QString & displayedText, const QStyleOptionViewItem & option,
                                                     const QString & nameSuffix) const
{
    QFontMetrics fontMetrics(option.font);

    int displayedTextWidth = fontMetrics.width(displayedText);

    int nameSuffixWidth = (nameSuffix.isEmpty() ? 0 : fontMetrics.width(nameSuffix));
    int optionRectWidth = option.rect.width();
    optionRectWidth -= 2;   // Shorten the available width a tiny bit to ensure there's some margin and there're no weird rendering glitches
    optionRectWidth = std::max(optionRectWidth, 0);

    if ((displayedTextWidth + nameSuffixWidth) <= optionRectWidth) {
        return;
    }

    int idealDisplayedTextWidth = optionRectWidth - nameSuffixWidth;
    displayedText = fontMetrics.elidedText(displayedText, Qt::ElideRight, idealDisplayedTextWidth);
}

} // namespace quentier
