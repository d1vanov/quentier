#include "DeletedNoteTitleColumnDelegate.h"
#include "../models/NoteModel.h"
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>
#include <cmath>

using namespace quentier;

DeletedNoteTitleColumnDelegate::DeletedNoteTitleColumnDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_titleReplacementText(QStringLiteral("(") + tr("No title") + QStringLiteral(")"))
{}

QString DeletedNoteTitleColumnDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
    return QStyledItemDelegate::displayText(value, locale);
}

QWidget * DeletedNoteTitleColumnDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                                       const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void DeletedNoteTitleColumnDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                                           const QModelIndex & index) const
{
    if (Q_UNLIKELY(!index.isValid())) {
        return;
    }

    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return;
    }

    if (Q_UNLIKELY(index.column() != NoteModel::Columns::Title)) {
        return;
    }

    QString title = model->data(index).toString();
    if (!title.isEmpty()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QModelIndex previewTextIndex = model->index(index.row(), NoteModel::Columns::PreviewText, index.parent());
    QString previewText = model->data(previewTextIndex).toString();
    if (!previewText.isEmpty()) {
        QStyledItemDelegate::paint(painter, option, previewTextIndex);
        return;
    }

    // Ok, will write the replacement text ourselves
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    painter->setPen(option.state & QStyle::State_Selected
                    ? option.palette.highlightedText().color()
                    : option.palette.windowText().color());
    painter->drawText(option.rect, m_titleReplacementText, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));
    painter->restore();

    return;
}

void DeletedNoteTitleColumnDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
    return;
}

void DeletedNoteTitleColumnDelegate::setModelData(QWidget * editor, QAbstractItemModel * model,
                                                  const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize DeletedNoteTitleColumnDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    const QAbstractItemModel * model = index.model();
    if (Q_UNLIKELY(!model)) {
        return QSize();
    }

    QString title = model->data(index).toString();
    if (!title.isEmpty()) {
        return QStyledItemDelegate::sizeHint(option, index);
    }

    QModelIndex previewTextIndex = model->index(index.row(), NoteModel::Columns::PreviewText, index.parent());
    QString previewText = model->data(previewTextIndex).toString();
    if (!previewText.isEmpty()) {
        return QStyledItemDelegate::sizeHint(option, previewTextIndex);
    }

    QFontMetrics fontMetrics(option.font);
    int replacementTextWidth = fontMetrics.width(m_titleReplacementText);
    return QSize(replacementTextWidth, option.rect.height());
}

void DeletedNoteTitleColumnDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option,
                                                          const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}
