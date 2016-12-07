#include "NotebookItemDelegate.h"
#include "../models/NotebookModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

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

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    if (index.isValid())
    {
        int column = index.column();
        if (column == NotebookModel::Columns::Name)
        {
            drawNotebookName(painter, index, option);
        }
        else if (column == NotebookModel::Columns::Default)
        {
            bool isDefault = index.model()->data(index).toBool();
            if (isDefault) {
                painter->setBrush(QBrush(Qt::green));
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
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void NotebookItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                        const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == NotebookModel::Columns::Name)) {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize NotebookItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return QSize();
    }

    int column = index.column();

    QString columnName;
    const NotebookModel * model = qobject_cast<const NotebookModel*>(index.model());
    if (Q_LIKELY(model && (model->columnCount(index.parent()) > column))) {
        columnName = model->columnName(static_cast<NotebookModel::Columns::type>(column));
    }

    QFontMetrics fontMetrics(option.font);
    double margin = 0.1;

    int columnNameWidth = static_cast<int>(std::floor(fontMetrics.width(columnName) *
                                                      (1.0 + margin) + 0.5));

    if ((column == NotebookModel::Columns::Default) ||
        (column == NotebookModel::Columns::Published))
    {
        int side = CIRCLE_RADIUS;
        side += 1;
        side *= 2;
        int width = std::max(columnNameWidth, side);
        return QSize(width, side);
    }
    else if (column == NotebookModel::Columns::Name)
    {
        return notebookNameSizeHint(option, index, columnNameWidth);
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

void NotebookItemDelegate::drawNotebookName(QPainter * painter, const QModelIndex & index,
                                            const QStyleOptionViewItem & option) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNDEBUG(QStringLiteral("NotebookItemDelegate::drawNotebookName: can't draw, no model"));
        return;
    }

    QString name = model->data(index).toString();
    if (name.isEmpty()) {
        QNDEBUG(QStringLiteral("NotebookItemDelegate::drawNotebookName: notebook name is empty"));
        return;
    }

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.highlightedText().color()
                    : option.palette.windowText().color());
    painter->drawText(option.rect, name, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name);

    QModelIndex numNotesPerNotebookIndex = model->index(index.row(), NotebookModel::Columns::NumNotesPerNotebook, index.parent());
    QVariant numNotesPerNotebook = model->data(numNotesPerNotebookIndex);
    bool conversionResult = false;
    int numNotesPerNotebookInt = numNotesPerNotebook.toInt(&conversionResult);
    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the number of notes per notebook to int: ") << numNotesPerNotebook);
        return;
    }

    if (numNotesPerNotebookInt <= 0) {
        return;
    }

    QString nameSuffix = QStringLiteral(" (");
    nameSuffix += QString::number(numNotesPerNotebookInt);
    nameSuffix += QStringLiteral(")");

    painter->setPen(painter->pen().color().lighter());
    painter->drawText(option.rect.translated(nameWidth, 0), nameSuffix, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

QSize NotebookItemDelegate::notebookNameSizeHint(const QStyleOptionViewItem & option,
                                                 const QModelIndex & index,
                                                 const int columnNameWidth) const
{
    QNTRACE(QStringLiteral("NotebookItemDelegate::notebookNameSizeHint: column name width = ")
            << columnNameWidth);

    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNDEBUG(QStringLiteral("No model, fallback to the default size hint"));
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QString name = model->data(index).toString();
    if (Q_UNLIKELY(name.isEmpty())) {
        QNDEBUG(QStringLiteral("Notebook name is empty, fallback to the default size hint"));
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QModelIndex numNotesPerNotebookIndex =
            model->index(index.row(), NotebookModel::Columns::NumNotesPerNotebook, index.parent());
    QVariant numNotesPerNotebook = model->data(numNotesPerNotebookIndex);
    bool conversionResult = false;
    int numNotesPerNotebookInt = numNotesPerNotebook.toInt(&conversionResult);

    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the number of notes per notebook to int: ")
                << numNotesPerNotebook);
    }
    else if (numNotesPerNotebookInt > 0) {
        QNTRACE(QStringLiteral("Appending num notes per notebook to the notebook name: ")
                << numNotesPerNotebookInt);
        name += QStringLiteral(" (");
        name += QString::number(numNotesPerNotebookInt);
        name += QStringLiteral(")");
    }

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name);
    int fontHeight = fontMetrics.height();

    double margin = 0.1;
    int width = std::max(static_cast<int>(std::floor(nameWidth * (1.0 + margin) + 0.5)),
                         option.rect.width());
    if (width < columnNameWidth) {
        width = columnNameWidth;
    }

    int height = std::max(static_cast<int>(std::floor(fontHeight * (1.0 + margin) + 0.5)),
                          option.rect.height());

    QSize size = QSize(width, height);

    QNTRACE(QStringLiteral("Computed size: width = ") << width
            << QStringLiteral(", height = ") << height);
    return size;
}
