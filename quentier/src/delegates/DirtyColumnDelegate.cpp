#include "DirtyColumnDelegate.h"
#include <QPainter>

DirtyColumnDelegate::DirtyColumnDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

QString DirtyColumnDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)
    return QString();
}

QWidget * DirtyColumnDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                            const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void DirtyColumnDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                                const QModelIndex & index) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return;
    }

    bool dirty = model->data(index).toBool();
    if (!dirty) {
        // Don't draw anything on non-dirty items
        return;
    }

    painter->save();

    painter->setRenderHints(QPainter::Antialiasing);
    painter->setBrush(QBrush(Qt::red));

    int side = std::min(option.rect.width(), option.rect.height());
    int radius = std::min(side, 3);
    int diameter = 2 * radius;
    QPoint center = option.rect.center();
    painter->drawEllipse(QRectF(center.x() - radius, center.y() - radius, diameter, diameter));

    painter->restore();
}

void DirtyColumnDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void DirtyColumnDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                       const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize DirtyColumnDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return QSize();
    }

    return option.rect.size();
}

void DirtyColumnDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                               const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}
