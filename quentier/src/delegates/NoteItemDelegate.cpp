/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteItemDelegate.h"
#include "../models/NoteModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QPainter>
#include <QDateTime>
#include <cmath>

#define MSEC_PER_DAY (864e5)
#define MSEC_PER_WEEK (6048e5)

namespace quentier {

NoteItemDelegate::NoteItemDelegate(QObject * parent) :
    QStyledItemDelegate(parent),
    m_itemRect(0, 0, 350, 120),
    m_leftMargin(15),
    m_rightMargin(15),
    m_topMargin(8),
    m_bottomMargin(12)
{}

QWidget * NoteItemDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option,
                                         const QModelIndex & index) const
{
    Q_UNUSED(parent)
    Q_UNUSED(option)
    Q_UNUSED(index)
    return Q_NULLPTR;
}

void NoteItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option,
                             const QModelIndex & index) const
{
    const NoteModel * pNoteModel = qobject_cast<const NoteModel*>(index.model());
    if (Q_UNLIKELY(!pNoteModel)) {
        QNLocalizedString error = QT_TR_NOOP("wrong model connected to the note item delegate");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    const NoteModelItem * pItem = pNoteModel->itemForIndex(index);
    if (Q_UNLIKELY(!pItem)) {
        QNLocalizedString error = QT_TR_NOOP("can't retrieve item to paint for note model index");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    painter->save();

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    painter->fillRect(m_itemRect, option.palette.background());
    painter->setPen(option.palette.windowText().color());

    if (option.state & QStyle::State_Selected) {
        QPen originalPen = painter->pen();
        QPen pen = originalPen;
        pen.setWidth(1);
        pen.setColor(option.palette.highlightedText().color());
        painter->setPen(pen);
        painter->drawRoundedRect(QRectF(m_itemRect.adjusted(1, 1, -1, -1)), 2, 2);
        painter->setPen(originalPen);
    }

    QFont boldFont = painter->font();
    boldFont.setBold(true);
    QFontMetrics boldFontMetrics(boldFont);

    QFont smallerFont = painter->font();
    smallerFont.setPointSize(smallerFont.pointSize() - 1);
    QFontMetrics smallerFontMetrics(smallerFont);

    const QImage & thumbnail = pItem->thumbnail();

    int left = m_itemRect.left() + m_leftMargin;
    int width = m_itemRect.width() - m_leftMargin - m_rightMargin;
    if (!thumbnail.isNull()) {
        width -= 120;
    }

    // 1) Painting the title (or a piece of preview text if there's no title)
    QRect titleRect(left, m_itemRect.top() + m_topMargin,
                    width, boldFontMetrics.height());

    QString title = pItem->title();
    if (title.isEmpty()) {
        title = pItem->previewText();
    }

    if (!title.isEmpty()) {
        m_doc.setHtml(QStringLiteral("<b>") + title + QStringLiteral("</b>"));
    }
    else {
        m_doc.setPlainText(QString());
    }

    m_doc.drawContents(painter, titleRect);

    // 2) Painting the created/modified/deleted datetime
    qint64 targetTimestamp = 0;
    bool deleted = false;

    qint64 deletionTimestamp = pItem->deletionTimestamp();
    if (deletionTimestamp != 0) {
        targetTimestamp = deletionTimestamp;
        deleted = true;
    }
    else {
        qint64 modificationTimestamp = pItem->modificationTimestamp();
        qint64 creationTimestamp = pItem->creationTimestamp();
        targetTimestamp = std::max(creationTimestamp, modificationTimestamp);
    }

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

    qint64 timeSpan = currentTimestamp - targetTimestamp;

#define CHECK_AND_SET_DELETED_PREFIX(text) \
    if (deleted) { \
        text.prepend(tr(QT_TR_NOOP("Deleted")) + QStringLiteral(" ")); \
    }

    QString text;
    if (timeSpan > MSEC_PER_WEEK)
    {
        int pastWeeks = static_cast<int>(std::floor(timeSpan / MSEC_PER_WEEK + 0.5));

        if (pastWeeks == 1) {
            text = (deleted ? tr("Deleted last week") : tr("Last week"));
        }
        else if (pastWeeks < 5) {
            text = tr("%n weeks ago", Q_NULLPTR, pastWeeks);
            CHECK_AND_SET_DELETED_PREFIX(text)
        }
    }
    else if (timeSpan > MSEC_PER_DAY)
    {
        int pastDays = static_cast<int>(std::floor(timeSpan / MSEC_PER_DAY + 0.5));

        if (pastDays == 1) {
            text = (deleted ? tr("Deleted yesterday") : tr("Yesterday"));
        }
        else if (pastDays < 6) {
            text = tr("%n days ago", Q_NULLPTR, pastDays);
            if (deleted) {
                CHECK_AND_SET_DELETED_PREFIX(text)
            }
        }
    }

    if (text.isEmpty()) {
        text = QDateTime::fromMSecsSinceEpoch(targetTimestamp).toString(Qt::ISODate);
        CHECK_AND_SET_DELETED_PREFIX(text)
    }

    int top = m_itemRect.top() + m_topMargin + boldFontMetrics.height() + 2;
    QRect dateTimeRect(left, top, width, m_itemRect.height() - top);

    m_doc.setDefaultFont(smallerFont);
    m_doc.setPlainText(text);
    m_doc.drawContents(painter, dateTimeRect);

#undef CHECK_AND_SET_DELETED_PREFIX

    // 3) Painting the preview text

    top = m_itemRect.top() + m_topMargin + boldFontMetrics.height() + smallerFontMetrics.height() + 4;
    QRect previewTextRect(left, top, width, m_itemRect.height() - top);

    m_doc.setDefaultFont(painter->font());
    m_doc.setPlainText(pItem->previewText());
    m_doc.drawContents(painter, previewTextRect);

    // 4) Painting the thumbnail (if any)
    if (!thumbnail.isNull()) {
        int top = m_itemRect.top() + m_topMargin;
        int bottom = m_itemRect.bottom() - m_bottomMargin;
        QRect thumbnailRect(m_itemRect.width() - 110, top, 100, bottom);

        painter->drawImage(thumbnailRect, thumbnail, thumbnail.rect());
    }

    painter->restore();
}

void NoteItemDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(index)
}

void NoteItemDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(model)
    Q_UNUSED(index)
}

QSize NoteItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(350, 120);
}

void NoteItemDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(editor)
    Q_UNUSED(option)
    Q_UNUSED(index)
}

} // namespace quentier
