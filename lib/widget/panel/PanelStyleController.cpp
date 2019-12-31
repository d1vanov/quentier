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

#include "PanelStyleController.h"

#include <quentier/logging/QuentierLogger.h>

#include <QFrame>
#include <QTextStream>

namespace quentier {

PanelStyleController::PanelStyleController(QFrame * pPanel, QString extraStyleSheet) :
    m_pPanel(pPanel),
    m_extraStyleSheet(std::move(extraStyleSheet))
{
    Q_ASSERT(m_pPanel);
    m_defaultStyleSheet = m_pPanel->styleSheet();
}

QFrame * PanelStyleController::panel()
{
    return m_pPanel;
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
        m_pOverrideBackgroundGradient.reset();
    }

    updateStyleSheet();
}

const QLinearGradient * PanelStyleController::overrideBackgroundGradient() const
{
    return m_pOverrideBackgroundGradient.get();
}

void PanelStyleController::setOverrideBackgroundGradient(
    QLinearGradient gradient)
{
    if (m_pOverrideBackgroundGradient &&
        (*m_pOverrideBackgroundGradient == gradient))
    {
        return;
    }

    if (m_pOverrideBackgroundGradient) {
        *m_pOverrideBackgroundGradient = std::move(gradient);
    }
    else {
        m_pOverrideBackgroundGradient = std::make_unique<QLinearGradient>(
            std::move(gradient));
    }

    m_overrideBackgroundColor = QColor();
    updateStyleSheet();
}

void PanelStyleController::resetOverrideBackgroundGradient()
{
    if (!m_pOverrideBackgroundGradient) {
        // Nothing to do
        return;
    }

    m_pOverrideBackgroundGradient.reset();
    updateStyleSheet();
}

void PanelStyleController::resetOverrides()
{
    if (!m_overrideBackgroundColor.isValid() &&
        !m_pOverrideBackgroundGradient)
    {
        // Nothing to do
        return;
    }

    m_overrideBackgroundColor = QColor();
    m_pOverrideBackgroundGradient.reset();

    updateStyleSheet();
}

QString PanelStyleController::backgroundColorToString() const
{
    if (m_overrideBackgroundColor.isValid()) {
        return m_overrideBackgroundColor.name(QColor::HexRgb);
    }

    if (!m_pOverrideBackgroundGradient) {
        return {};
    }

    return gradientToString(*m_pOverrideBackgroundGradient);
}

QString PanelStyleController::gradientToString(
    const QLinearGradient & gradient) const
{
    auto stops = gradient.stops();
    if (stops.isEmpty()) {
        return {};
    }

    auto start = gradient.start();
    auto finalStop = gradient.finalStop();

    QString result;
    QTextStream strm(&result);

    strm << "qlineargradient(x1: " << start.x() << ", y1: " << start.y()
         << ", x2: " << finalStop.x() << ", y2: " << finalStop.y() << ",\n";

    for(int i = 0, size = stops.size(); i < size; ++i)
    {
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
    QLinearGradient result(gradient.start(), gradient.finalStop());
    auto stops = gradient.stops();
    for(const auto & stop: stops) {
        result.setColorAt(stop.first, stop.second.lighter(150));
    }
    return result;
}

QLinearGradient PanelStyleController::darkerGradient(
    const QLinearGradient & gradient) const
{
    QLinearGradient result(gradient.start(), gradient.finalStop());
    auto stops = gradient.stops();
    for(const auto & stop: stops) {
        result.setColorAt(stop.first, stop.second.darker(200));
    }
    return result;
}

QString PanelStyleController::generateStyleSheet() const
{
    if (!m_overrideBackgroundColor.isValid() && !m_pOverrideBackgroundGradient) {
        return m_defaultStyleSheet;
    }

    QString result;
    QTextStream strm(&result);

    strm << "QWidget {\n"
        << "border: none;\n";

    auto backgroundColorStr = backgroundColorToString();
    if (!backgroundColorStr.isEmpty()) {
        strm << "background-color: " << backgroundColorStr << ";\n";
    }

    strm << "}\n";

    strm << m_extraStyleSheet;

    return result;
}

void PanelStyleController::updateStyleSheet()
{
    auto styleSheetStr = generateStyleSheet();
    QNDEBUG("PanelStyleController::updateStyleSheet: setting stylesheet "
        << "for panel " << m_pPanel->objectName() << ":" << styleSheetStr);
    m_pPanel->setStyleSheet(std::move(styleSheetStr));
}

} // namespace quentier
