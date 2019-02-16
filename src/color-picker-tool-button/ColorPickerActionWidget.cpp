/*
* The MIT License (MIT)
*
* Copyright (c) 2015-2019 Dmitry Ivanov
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include "ColorPickerActionWidget.h"
#include <QColorDialog>

ColorPickerActionWidget::ColorPickerActionWidget(QWidget * parent) :
    QWidgetAction(parent),
    m_colorDialog(new QColorDialog(parent))
{
    m_colorDialog->setWindowFlags(Qt::Widget);
    m_colorDialog->setOptions(QColorDialog::DontUseNativeDialog |
                              QColorDialog::ShowAlphaChannel);

    QColor currentColor = m_colorDialog->currentColor();
    currentColor.setAlpha(255);
    m_colorDialog->setCurrentColor(currentColor);

    QObject::connect(m_colorDialog, SIGNAL(colorSelected(QColor)),
                     this, SIGNAL(colorSelected(QColor)));
    QObject::connect(m_colorDialog, SIGNAL(rejected()),
                     this, SIGNAL(rejected()));

    setDefaultWidget(m_colorDialog);
}

void ColorPickerActionWidget::aboutToShow()
{
    m_colorDialog->show();
}

void ColorPickerActionWidget::aboutToHide()
{
    m_colorDialog->hide();
}
