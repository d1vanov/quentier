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

#include "PanelStyleController.h"

#include <quentier/logging/QuentierLogger.h>

#include <QFrame>
#include <QTextStream>

namespace quentier {

PanelStyleController::PanelStyleController(
    QFrame * panel, QString extraStyleSheet) :
    m_panel{panel},
    m_extraStyleSheet{std::move(extraStyleSheet)}
{
    Q_ASSERT(m_panel);
    m_defaultStyleSheet = m_panel->styleSheet();
}

QFrame * PanelStyleController::panel()
{
    return m_panel;
}

QColor PanelStyleController::overrideFontColor() const
{
    return m_overrideFontColor;
}

void PanelStyleController::setOverrideFontColor(QColor color)
{
    if (m_overrideFontColor == color) {
        return;
    }

    m_overrideFontColor = std::move(color);
    updateStyleSheet();
}

QColor PanelStyleController::overrideBackgroundColor() const
{
    return m_overrideBackgroundColor;
}

void PanelStyleController::setOverrideBackgroundColor(QColor color)
{
    if (m_overrideBackgroundColor == color) {
        return;
    }

    m_overrideBackgroundColor = std::move(color);
    if (m_overrideBackgroundColor.isValid()) {
        m_overrideBackgroundGradient.reset();
    }

    updateStyleSheet();
}

const QLinearGradient * PanelStyleController::overrideBackgroundGradient() const
{
    return m_overrideBackgroundGradient.get();
}

void PanelStyleController::setOverrideBackgroundGradient(
    QLinearGradient gradient)
{
    if (m_overrideBackgroundGradient &&
        *m_overrideBackgroundGradient == gradient)
    {
        return;
    }

    if (m_overrideBackgroundGradient) {
        *m_overrideBackgroundGradient = std::move(gradient);
    }
    else {
        m_overrideBackgroundGradient =
            std::make_unique<QLinearGradient>(std::move(gradient));
    }

    m_overrideBackgroundColor = QColor{};
    updateStyleSheet();
}

void PanelStyleController::resetOverrideBackgroundGradient()
{
    if (!m_overrideBackgroundGradient) {
        // Nothing to do
        return;
    }

    m_overrideBackgroundGradient.reset();
    updateStyleSheet();
}

void PanelStyleController::setOverrideColors(
    QColor fontColor, QColor backgroundColor)
{
    if (m_overrideFontColor == fontColor &&
        m_overrideBackgroundColor == backgroundColor)
    {
        return;
    }

    m_overrideFontColor = std::move(fontColor);
    m_overrideBackgroundColor = std::move(backgroundColor);
    if (m_overrideBackgroundColor.isValid()) {
        m_overrideBackgroundGradient.reset();
    }

    updateStyleSheet();
}

void PanelStyleController::setOverrideColors(
    QColor fontColor, QLinearGradient backgroundGradient)
{
    if (m_overrideFontColor == fontColor && m_overrideBackgroundGradient &&
        *m_overrideBackgroundGradient == backgroundGradient)
    {
        return;
    }

    m_overrideFontColor = std::move(fontColor);

    if (m_overrideBackgroundGradient) {
        *m_overrideBackgroundGradient = std::move(backgroundGradient);
    }
    else {
        m_overrideBackgroundGradient =
            std::make_unique<QLinearGradient>(std::move(backgroundGradient));
    }

    m_overrideBackgroundColor = QColor{};
    updateStyleSheet();
}

void PanelStyleController::resetOverrides()
{
    if (!m_overrideFontColor.isValid() &&
        !m_overrideBackgroundColor.isValid() && !m_overrideBackgroundGradient)
    {
        // Nothing to do
        return;
    }

    m_overrideFontColor = QColor{};
    m_overrideBackgroundColor = QColor{};
    m_overrideBackgroundGradient.reset();

    updateStyleSheet();
}

QString PanelStyleController::backgroundColorToString() const
{
    if (m_overrideBackgroundColor.isValid()) {
        return m_overrideBackgroundColor.name(QColor::HexRgb);
    }

    if (!m_overrideBackgroundGradient) {
        return {};
    }

    return gradientToString(*m_overrideBackgroundGradient);
}

QString PanelStyleController::gradientToString(
    const QLinearGradient & gradient) const
{
    const auto stops = gradient.stops();
    if (stops.isEmpty()) {
        return {};
    }

    const auto start = gradient.start();
    const auto finalStop = gradient.finalStop();

    QString result;
    QTextStream strm{&result};

    strm << "qlineargradient(x1: " << start.x() << ", y1: " << start.y()
         << ", x2: " << finalStop.x() << ", y2: " << finalStop.y() << ",\n";

    for (int i = 0, size = stops.size(); i < size; ++i) {
        const auto & stop = stops[i];

        strm << "stop: " << stop.first << " "
             << stop.second.name(QColor::HexRgb);

        if (i != (size - 1)) {
            strm << ",";
        }
        else {
            strm << ")";
        }
    }

    strm.flush();
    return result;
}

QLinearGradient PanelStyleController::lighterGradient(
    const QLinearGradient & gradient) const
{
    QLinearGradient result{gradient.start(), gradient.finalStop()};
    const auto stops = gradient.stops();
    for (const auto & stop: std::as_const(stops)) {
        result.setColorAt(stop.first, stop.second.lighter(150));
    }
    return result;
}

QLinearGradient PanelStyleController::darkerGradient(
    const QLinearGradient & gradient) const
{
    QLinearGradient result{gradient.start(), gradient.finalStop()};
    const auto stops = gradient.stops();
    for (const auto & stop: std::as_const(stops)) {
        result.setColorAt(stop.first, stop.second.darker(200));
    }
    return result;
}

QString PanelStyleController::generateStyleSheet() const
{
    if (!m_overrideBackgroundColor.isValid() && !m_overrideBackgroundGradient) {
        return m_defaultStyleSheet;
    }

    QString result;
    QTextStream strm{&result};

    strm << "#" << m_panel->objectName() << " {\n"
         << "border: none;\n";

    const auto backgroundColorStr = backgroundColorToString();
    if (!backgroundColorStr.isEmpty()) {
        strm << "background-color: " << backgroundColorStr << ";\n";
    }

    strm << "}\n";

    if (m_overrideFontColor.isValid()) {
        strm << "QLabel {\n"
             << "color: " << m_overrideFontColor.name() << ";\n"
             << "}\n";
    }

    strm << "QLabel {\n"
         << "border: none;\n";

    if (m_overrideFontColor.isValid()) {
        strm << "color: " << m_overrideFontColor.name() << ";\n";
    }

    strm << "}\n";

    strm << m_extraStyleSheet;

    return result;
}

void PanelStyleController::updateStyleSheet()
{
    auto styleSheetStr = generateStyleSheet();

    QNDEBUG(
        "widget::PanelStyleController",
        "PanelStyleController::updateStyleSheet: setting "
            << "stylesheet for panel " << m_panel->objectName() << ":"
            << styleSheetStr);

    m_panel->setStyleSheet(std::move(styleSheetStr));
}

} // namespace quentier
