/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#pragma once

#include "PanelStyleController.h"

class QLabel;
class QPushButton;

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
    explicit SidePanelStyleController(QFrame * panel);

    [[nodiscard]] QString title() const;
    void setTitle(const QString & title);

private:
    void findChildWidgets();

    [[nodiscard]] QString generateStyleSheet() const override;

private:
    QLabel * m_titleLabel = nullptr;
    QPushButton * m_staticIconHolder = nullptr;
};

} // namespace quentier
