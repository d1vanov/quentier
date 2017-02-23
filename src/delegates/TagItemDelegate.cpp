#include "TagItemDelegate.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

namespace quentier {

TagItemDelegate::TagItemDelegate(QObject * parent) :
    AbstractStyledItemDelegate(parent)
{}

QString TagItemDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    return AbstractStyledItemDelegate::displayText(value, locale);
}

QWidget * TagItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                        const QModelIndex & index) const
{
    // Only allow to edit the tag name but no other tag model columns
    if (index.isValid() && (index.column() == TagModel::Columns::Name)) {
        return AbstractStyledItemDelegate::createEditor(parent, option, index);
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
        AbstractStyledItemDelegate::setEditorData(editor, index);
    }
}

void TagItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                   const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == TagModel::Columns::Name)) {
        AbstractStyledItemDelegate::setModelData(editor, model, index);
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

    return AbstractStyledItemDelegate::sizeHint(option, index);
}

void TagItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                           const QModelIndex & index) const
{
    if (index.isValid() && (index.column() == TagModel::Columns::Name)) {
        AbstractStyledItemDelegate::updateEditorGeometry(editor, option, index);
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

    QString name = model->data(index).toString().simplified();
    if (name.isEmpty()) {
        QNDEBUG(QStringLiteral("TagItemDelegate::drawTagName: tag name is empty"));
        return;
    }

    QString nameSuffix;

    QModelIndex numNotesPerTagIndex = model->index(index.row(), TagModel::Columns::NumNotesPerTag, index.parent());
    QVariant numNotesPerTag = model->data(numNotesPerTagIndex);
    bool conversionResult = false;
    int numNotesPerTagInt = numNotesPerTag.toInt(&conversionResult);
    if (conversionResult && (numNotesPerTagInt > 0)) {
        nameSuffix = QStringLiteral(" (");
        nameSuffix += QString::number(numNotesPerTagInt);
        nameSuffix += QStringLiteral(")");
    }

    adjustDisplayedText(name, option, nameSuffix);

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.highlightedText().color()
                    : option.palette.windowText().color());
    painter->drawText(option.rect, name, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));

    if (nameSuffix.isEmpty()) {
        return;
    }

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name);

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.color(QPalette::Active, QPalette::WindowText)
                    : option.palette.color(QPalette::Active, QPalette::Highlight));
    painter->drawText(option.rect.translated(nameWidth, 0), nameSuffix, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
}

QSize TagItemDelegate::tagNameSizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    QString name = model->data(index).toString().simplified();
    if (Q_UNLIKELY(name.isEmpty())) {
        return AbstractStyledItemDelegate::sizeHint(option, index);
    }

    QString nameSuffix;

    QModelIndex numNotesPerTagIndex = model->index(index.row(), TagModel::Columns::NumNotesPerTag, index.parent());
    QVariant numNotesPerTag = model->data(numNotesPerTagIndex);
    bool conversionResult = false;
    int numNotesPerTagInt = numNotesPerTag.toInt(&conversionResult);
    if (conversionResult && (numNotesPerTagInt > 0)) {
        nameSuffix = QStringLiteral(" (");
        nameSuffix += QString::number(numNotesPerTagInt);
        nameSuffix += QStringLiteral(")");
    }

    QFontMetrics fontMetrics(option.font);
    int nameWidth = fontMetrics.width(name + nameSuffix);
    int fontHeight = fontMetrics.height();

    double margin = 1.1;
    return QSize(std::max(static_cast<int>(std::floor(nameWidth * margin + 0.5)), option.rect.width()),
                 std::max(static_cast<int>(std::floor(fontHeight * margin + 0.5)), option.rect.height()));
}

} // namespace quentier
