/*
 * Copyright 2019-2020 Dmitry Ivanov
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

#ifndef QUENTIER_WIDGETS_PANEL_STYLE_CONTROLLER_H
#define QUENTIER_WIDGETS_PANEL_STYLE_CONTROLLER_H

#include <QColor>
#include <QLinearGradient>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QFrame)

namespace quentier {

/**
 * @brief The PanelStyleController class manages the style of a QFrame which
 * represents one of panels used in the UI of Quentier app
 */
class PanelStyleController
{
public:
    explicit PanelStyleController(
        QFrame * pPanel, QString extraStyleSheet = {});

    virtual ~PanelStyleController() = default;

    QFrame * panel();

    QColor overrideFontColor() const;
    void setOverrideFontColor(QColor color);

    QColor overrideBackgroundColor() const;
    void setOverrideBackgroundColor(QColor color);

    const QLinearGradient * overrideBackgroundGradient() const;
    void setOverrideBackgroundGradient(QLinearGradient gradient);
    void resetOverrideBackgroundGradient();

    void setOverrideColors(QColor fontColor, QColor backgroundColor);
    void setOverrideColors(
        QColor fontColor, QLinearGradient backgroundGradient);

    virtual void resetOverrides();

protected:
    QString backgroundColorToString() const;
    QString gradientToString(const QLinearGradient & gradient) const;

    QLinearGradient lighterGradient(const QLinearGradient & gradient) const;
    QLinearGradient darkerGradient(const QLinearGradient & gradient) const;

    virtual QString generateStyleSheet() const;

    void updateStyleSheet();

protected:
    QFrame * m_pPanel = nullptr;
    QString m_defaultStyleSheet;
    QString m_extraStyleSheet;

    QColor m_overrideFontColor;
    QColor m_overrideBackgroundColor;
    std::unique_ptr<QLinearGradient> m_pOverrideBackgroundGradient;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_PANEL_STYLE_CONTROLLER_H
