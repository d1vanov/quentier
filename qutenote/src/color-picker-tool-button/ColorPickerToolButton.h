/*
* The MIT License (MIT)
*
* Copyright (c) 2015 Dmitry Ivanov
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

#ifndef COLOR_PICKER_TOOL_BUTTON_H
#define COLOR_PICKER_TOOL_BUTTON_H

#include <QToolButton>
#include <QColor>

QT_FORWARD_DECLARE_CLASS(QMenu)

class ColorPickerToolButton: public QToolButton
{
    Q_OBJECT
public:
    explicit ColorPickerToolButton(QWidget * parent = 0);

Q_SIGNALS:
    void colorSelected(QColor color);
    void rejected();

private Q_SLOTS:
    void onColorDialogAction();

private:
    QMenu * m_menu;
};

#endif // COLOR_PICKER_TOOL_BUTTON_H
