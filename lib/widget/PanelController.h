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

#ifndef QUENTIER_WIDGETS_PANEL_CONTROLLER_H
#define QUENTIER_WIDGETS_PANEL_CONTROLLER_H

#include <QColor>
#include <QLinearGradient>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace quentier {

/**
 * @brief The PanelController class manages QFrame with child widgets which
 * as a whole represent one of panels used in the UI of Quentier app
 */
class PanelController
{
public:
    /**
     * @brief PanelController contructor
     * @param pPanel        QFrame representing the panel; certain layout of
     *                      child widgets is expected.
     */
    explicit PanelController(QFrame * pPanel);

    QString title() const;
    void setTitle(const QString & title);

    QColor overrideFontColor() const;
    void setOverrideFontColor(QColor color);

    QColor overrideBackgroundColor() const;
    void setOverrideBackgroundColor(QColor color);

    QLinearGradient overrideBackgroundGradient() const;
    void setOverrideBackgroundGradient(QLinearGradient gradient);

    void resetOverrides();

private:
    void findChildWidgets();

    void updateStyleSheet();

    QString backgroundColorToString() const;
    QString gradientToString(const QLinearGradient & gradient) const;

    QLinearGradient lighterGradient(const QLinearGradient & gradient) const;
    QLinearGradient darkerGradient(const QLinearGradient & gradient) const;

private:
    QFrame *        m_pPanel = nullptr;
    QLabel *        m_pTitleLabel = nullptr;
    QPushButton *   m_pStaticIconHolder = nullptr;

    QString         m_defaultStyleSheet;

    QColor          m_overrideFontColor;
    QColor          m_overrideBackgroundColor;
    QLinearGradient m_overrideBackgroundGradient;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_PANEL_CONTROLLER_H
