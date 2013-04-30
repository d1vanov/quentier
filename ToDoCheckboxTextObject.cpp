#include "ToDoCheckboxTextObject.h"
#include <QTextDocument>
#include <QTextFormat>
#include <QPainter>
#include <QRectF>
#include <QSizeF>

ToDoCheckboxTextObject::ToDoCheckboxTextObject()
{
    QCheckBox::setTristate(false);
}

void ToDoCheckboxTextObject::drawObject(QPainter * pPainter, const QRectF & rect,
                                        QTextDocument * /* pDoc */, int /* positionInDocument */,
                                        const QTextFormat & format)
{
    QImage bufferedImage = qvariant_cast<QImage>(format.property(CheckboxTextData));
    pPainter->drawImage(rect, bufferedImage);
}

QSizeF ToDoCheckboxTextObject::intrinsicSize(QTextDocument * /* pDoc */,
                                             int /* positionInDocument */,
                                             const QTextFormat & format)
{
    QImage bufferedImage = qvariant_cast<QImage>(format.property(CheckboxTextData));
    QSize size = bufferedImage.size();

    if (size.height() > 25) {
        size *= 25.0 / static_cast<double>(size.height());
    }

    return QSizeF(size);
}

QImage ToDoCheckboxTextObject::getImage() const
{
    return icon().pixmap(icon().actualSize(QSize(200,200))).toImage();
}
