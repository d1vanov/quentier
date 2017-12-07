#include "DeletedNoteItemDelegate.h"
#include "../models/NoteModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>
#include <QDateTime>
#include <QTextOption>
#include <cmath>

#define TEXT_HEIGHT_MARGIN (4)
#define TEXT_WIDTH_MARGIN (8)

namespace quentier {

DeletedNoteItemDelegate::DeletedNoteItemDelegate(QObject * parent) :
    AbstractStyledItemDelegate(parent),
    m_deletionDateTimeReplacementText()
{
    m_deletionDateTimeReplacementText = QStringLiteral("(") + tr("No deletion datetime") + QStringLiteral(")");
}

QWidget * DeletedNoteItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                                const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void DeletedNoteItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                                    const QModelIndex & index) const
{
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    doPaint(painter, option, index);

    painter->restore();
}

void DeletedNoteItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void DeletedNoteItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize DeletedNoteItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QSize size = doSizeHint(option, index);
    if (size.isValid()) {
        return size;
    }

    // Fallback to the base class' implementation
    return AbstractStyledItemDelegate::sizeHint(option, index);
}

void DeletedNoteItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

void DeletedNoteItemDelegate::doPaint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (!index.isValid()) {
        return;
    }

    const QAbstractItemModel * pModel = index.model();
    if (Q_UNLIKELY(!pModel)) {
        QNDEBUG(QStringLiteral("DeletedNoteItemDelegate::doPaint: can't paint, no model"));
        return;
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pModel);
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("DeletedNoteItemDelegate::doPaint: can't paint, can't cast the model to NoteModel"));
        return;
    }

    const NoteModelItem * pNoteItem = pNoteModel->itemForIndex(index);
    if (Q_UNLIKELY(!pNoteItem)) {
        QNDEBUG(QStringLiteral("DeletedNoteItemDelegate::doPaint: can't paint, no note item for index: row = ")
                << index.row() << QStringLiteral(", column = ") << index.column());
        return;
    }

    int column = index.column();

    if (column == NoteModel::Columns::Title) {
        drawDeletedNoteTitleOrPreviewText(painter, option, *pNoteItem);
    }
    else if (column == NoteModel::Columns::DeletionTimestamp) {
        drawDeletionDateTime(painter, option, *pNoteItem);
    }
}

void DeletedNoteItemDelegate::drawDeletedNoteTitleOrPreviewText(QPainter * painter, const QStyleOptionViewItem & option,
                                                                const NoteModelItem & item) const
{
    QString text = item.title();
    if (text.isEmpty()) {
        text = item.previewText();
    }

    text = text.simplified();

    if (text.isEmpty()) {
        painter->setPen(option.state & QStyle::State_Selected
                        ? option.palette.color(QPalette::Active, QPalette::WindowText)
                        : option.palette.color(QPalette::Active, QPalette::Highlight));
        text = QStringLiteral("(") + tr("Note without title or content") + QStringLiteral(")");
    }
    else {
        painter->setPen(option.state & QStyle::State_Selected
                        ? option.palette.highlightedText().color()
                        : option.palette.windowText().color());
    }

    adjustDisplayedText(text, option);
    painter->drawText(option.rect, text, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
}

void DeletedNoteItemDelegate::drawDeletionDateTime(QPainter * painter, const QStyleOptionViewItem & option,
                                                   const NoteModelItem & item) const
{
    qint64 deletionTimestamp = item.deletionTimestamp();

    QString text;
    if (deletionTimestamp == static_cast<qint64>(0))
    {
        painter->setPen(option.state & QStyle::State_Selected
                        ? option.palette.color(QPalette::Active, QPalette::WindowText)
                        : option.palette.color(QPalette::Active, QPalette::Highlight));
        text = m_deletionDateTimeReplacementText;
    }
    else
    {
        painter->setPen(option.state & QStyle::State_Selected
                        ? option.palette.highlightedText().color()
                        : option.palette.windowText().color());
        text = QDateTime::fromMSecsSinceEpoch(deletionTimestamp).toString(Qt::DefaultLocaleShortDate);

        text.prepend(QStringLiteral(" "));

        adjustDisplayedText(text, option);
    }

    painter->drawText(option.rect, text, QTextOption(Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter)));
}

QSize DeletedNoteItemDelegate::doSizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    if (!index.isValid()) {
        return QSize();
    }

    const QAbstractItemModel * pModel = index.model();
    if (Q_UNLIKELY(!pModel)) {
        QNDEBUG(QStringLiteral("DeletedNoteItemDelegate::doSizeHint: can't compute size hint, no model"));
        return QSize();
    }

    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(pModel);
    if (Q_UNLIKELY(!pNoteModel)) {
        QNDEBUG(QStringLiteral("DeletedNoteItemDelegate::doSizeHint: can't compute size hint, can't cast the model to NoteModel"));
        return QSize();
    }

    const NoteModelItem * pNoteItem = pNoteModel->itemForIndex(index);
    if (Q_UNLIKELY(!pNoteItem)) {
        QNDEBUG(QStringLiteral("DeletedNoteItemDelegate::doSizeHint: can't compute size hint, no note item for index: row = ")
                << index.row() << QStringLiteral(", column = ") << index.column());
        return QSize();
    }

    int column = index.column();

    QFontMetrics fontMetrics(option.font);
    int height = fontMetrics.height() + TEXT_HEIGHT_MARGIN;

    if (column == NoteModel::Columns::Title)
    {
        QString text = pNoteItem->title();
        if (text.isEmpty()) {
            text = pNoteItem->previewText();
        }

        text = text.simplified();

        int width = fontMetrics.width(text) + TEXT_WIDTH_MARGIN;
        return QSize(width, height);
    }
    else if (column == NoteModel::Columns::DeletionTimestamp)
    {
        QString text;

        qint64 deletionTimestamp = pNoteItem->deletionTimestamp();
        if (deletionTimestamp == static_cast<qint64>(0)) {
            text = m_deletionDateTimeReplacementText;
        }
        else {
            text = QDateTime::fromMSecsSinceEpoch(deletionTimestamp).toString(Qt::DefaultLocaleShortDate);
        }

        text.prepend(QStringLiteral(" "));

        int width = fontMetrics.width(text) + TEXT_WIDTH_MARGIN;
        return QSize(width, height);
    }

    return QSize();
}

} // namespace quentier
