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
#include <QtGlobal>

void drawCheckboxImpl(const QCheckBox & checkbox, const QRectF & rectF, const bool checked,
                      QPainter * pPainter)
{
    if (Q_UNLIKELY(!pPainter)) {
        QNWARNING("Detected null pointer to QPainter, can't render " << (checked ? "checked" : "unchecked") << " checkbox");
        return;
    }

    QStyle * pStyle = QApplication::style();
    if (Q_UNLIKELY(!pStyle)) {
        QNWARNING("Detected null pointer to QStyle from QApplication; can't render "
                  << (checked ? "checked" : "unchecked") << " checkbox");
        return;
    }

    QStyleOptionButton styleOptionButton;
    styleOptionButton.state = QStyle::State_Enabled;
    if (checked) {
        styleOptionButton.state |= QStyle::State_On;
    }
    else {
        styleOptionButton.state |= QStyle::State_Off;
    }

    QSize rectSize(static_cast<int>(rectF.width()), static_cast<int>(rectF.height()));

    QPointF topLeftF = rectF.topLeft();
    QPoint topLeft;
    topLeft.setX(static_cast<int>(topLeftF.x()));
    topLeft.setY(static_cast<int>(topLeftF.y()));

    QRect rect(topLeft, rectSize);

    QRect alignedRect = pStyle->alignedRect(Qt::LeftToRight, Qt::AlignLeft | Qt::AlignBottom, rectSize, rect);

    styleOptionButton.rect = alignedRect;
    styleOptionButton.icon = checkbox.icon();

    pStyle->drawControl(QStyle::CE_CheckBox, &styleOptionButton, pPainter);
}

ToDoCheckboxTextObjectUnchecked::ToDoCheckboxTextObjectUnchecked() :
    m_checkbox()
{
    m_checkbox.setCheckState(Qt::Unchecked);
}

void ToDoCheckboxTextObjectUnchecked::drawObject(QPainter * pPainter, const QRectF & rectF,
                                                 QTextDocument * /* pDoc */, int /* positionInDocument */,
                                                 const QTextFormat & /* format */)
{
    drawCheckboxImpl(m_checkbox, rectF, /* checked = */ false, pPainter);
}

QSizeF ToDoCheckboxTextObjectUnchecked::intrinsicSize(QTextDocument * /* pDoc */,
                                                      int /* positionInDocument */,
                                                      const QTextFormat & /* format */)
{
    return QSizeF(m_checkbox.iconSize());
}

ToDoCheckboxTextObjectChecked::ToDoCheckboxTextObjectChecked() :
    m_checkbox()
{
    m_checkbox.setCheckState(Qt::Checked);
}

void ToDoCheckboxTextObjectChecked::drawObject(QPainter * pPainter, const QRectF & rectF,
                                               QTextDocument * /* pDoc */, int /* positionInDocument */,
                                               const QTextFormat & format)
{
    drawCheckboxImpl(m_checkbox, rectF, /* checked = */ true, pPainter);
}

QSizeF ToDoCheckboxTextObjectChecked::intrinsicSize(QTextDocument * /* pDoc */,
                                                    int /* positionInDocument */,
                                                    const QTextFormat & /* format */)
{
    return QSizeF(m_checkbox.iconSize());
}
