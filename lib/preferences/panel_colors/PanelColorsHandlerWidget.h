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

#ifndef QUENTIER_PREFERENCES_PANEL_COLORS_HANDLER_WIDGET_H
#define QUENTIER_PREFERENCES_PANEL_COLORS_HANDLER_WIDGET_H

#include <quentier/types/Account.h>

#include <QColor>
#include <QLinearGradient>
#include <QPointer>
#include <QWidget>

#include <vector>

namespace Ui {
class PanelColorsHandlerWidget;
}

QT_FORWARD_DECLARE_CLASS(QColorDialog)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace quentier {

class PanelColorsHandlerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PanelColorsHandlerWidget(QWidget * parent = nullptr);
    virtual ~PanelColorsHandlerWidget() override;

    void initialize(const Account & account);

Q_SIGNALS:
    void fontColorChanged(QColor color);
    void backgroundColorChanged(QColor color);
    void useBackgroundGradientSettingChanged(bool useBackgroundGradient);
    void backgroundLinearGradientChanged(QLinearGradient gradient);

private Q_SLOTS:
    void onFontColorEntered();
    void onFontColorDialogRequested();
    void onFontColorSelected(const QColor & color);

    void onBackgroundColorEntered();
    void onBackgroundColorDialogRequested();
    void onBackgroundColorSelected(const QColor & color);

    void onUseBackgroundGradientRadioButtonToggled();
    void onUseBackgroundColorRadioButtonToggled();

    void onBackgroundGradientBaseColorEntered();
    void onBackgroundGradientBaseColorDialogRequested();
    void onBackgroundGradientBaseColorSelected(const QColor & color);

private:
    void restoreAccountSettings();

    QColor fontColor();
    QColor backgroundColor();
    QColor backgroundGradientBaseColor();
    bool useBackgroundGradient();

    QColor colorFromSettingsImpl(
        const QString & settingName,
        Qt::GlobalColor defaultColor);

    bool onColorEnteredImpl(
        QColor color,
        QColor prevColor,
        const QString & settingName,
        QLineEdit & colorLineEdit,
        QFrame & colorDemoFrame);

    void saveFontColor(const QColor & color);
    void saveBackgroundColor(const QColor & color);
    void saveBackgroundGradientBaseColor(const QColor & color);
    void saveColorImpl(const QColor & color, const QString & settingName);

    void saveUseBackgroundGradientSetting(bool useBackgroundGradient);

    void setBackgroundColorToDemoFrame(const QColor & color, QFrame & frame);

private:
    Ui::PanelColorsHandlerWidget *  m_pUi;
    Account     m_currentAccount;

    QPointer<QColorDialog>      m_pFontColorDialog;
    QPointer<QColorDialog>      m_pBackgroundColorDialog;
    QPointer<QColorDialog>      m_pBackgroundGradientBaseColorDialog;

    std::vector<QPointer<QColorDialog>>     m_backgroundGradientColorDialogs;
};

} // namespace quentier

#endif // QUENTIER_PREFERENCES_PANEL_COLORS_HANDLER_WIDGET_H
