/*
 * Copyright 2015-2019 Dmitry Ivanov
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

#include "ColorPickerToolButton.h"
#include "ColorPickerActionWidget.h"

#include <QMenu>
#include <QColorDialog>
#include <QScopedPointer>

namespace quentier {

ColorPickerToolButton::ColorPickerToolButton(QWidget * parent) :
    QToolButton(parent),
    m_menu(new QMenu(this))
{
    ColorPickerActionWidget * colorPickerActionWidget =
        new ColorPickerActionWidget(this);
    m_menu->addAction(colorPickerActionWidget);

    setMenu(m_menu);

    QAction * colorDialogAction = new QAction(this);
    setDefaultAction(colorDialogAction);

    QObject::connect(colorDialogAction, SIGNAL(triggered(bool)),
                     this, SLOT(onColorDialogAction()));

    QObject::connect(colorPickerActionWidget, SIGNAL(colorSelected(QColor)),
                     this, SIGNAL(colorSelected(QColor)));
    QObject::connect(colorPickerActionWidget, SIGNAL(rejected()),
                     this, SIGNAL(rejected()));

    QObject::connect(colorPickerActionWidget, SIGNAL(colorSelected(QColor)),
                     m_menu, SLOT(hide()));
    QObject::connect(colorPickerActionWidget, SIGNAL(rejected()),
                     m_menu, SLOT(hide()));

    QObject::connect(m_menu, SIGNAL(aboutToShow()),
                     colorPickerActionWidget, SLOT(aboutToShow()));
    QObject::connect(m_menu, SIGNAL(aboutToHide()),
                     colorPickerActionWidget, SLOT(aboutToHide()));
}

void ColorPickerToolButton::onColorDialogAction()
{
    QScopedPointer<QColorDialog> colorDialogPtr(new QColorDialog(this));
    QColorDialog * colorDialog = colorDialogPtr.data();
    colorDialog->setOptions(QColorDialog::DontUseNativeDialog |
                            QColorDialog::ShowAlphaChannel);

    QColor currentColor = colorDialog->currentColor();
    currentColor.setAlpha(255);
    colorDialog->setCurrentColor(currentColor);

    if (colorDialog->exec() == QColorDialog::Accepted)
    {
        QColor color = colorDialog->currentColor();
        Q_EMIT colorSelected(color);
    }
    else
    {
        Q_EMIT rejected();
    }
}

} // namespace quentier
