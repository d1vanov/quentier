#include "NotebookItemDelegate.h"
#include "../models/NotebookModel.h"
#include <QPainter>

#define CIRCLE_RADIUS (2)

using namespace quentier;

NotebookItemDelegate::NotebookItemDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

QString NotebookItemDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    return QStyledItemDelegate::displayText(value, locale);
}

QWidget * NotebookItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                             const QModelIndex & index) const
{
    // Only allow to edit the notebook name but no other notebook model columns
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
    else {
        return Q_NULLPTR;
    }
}

void NotebookItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                                 const QModelIndex & index) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (index.isValid())
    {
        int column = index.column();
        if (column == NotebookModel::Columns::Name)
        {
            QStyledItemDelegate::paint(painter, option, index);
        }
        else if (column == NotebookModel::Columns::Default)
        {
            bool dirty = index.model()->data(index).toBool();
            if (dirty) {
                painter->setBrush(QBrush(Qt::magenta));
                drawEllipse(painter, option);
            }
        }
        else if (column == NotebookModel::Columns::Published)
        {
            bool published = index.model()->data(index).toBool();
            if (published) {
                painter->setBrush(QBrush(Qt::blue));
                drawEllipse(painter, option);
            }
        }
    }

    painter->restore();
}

void NotebookItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    if (index.column() == NotebookModel::Columns::Name) {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void NotebookItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                        const QModelIndex & index) const
{
    if (index.column() == NotebookModel::Columns::Name) {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize NotebookItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return QSize();
    }

    int column = index.column();
    if ((column == NotebookModel::Columns::Default) ||
        (column == NotebookModel::Columns::Published))
    {
        int side = CIRCLE_RADIUS;
        side += 1;
        side *= 2;
        return QSize(side, side);
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

void NotebookItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                                const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}

void NotebookItemDelegate::drawEllipse(QPainter * painter, const QStyleOptionViewItem & option) const
{
    int side = std::min(option.rect.width(), option.rect.height());
    int radius = std::min(side, CIRCLE_RADIUS);
    int diameter = 2 * radius;
    QPoint center = option.rect.center();
    painter->drawEllipse(QRectF(center.x() - radius, center.y() - radius, diameter, diameter));
}
