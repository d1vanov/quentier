/*
 * Copyright 2015-2020 Dmitry Ivanov
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

#include "ColorPickerActionWidget.h"

#include <QColorDialog>

namespace quentier {

ColorPickerActionWidget::ColorPickerActionWidget(QWidget * parent) :
    QWidgetAction(parent),
    m_colorDialog(new QColorDialog(parent))
{
    m_colorDialog->setWindowFlags(Qt::Widget);

    m_colorDialog->setOptions(
        QColorDialog::DontUseNativeDialog |
        QColorDialog::ShowAlphaChannel);

    QColor currentColor = m_colorDialog->currentColor();
    currentColor.setAlpha(255);
    m_colorDialog->setCurrentColor(currentColor);

    QObject::connect(
        m_colorDialog,
        &QColorDialog::colorSelected,
        this,
        &ColorPickerActionWidget::colorSelected);

    QObject::connect(
        m_colorDialog,
        &QColorDialog::rejected,
        this,
        &ColorPickerActionWidget::rejected);

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

} // namespace quentier
