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

#include <quentier/types/Account.h>

#include <QColor>
#include <QLinearGradient>
#include <QPointer>
#include <QWidget>

#include <vector>

namespace Ui {
class PanelColorsHandlerWidget;
} // namespace Ui

class QColorDialog;
class QFrame;
class QLineEdit;

namespace quentier {

class PanelColorsHandlerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PanelColorsHandlerWidget(QWidget * parent = nullptr);
    ~PanelColorsHandlerWidget() override;

    void initialize(const Account & account);

Q_SIGNALS:
    void fontColorChanged(QColor color);
    void backgroundColorChanged(QColor color);
    void useBackgroundGradientSettingChanged(bool useBackgroundGradient);
    void backgroundLinearGradientChanged(QLinearGradient gradient);

    void notifyUserError(QString message);

private Q_SLOTS:
    void onFontColorEntered();
    void onFontColorDialogRequested();
    void onFontColorSelected(const QColor & color);

    void onBackgroundColorEntered();
    void onBackgroundColorDialogRequested();
    void onBackgroundColorSelected(const QColor & color);

    void onUseBackgroundGradientRadioButtonToggled(bool checked);
    void onUseBackgroundColorRadioButtonToggled(bool checked);

    void onBackgroundGradientBaseColorEntered();
    void onBackgroundGradientBaseColorDialogRequested();
    void onBackgroundGradientBaseColorSelected(const QColor & color);

    void onBackgroundGradientTableWidgetRowValueEdited(double value);
    void onBackgroundGradientTableWidgetRowColorEntered();
    void onBackgroundGradientTableWidgetRowColorSelected(const QColor & color);
    void onBackgroundGradientTableWidgetRowColorDialogRequested();

    void onGenerateButtonPressed();

    void onAddRowButtonPressed();
    void onRemoveRowButtonPressed();

private:
    bool eventFilter(QObject * pObject, QEvent * pEvent) override;

private:
    struct GradientLine
    {
        GradientLine(double value, QString colorName) :
            m_value{value}, m_color{std::move(colorName)}
        {}

        double m_value = 0.0;
        QColor m_color;
    };

    void restoreAccountSettings();

    void createConnections();

    void setupBackgroundGradientTableWidget();
    void setupBackgroundGradientTableWidgetRow(
        const GradientLine & gradientLine, const int rowIndex);

    void setNamesToBackgroundGradientTableWidgetRow(const int rowIndex);

    void installEventFilters();

    void openColorDialogForBackgroundGradientTableWidgetRow(int rowIndex);
    void notifyBackgroundGradientUpdated();
    void updateBackgroundGradientDemoFrameStyleSheet();
    void handleBackgroundGradientLinesUpdated();

    [[nodiscard]] QColor fontColor();
    [[nodiscard]] QColor backgroundColor();
    [[nodiscard]] QColor backgroundGradientBaseColor();
    [[nodiscard]] bool useBackgroundGradient();

    [[nodiscard]] QColor colorFromSettingsImpl(
        const char * key, Qt::GlobalColor defaultColor);

    [[nodiscard]] bool onColorEnteredImpl(
        QColor color, QColor prevColor, const char * key,
        QLineEdit & colorLineEdit, QFrame & colorDemoFrame);

    void onUseBackgroundGradientOptionChanged(bool enabled);

    void saveFontColor(const QColor & color);
    void saveBackgroundColor(const QColor & color);
    void saveBackgroundGradientBaseColor(const QColor & color);
    void saveUseBackgroundGradientSetting(bool useBackgroundGradient);
    void saveSettingImpl(const QVariant & value, const char * key);

    void saveBackgroundGradientLinesToSettings();

    void setBackgroundColorToDemoFrame(const QColor & color, QFrame & frame);

private:
    Ui::PanelColorsHandlerWidget * m_pUi;
    Account m_currentAccount;

    QPointer<QColorDialog> m_pFontColorDialog;
    QPointer<QColorDialog> m_pBackgroundColorDialog;
    QPointer<QColorDialog> m_pBackgroundGradientBaseColorDialog;

    std::vector<QPointer<QColorDialog>> m_backgroundGradientColorDialogs;
    std::vector<GradientLine> m_backgroundGradientLines;
};

} // namespace quentier
