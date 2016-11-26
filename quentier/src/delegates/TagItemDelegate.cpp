#include "TagItemDelegate.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

#define CIRCLE_RADIUS (2)

using namespace quentier;

TagItemDelegate::TagItemDelegate(QObject * parent) :
    QStyledItemDelegate(parent)
{}

QString TagItemDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    return QStyledItemDelegate::displayText(value, locale);
}

QWidget * TagItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                        const QModelIndex & index) const
{
    // Only allow to edit the tag name but no other tag model columns
    if (index.isValid() && (index.column() == TagModel::Columns::Name)) {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
    else {
        return Q_NULLPTR;
    }
}

void TagItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                            const QModelIndex & index) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (index.isValid() && index.column() == TagModel::Columns::Name) {
        drawTagName(painter, index, option);
    }

    painter->restore();
}

void TagItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == TagModel::Columns::Name)) {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void TagItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                   const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == TagModel::Columns::Name)) {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QSize TagItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return QSize();
    }

    if (index.column() == TagModel::Columns::Name) {
        return tagNameSizeHint(option, index);
    }

    return QStyledItemDelegate::sizeHint(option, index);
}

void TagItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                           const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == TagModel::Columns::Name)) {
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}

void TagItemDelegate::drawTagName(QPainter * painter, const QModelIndex & index,
                                  const QStyleOptionViewItem & option) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        QNDEBUG(QStringLiteral("TagItemDelegate::drawTagName: can't draw, no model"));
        return;
    }

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    QString name = model->data(index).toString();
    if (name.isEmpty()) {
        QNDEBUG(QStringLiteral("TagItemDelegate::drawTagName: tag name is empty"));
        return;
    }

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.highlightedText().color()
                    : option.palette.windowText().color());
    painter->drawText(option.rect, name, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name);

    QModelIndex numNotesPerTagIndex = model->index(index.row(), TagModel::Columns::NumNotesPerTag, index.parent());
    QVariant numNotesPerTag = model->data(numNotesPerTagIndex);
    bool conversionResult = false;
    int numNotesPerTagInt = numNotesPerTag.toInt(&conversionResult);
    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the number of notes per tag to int: ") << numNotesPerTag);
        return;
    }

    if (numNotesPerTagInt <= 0) {
        return;
    }

    QString nameSuffix = QStringLiteral(" (");
    nameSuffix += QString::number(numNotesPerTagInt);
    nameSuffix += QStringLiteral(")");

    painter->setPen(painter->pen().color().lighter());
    painter->drawText(option.rect.translated(nameWidth, 0), nameSuffix, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
}

QSize TagItemDelegate::tagNameSizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QString name = model->data(index).toString();
    if (Q_UNLIKELY(name.isEmpty())) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name);
    int fontHeight = fontMetrics.height();

    QModelIndex numNotesPerTagIndex = model->index(index.row(), TagModel::Columns::NumNotesPerTag, index.parent());
    QVariant numNotesPerTag = model->data(numNotesPerTagIndex);
    bool conversionResult = false;
    int numNotesPerTagInt = numNotesPerTag.toInt(&conversionResult);

    if (!conversionResult) {
        QNDEBUG(QStringLiteral("Failed to convert the number of notes per tag to int: ") << numNotesPerTag);
        return QStyledItemDelegate::sizeHint(option, index);
    }

    if (numNotesPerTagInt <= 0) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    double margin = 1.1;
    return QSize(std::max(static_cast<int>(std::floor(nameWidth * margin + 0.5)), option.rect.width()),
                 std::max(static_cast<int>(std::floor(fontHeight * margin + 0.5)), option.rect.height()));
}
