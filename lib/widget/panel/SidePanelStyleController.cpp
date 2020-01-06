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

#include "SidePanelStyleController.h"

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>

namespace quentier {

SidePanelStyleController::SidePanelStyleController(QFrame * pPanel) :
    PanelStyleController(pPanel)
{
    findChildWidgets();
}

QString SidePanelStyleController::title() const
{
    return m_pTitleLabel->text();
}

void SidePanelStyleController::setTitle(const QString & title)
{
    m_pTitleLabel->setText(title);
}

void SidePanelStyleController::findChildWidgets()
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

QString SidePanelStyleController::generateStyleSheet() const
{
    if (!m_overrideFontColor.isValid() &&
        !m_overrideBackgroundColor.isValid() &&
        !m_pOverrideBackgroundGradient)
    {
        return m_defaultStyleSheet;
    }

    QString styleSheetStr = PanelStyleController::generateStyleSheet();
    QTextStream strm(&styleSheetStr, QIODevice::Append);
    strm << "\n";

    strm << "\n"
        << "QPushButton {\n"
        << "border: none;\n"
        << "padding: 0px;\n"
        << "}\n"
        << "\n"
        << "QPushButton:focus {\n"
        << "outline: none;\n"
        << "}\n\n";

    if (!m_overrideBackgroundColor.isValid() && !m_pOverrideBackgroundGradient) {
        return styleSheetStr;
    }

    strm << "QPushButton:hover:!pressed {\n";
    strm << "background-color: ";
    if (m_overrideBackgroundColor.isValid()) {
        strm << m_overrideBackgroundColor.lighter(150).name()
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
        strm << m_overrideBackgroundColor.darker(200).name()
            << ";\n";
    }
    else {
        Q_ASSERT(m_pOverrideBackgroundGradient);
        strm << gradientToString(darkerGradient(*m_pOverrideBackgroundGradient))
            << ";\n";
    }
    strm << "}\n";

    strm << "\n";

    auto backgroundColorStr = backgroundColorToString();

    strm << "QPushButton#" << m_pStaticIconHolder->objectName() << ":hover{\n"
        << "background-color: " << backgroundColorStr << ";\n"
        << "}\n";

    strm << "\n";

    strm << "QPushButton#" << m_pStaticIconHolder->objectName() << ":pressed{\n"
        << "background-color: " << backgroundColorStr << ";\n"
        << "}\n";

    return styleSheetStr;
}

} // namespace quentier
