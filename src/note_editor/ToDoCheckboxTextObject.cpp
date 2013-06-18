#include "ToDoCheckboxTextObject.h"
#include "NoteEditorWidget.h"
#include <QTextDocument>
#include <QTextFormat>
#include <QPainter>
#include <QRectF>
#include <QSizeF>

void ToDoCheckboxTextObjectUnchecked::drawObject(QPainter * pPainter, const QRectF & rect,
                                                 QTextDocument * /* pDoc */, int /* positionInDocument */,
                                                 const QTextFormat & format)
{
    QImage bufferedImage = qvariant_cast<QImage>(format.property(NoteEditorWidget::CHECKBOX_TEXT_DATA_UNCHECKED));
    pPainter->drawImage(rect, bufferedImage);
}

QSizeF ToDoCheckboxTextObjectUnchecked::intrinsicSize(QTextDocument * /* pDoc */,
                                                      int /* positionInDocument */,
                                                      const QTextFormat & format)
{
    QImage bufferedImage = qvariant_cast<QImage>(format.property(NoteEditorWidget::CHECKBOX_TEXT_DATA_UNCHECKED));
    QSize size = bufferedImage.size();

    if (size.height() > 25) {
        size *= 25.0 / static_cast<double>(size.height());
    }

    return QSizeF(size);
}

void ToDoCheckboxTextObjectChecked::drawObject(QPainter * pPainter, const QRectF & rect,
                                               QTextDocument * /* pDoc */, int /* positionInDocument */,
                                               const QTextFormat & format)
{
    QImage bufferedImage = qvariant_cast<QImage>(format.property(NoteEditorWidget::CHECKBOX_TEXT_DATA_CHECKED));
    pPainter->drawImage(rect, bufferedImage);
}

QSizeF ToDoCheckboxTextObjectChecked::intrinsicSize(QTextDocument * /* pDoc */,
                                                    int /* positionInDocument */,
                                                    const QTextFormat & format)
{
    QImage bufferedImage = qvariant_cast<QImage>(format.property(NoteEditorWidget::CHECKBOX_TEXT_DATA_CHECKED));
    QSize size = bufferedImage.size();

    if (size.height() > 25) {
        size *= 25.0 / static_cast<double>(size.height());
    }

    return QSizeF(size);
}
