/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include <QWidgetAction>

namespace quentier {

class TableSizeConstraintsActionWidget : public QWidgetAction
{
    Q_OBJECT
public:
    explicit TableSizeConstraintsActionWidget(QWidget * parent = nullptr);

    [[nodiscard]] double width() const noexcept;
    [[nodiscard]] bool isRelative() const noexcept;

Q_SIGNALS:
    void chosenTableWidthConstraints(double width, bool relative);

private Q_SLOTS:
    void onWidthChange(double width);
    void onWidthTypeChange(int widthTypeIndex);

private:
    double m_currentWidth = 400.0;
    bool m_currentWidthTypeIsRelative = false;
};

} // namespace quentier
