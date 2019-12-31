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

#include "SidePanelController.h"

#include <quentier/logging/QuentierLogger.h>

#include <QFrame>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>

namespace quentier {

SidePanelController::SidePanelController(QFrame * pPanel) :
    m_pPanel(pPanel)
{
    Q_ASSERT(m_pPanel);
    m_defaultStyleSheet = m_pPanel->styleSheet();
    findChildWidgets();
}

QFrame * SidePanelController::panel()
{
    return m_pPanel;
}

QString SidePanelController::title() const
{
    return m_pTitleLabel->text();
}

void SidePanelController::setTitle(const QString & title)
{
    m_pTitleLabel->setText(title);
}

QColor SidePanelController::overrideFontColor() const
{
    return m_overrideFontColor;
}

void SidePanelController::setOverrideFontColor(QColor color)
{
    if (m_overrideFontColor == color) {
        return;
    }

    m_overrideFontColor = std::move(color);
    updateStyleSheet();
}

QColor SidePanelController::overrideBackgroundColor() const
{
    return m_overrideBackgroundColor;
}

void SidePanelController::setOverrideBackgroundColor(QColor color)
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

const QLinearGradient * SidePanelController::overrideBackgroundGradient() const
{
    return m_pOverrideBackgroundGradient.get();
}

void SidePanelController::setOverrideBackgroundGradient(
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

void SidePanelController::resetOverrideBackgroundGradient()
{
    if (!m_pOverrideBackgroundGradient) {
        // Nothing to do
        return;
    }

    m_pOverrideBackgroundGradient.reset();
    updateStyleSheet();
}

void SidePanelController::setOverrideColors(
    QColor fontColor, QColor backgroundColor)
{
    if ((m_overrideFontColor == fontColor) &&
        (m_overrideBackgroundColor == backgroundColor))
    {
        return;
    }

    m_overrideFontColor = std::move(fontColor);
    m_overrideBackgroundColor = std::move(backgroundColor);
    if (m_overrideBackgroundColor.isValid()) {
        m_pOverrideBackgroundGradient.reset();
    }

    updateStyleSheet();
}

void SidePanelController::setOverrideColors(
    QColor fontColor, QLinearGradient backgroundGradient)
{
    if ((m_overrideFontColor == fontColor) &&
        (m_pOverrideBackgroundGradient &&
         (*m_pOverrideBackgroundGradient == backgroundGradient)))
    {
        return;
    }

    m_overrideFontColor = std::move(fontColor);

    if (m_pOverrideBackgroundGradient) {
        *m_pOverrideBackgroundGradient = std::move(backgroundGradient);
    }
    else {
        m_pOverrideBackgroundGradient = std::make_unique<QLinearGradient>(
            std::move(backgroundGradient));
    }

    m_overrideBackgroundColor = QColor();
    updateStyleSheet();
}

void SidePanelController::resetOverrides()
{
    if (!m_overrideFontColor.isValid() &&
        !m_overrideBackgroundColor.isValid() &&
        !m_pOverrideBackgroundGradient)
    {
        // Nothing to do
        return;
    }

    m_overrideFontColor = QColor();
    m_overrideBackgroundColor = QColor();
    m_pOverrideBackgroundGradient.reset();

    updateStyleSheet();
}

void SidePanelController::findChildWidgets()
{
    auto staticIconHolders = m_pPanel->findChildren<QPushButton*>(
        QRegularExpression(QStringLiteral("(.*)PanelIconPseudoPushButton")));
    Q_ASSERT(staticIconHolders.size() == 1);
    m_pStaticIconHolder = staticIconHolders[0];
    Q_ASSERT(m_pStaticIconHolder);

    auto labels = m_pPanel->findChildren<QLabel*>();
    Q_ASSERT(labels.size() == 2);
    for(const auto pLabel: labels) {
        if (pLabel->objectName().endsWith(QStringLiteral("LeftPaddingLabel"))) {
            continue;
        }

        m_pTitleLabel = pLabel;
        break;
    }

    Q_ASSERT(m_pTitleLabel);
}

void SidePanelController::updateStyleSheet()
{
    if (!m_overrideFontColor.isValid() &&
        !m_overrideBackgroundColor.isValid() &&
        !m_pOverrideBackgroundGradient)
    {
        m_pPanel->setStyleSheet(m_defaultStyleSheet);
        return;
    }

    QString styleSheetStr;
    QTextStream strm(&styleSheetStr);

    strm << "QWidget {\n"
        << "border: none;\n";

    auto backgroundColorStr = backgroundColorToString();
    if (!backgroundColorStr.isEmpty()) {
        strm << "background-color: " << backgroundColorStr << ";\n";
    }

    if (m_overrideFontColor.isValid()) {
        strm << "color: " << m_overrideFontColor.name(QColor::HexRgb) << ";\n";
    }

    strm << "}\n";

    strm << "\n"
        << "QPushButton {\n"
        << "border: none;\n"
        << "padding: 0px;\n"
        << "}\n"
        << "\n"
        << "QPushButton:focus {\n"
        << "outline: none;\n"
        << "}\n";

    strm << "QLabel {\n"
        << "border: none;\n";

    if (m_overrideFontColor.isValid()) {
        strm << "color: " << m_overrideFontColor.name(QColor::HexRgb) << ";\n";
    }

    strm << "}\n";

    if (backgroundColorStr.isEmpty()) {
        strm.flush();
        QNDEBUG("Setting stylesheet for panel " << m_pPanel->objectName()
               << ": " << styleSheetStr);
        m_pPanel->setStyleSheet(styleSheetStr);
        return;
    }

    strm << "QPushButton:hover:!pressed {\n";
    strm << "background-color: ";
    if (m_overrideBackgroundColor.isValid()) {
        strm << m_overrideBackgroundColor.lighter(150).name(QColor::HexRgb)
            << ";\n";
    }
    else {
        Q_ASSERT(m_pOverrideBackgroundGradient);
        strm << gradientToString(lighterGradient(*m_pOverrideBackgroundGradient))
            << ";\n";
    }
    strm << "}\n";

    strm << "\n";

    strm << "QPushButton:pressed {\n";
    strm << "background-color: ";
    if (m_overrideBackgroundColor.isValid()) {
        strm << m_overrideBackgroundColor.darker(200).name(QColor::HexRgb)
            << ";\n";
    }
    else {
        Q_ASSERT(m_pOverrideBackgroundGradient);
        strm << gradientToString(darkerGradient(*m_pOverrideBackgroundGradient))
            << ";\n";
    }
    strm << "}\n";

    strm << "\n";

    strm << "QPushButton#" << m_pStaticIconHolder->objectName() << ":hover{\n"
        << "background-color: " << backgroundColorStr << ";\n"
        << "}\n";

    strm << "\n";

    strm << "QPushButton#" << m_pStaticIconHolder->objectName() << ":pressed{\n"
        << "background-color: " << backgroundColorStr << ";\n"
        << "}\n";

    strm.flush();
    QNDEBUG("Setting stylesheet for panel " << m_pPanel->objectName()
           << ": " << styleSheetStr);
    m_pPanel->setStyleSheet(styleSheetStr);
}

QString SidePanelController::backgroundColorToString() const
{
    if (m_overrideBackgroundColor.isValid()) {
        return m_overrideBackgroundColor.name(QColor::HexRgb);
    }

    if (!m_pOverrideBackgroundGradient) {
        return {};
    }

    return gradientToString(*m_pOverrideBackgroundGradient);
}

QString SidePanelController::gradientToString(
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

QLinearGradient SidePanelController::lighterGradient(
    const QLinearGradient & gradient) const
{
    QLinearGradient result(gradient.start(), gradient.finalStop());
    auto stops = gradient.stops();
    for(const auto & stop: stops) {
        result.setColorAt(stop.first, stop.second.lighter(150));
    }
    return result;
}

QLinearGradient SidePanelController::darkerGradient(const QLinearGradient & gradient) const
{
    QLinearGradient result(gradient.start(), gradient.finalStop());
    auto stops = gradient.stops();
    for(const auto & stop: stops) {
        result.setColorAt(stop.first, stop.second.darker(200));
    }
    return result;
}

} // namespace quentier
