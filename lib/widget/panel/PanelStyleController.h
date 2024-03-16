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

#include <QColor>
#include <QLinearGradient>

#include <memory>

class QFrame;

namespace quentier {

/**
 * @brief The PanelStyleController class manages the style of a QFrame which
 * represents one of panels used in the UI of Quentier app
 */
class PanelStyleController
{
public:
    explicit PanelStyleController(
        QFrame * panel, QString extraStyleSheet = {});

    virtual ~PanelStyleController() = default;

    [[nodiscard]] QFrame * panel();

    [[nodiscard]] QColor overrideFontColor() const;
    void setOverrideFontColor(QColor color);

    [[nodiscard]] QColor overrideBackgroundColor() const;
    void setOverrideBackgroundColor(QColor color);

    [[nodiscard]] const QLinearGradient * overrideBackgroundGradient() const;
    void setOverrideBackgroundGradient(QLinearGradient gradient);
    void resetOverrideBackgroundGradient();

    void setOverrideColors(QColor fontColor, QColor backgroundColor);
    void setOverrideColors(
        QColor fontColor, QLinearGradient backgroundGradient);

    virtual void resetOverrides();

protected:
    [[nodiscard]] QString backgroundColorToString() const;
    [[nodiscard]] QString gradientToString(
        const QLinearGradient & gradient) const;

    [[nodiscard]] QLinearGradient lighterGradient(
        const QLinearGradient & gradient) const;

    [[nodiscard]] QLinearGradient darkerGradient(
        const QLinearGradient & gradient) const;

    [[nodiscard]] virtual QString generateStyleSheet() const;

    void updateStyleSheet();

protected:
    QFrame * m_panel = nullptr;
    QString m_defaultStyleSheet;
    QString m_extraStyleSheet;

    QColor m_overrideFontColor;
    QColor m_overrideBackgroundColor;
    std::unique_ptr<QLinearGradient> m_overrideBackgroundGradient;
};

} // namespace quentier
