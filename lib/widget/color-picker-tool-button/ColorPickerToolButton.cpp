/*
 * Copyright 2015-2021 Dmitry Ivanov
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

#include <QColorDialog>
#include <QMenu>

#include <memory>

namespace quentier {

ColorPickerToolButton::ColorPickerToolButton(QWidget * parent) :
    QToolButton(parent), m_menu(new QMenu(this))
{
    auto * colorPickerActionWidget = new ColorPickerActionWidget(this);
    m_menu->addAction(colorPickerActionWidget);

    setMenu(m_menu);

    auto * colorDialogAction = new QAction(this);
    setDefaultAction(colorDialogAction);

    QObject::connect(
        colorDialogAction, &QAction::triggered, this,
        &ColorPickerToolButton::onColorDialogAction);

    QObject::connect(
        colorPickerActionWidget, &ColorPickerActionWidget::colorSelected, this,
        &ColorPickerToolButton::colorSelected);

    QObject::connect(
        colorPickerActionWidget, &ColorPickerActionWidget::rejected, this,
        &ColorPickerToolButton::rejected);

    QObject::connect(
        colorPickerActionWidget, &ColorPickerActionWidget::colorSelected,
        m_menu, &QMenu::hide);

    QObject::connect(
        colorPickerActionWidget, &ColorPickerActionWidget::rejected, m_menu,
        &QMenu::hide);

    QObject::connect(
        m_menu, &QMenu::aboutToShow, colorPickerActionWidget,
        &ColorPickerActionWidget::aboutToShow);

    QObject::connect(
        m_menu, &QMenu::aboutToHide, colorPickerActionWidget,
        &ColorPickerActionWidget::aboutToHide);
}

void ColorPickerToolButton::onColorDialogAction()
{
    auto pColorDialog = std::make_unique<QColorDialog>(this);

    pColorDialog->setOptions(
        QColorDialog::DontUseNativeDialog | QColorDialog::ShowAlphaChannel);

    auto currentColor = pColorDialog->currentColor();
    currentColor.setAlpha(255);
    pColorDialog->setCurrentColor(currentColor);

    if (pColorDialog->exec() == QColorDialog::Accepted) {
        auto color = pColorDialog->currentColor();
        Q_EMIT colorSelected(color);
    }
    else {
        Q_EMIT rejected();
    }
}

} // namespace quentier
