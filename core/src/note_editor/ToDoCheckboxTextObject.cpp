#include "ToDoCheckboxTextObject.h"
#include "QuteNoteTextEdit.h"
#include <logging/QuteNoteLogger.h>
#include <QTextDocument>
#include <QTextFormat>
#include <QPainter>
#include <QRectF>
#include <QSizeF>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QCheckBox>

void ToDoCheckboxTextObjectUnchecked::drawObject(QPainter * pPainter, const QRectF & /* rect */,
                                                 QTextDocument * /* pDoc */, int /* positionInDocument */,
                                                 const QTextFormat & /* format */)
{
    QStyle * pStyle = QApplication::style();
    if (!pStyle) {
        QNWARNING("Detected null pointer to QStyle from QApplication; can't render unchecked checkbox");
        return;
    }

    QStyleOptionButton styleOptionButton;
    styleOptionButton.state = QStyle::State_Enabled;
    styleOptionButton.state |= QStyle::State_Off;

    pStyle->drawControl(QStyle::CE_CheckBox, &styleOptionButton, pPainter);
}

QSizeF ToDoCheckboxTextObjectUnchecked::intrinsicSize(QTextDocument * /* pDoc */,
                                                      int /* positionInDocument */,
                                                      const QTextFormat & format)
{

    QStyle * pStyle = QApplication::style();
    if (!pStyle) {
        QNWARNING("Detected null pointer to QStyle from QApplication; can't get intrinsic size for unchecked checkbox");
        return QSizeF();
    }

    QStyleOption styleOptionButton;
    styleOptionButton.state = QStyle::State_Enabled;
    styleOptionButton.state |= QStyle::State_Off;

    QCheckBox checkbox;
    checkbox.setCheckState(Qt::Unchecked);

    QSize sizeHint = checkbox.sizeHint();
    QNTRACE("ToDoCheckboxTextObjectUnchecked::intrinsicSize: original size hint: height = "
            << sizeHint.height() << ", width: " << sizeHint.width() << ", is valid = " << (sizeHint.isValid() ? "true" : "false"));

    if (!sizeHint.isValid()) {
        sizeHint.setHeight(25);
        sizeHint.setWidth(25);
    }

    QSize sizeFromContents = pStyle->sizeFromContents(QStyle::CT_CheckBox, &styleOptionButton, sizeHint);
    QNTRACE("Size from contents: height = " << sizeFromContents.height() << ", width = " << sizeFromContents.width()
            << ", is valid = " << (sizeFromContents.isValid() ? "true" : "false"));

    return QSizeF(sizeFromContents);
}

void ToDoCheckboxTextObjectChecked::drawObject(QPainter * pPainter, const QRectF & rect,
                                               QTextDocument * /* pDoc */, int /* positionInDocument */,
                                               const QTextFormat & format)
{
    QImage bufferedImage = qvariant_cast<QImage>(format.property(QuteNoteTextEdit::TODO_CHKBOX_TXT_DATA_CHECKED));
    pPainter->drawImage(rect, bufferedImage);
}

QSizeF ToDoCheckboxTextObjectChecked::intrinsicSize(QTextDocument * /* pDoc */,
                                                    int /* positionInDocument */,
                                                    const QTextFormat & format)
{
    QImage bufferedImage = qvariant_cast<QImage>(format.property(QuteNoteTextEdit::TODO_CHKBOX_TXT_DATA_CHECKED));
    QSize size = bufferedImage.size();

    if (size.height() > 25) {
        size *= 25.0 / static_cast<double>(size.height());
    }

    return QSizeF(size);
}
