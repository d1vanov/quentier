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

} // namespace quentier
