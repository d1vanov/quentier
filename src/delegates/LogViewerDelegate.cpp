#include "LogViewerDelegate.h"
#include "../models/LogViewerModel.h"

namespace quentier {

LogViewerDelegate::LogViewerDelegate(QObject * parent) :
    AbstractStyledItemDelegate(parent)
{}

QWidget * LogViewerDelegate::createEditor(QWidget * parent,
                                          const QStyleOptionViewItem & option,
                                          const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void LogViewerDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // TODO: implement
    AbstractStyledItemDelegate::paint(painter, option, index);
}

QSize LogViewerDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    const LogViewerModel * pModel = qobject_cast<const LogViewerModel*>(index.model());
    if (Q_UNLIKELY(!pModel)) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    int row = index.row();
    const LogViewerModel::Data * pDataEntry = pModel->dataEntry(row);
    if (Q_UNLIKELY(!pDataEntry)) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    // TODO: continue from here

    return AbstractStyledItemDelegate::sizeHint(option, index);
}

} // namespace quentier
