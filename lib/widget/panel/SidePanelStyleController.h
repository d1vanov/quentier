/*
 * Copyright 2019 Dmitry Ivanov
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

#ifndef QUENTIER_WIDGETS_SIDE_PANEL_STYLE_CONTROLLER_H
#define QUENTIER_WIDGETS_SIDE_PANEL_STYLE_CONTROLLER_H

#include "PanelStyleController.h"

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace quentier {

/**
 * @brief The SidePanelStyleController class manages the style of a QFrame with
 * child widgets which as a whole represent one of side panels used in the UI
 * of Quentier app
 */
class SidePanelStyleController final : public PanelStyleController
{
public:
    /**
     * @brief SidePanelStyleController contructor
     * @param pPanel        QFrame representing the side panel; certain layout
     *                      of child widgets is expected, otherwise assert
     *                      fires!
     */
    explicit SidePanelStyleController(QFrame * pPanel);

    QString title() const;
    void setTitle(const QString & title);

private:
    void findChildWidgets();

    virtual QString generateStyleSheet() const override;

private:
    QLabel * m_pTitleLabel = nullptr;
    QPushButton * m_pStaticIconHolder = nullptr;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_SIDE_PANEL_STYLE_CONTROLLER_H
