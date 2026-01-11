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

#include "SidePanelStyleController.h"

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>

#include <utility>

namespace quentier {

SidePanelStyleController::SidePanelStyleController(QFrame * panel) :
    PanelStyleController{panel}
{
    findChildWidgets();
}

QString SidePanelStyleController::title() const
{
    return m_titleLabel->text();
}

void SidePanelStyleController::setTitle(const QString & title)
{
    m_titleLabel->setText(title);
}

void SidePanelStyleController::findChildWidgets()
{
    const auto staticIconHolders = m_panel->findChildren<QPushButton *>(
        QRegularExpression{QStringLiteral("(.*)PanelIconPseudoPushButton")});
    Q_ASSERT(staticIconHolders.size() == 1);
    m_staticIconHolder = staticIconHolders[0];
    Q_ASSERT(m_staticIconHolder);

    const auto labels = m_panel->findChildren<QLabel *>();
    Q_ASSERT(labels.size() == 2);
    for (const auto label: std::as_const(labels)) {
        if (label->objectName().endsWith(QStringLiteral("LeftPaddingLabel"))) {
            continue;
        }

        m_titleLabel = label;
        break;
    }

    Q_ASSERT(m_titleLabel);
}

QString SidePanelStyleController::generateStyleSheet() const
{
    if (!m_overrideFontColor.isValid() &&
        !m_overrideBackgroundColor.isValid() && !m_overrideBackgroundGradient)
    {
        return m_defaultStyleSheet;
    }

    QString styleSheetStr = PanelStyleController::generateStyleSheet();
    QTextStream strm{&styleSheetStr, QIODevice::Append};
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

    if (!m_overrideBackgroundColor.isValid() && !m_overrideBackgroundGradient) {
        return styleSheetStr;
    }

    strm << "QPushButton:hover:!pressed {\n";
    strm << "background-color: ";
    if (m_overrideBackgroundColor.isValid()) {
        strm << m_overrideBackgroundColor.lighter(150).name() << ";\n";
    }
    else {
        Q_ASSERT(m_overrideBackgroundGradient);
        strm << gradientToString(lighterGradient(*m_overrideBackgroundGradient))
             << ";\n";
    }
    strm << "}\n";

    strm << "\n";

    strm << "QPushButton:pressed {\n";
    strm << "background-color: ";
    if (m_overrideBackgroundColor.isValid()) {
        strm << m_overrideBackgroundColor.darker(200).name() << ";\n";
    }
    else {
        Q_ASSERT(m_overrideBackgroundGradient);
        strm << gradientToString(darkerGradient(*m_overrideBackgroundGradient))
             << ";\n";
    }
    strm << "}\n";

    strm << "\n";

    const auto backgroundColorStr = backgroundColorToString();

    strm << "QPushButton#" << m_staticIconHolder->objectName() << ":hover{\n"
         << "background-color: " << backgroundColorStr << ";\n"
         << "}\n";

    strm << "\n";

    strm << "QPushButton#" << m_staticIconHolder->objectName() << ":pressed{\n"
         << "background-color: " << backgroundColorStr << ";\n"
         << "}\n";

    return styleSheetStr;
}

} // namespace quentier
