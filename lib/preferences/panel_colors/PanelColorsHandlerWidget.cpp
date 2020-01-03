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

#include "PanelColorsHandlerWidget.h"
#include "ui_PanelColorsHandlerWidget.h"
#include "../SettingsNames.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QColorDialog>
#include <QDoubleSpinBox>

#include <memory>

namespace quentier {

PanelColorsHandlerWidget::PanelColorsHandlerWidget(QWidget *parent) :
    QWidget(parent),
    m_pUi(new Ui::PanelColorsHandlerWidget)
{
    m_pUi->setupUi(this);
}

PanelColorsHandlerWidget::~PanelColorsHandlerWidget()
{
    delete m_pUi;
}

void PanelColorsHandlerWidget::initialize(const Account & account)
{
    QNDEBUG("PanelColorsHandlerWidget::initialize: " << account);

    if (m_currentAccount == account) {
        QNDEBUG("Already initialized for this account, nothing to do");
        return;
    }

    m_currentAccount = account;
    restoreAccountSettings();
}

void PanelColorsHandlerWidget::onFontColorEntered()
{
    QString colorCode = m_pUi->fontColorLineEdit->text();

    QNDEBUG("PanelColorsHandlerWidget::onFontColorEntered: " << colorCode);

    QColor prevColor = fontColor();

    QColor color(colorCode);
    bool res = onColorEnteredImpl(
        color,
        prevColor,
        PANEL_COLORS_FONT_COLOR_SETTINGS_KEY,
        *m_pUi->fontColorLineEdit,
        *m_pUi->fontColorDemoFrame);
    if (!res) {
        return;
    }

    Q_EMIT fontColorChanged(color);
}

void PanelColorsHandlerWidget::onFontColorDialogRequested()
{
    QNDEBUG("PanelColorsHandlerWidget::onFontColorDialogRequested");

    if (!m_pFontColorDialog.isNull()) {
        QNDEBUG("Dialog is already opened");
        return;
    }

    auto pDialog = std::make_unique<QColorDialog>(this);
    pDialog->setWindowModality(Qt::WindowModal);
    pDialog->setCurrentColor(fontColor());

    m_pFontColorDialog = pDialog.get();
    QObject::connect(
        pDialog.get(),
        &QColorDialog::colorSelected,
        this,
        &PanelColorsHandlerWidget::onFontColorSelected);
    Q_UNUSED(pDialog->exec())
}

void PanelColorsHandlerWidget::onFontColorSelected(const QColor & color)
{
    QNDEBUG("PanelColorsHandlerWidget::onFontColorSelected: " << color.name());

    m_pUi->fontColorLineEdit->setText(color.name());
    setBackgroundColorToDemoFrame(color, *m_pUi->fontColorDemoFrame);

    QColor prevColor = fontColor();
    if (prevColor.isValid() && color.isValid() &&
        (color.name() == prevColor.name()))
    {
        QNDEBUG("Font color did not change");
        return;
    }

    saveFontColor(color);
    Q_EMIT fontColorChanged(color);
}

void PanelColorsHandlerWidget::onBackgroundColorEntered()
{
    QString colorCode = m_pUi->backgroundColorLineEdit->text();

    QNDEBUG("PanelColorsHandlerWidget::onBackgroundColorEntered: " << colorCode);

    QColor prevColor = backgroundColor();

    QColor color(colorCode);
    bool res = onColorEnteredImpl(
        color,
        prevColor,
        PANEL_COLORS_BACKGROUND_COLOR_SETTINGS_KEY,
        *m_pUi->backgroundColorLineEdit,
        *m_pUi->backgroundColorDemoFrame);
    if (!res) {
        return;
    }

    Q_EMIT backgroundColorChanged(color);
}

void PanelColorsHandlerWidget::onBackgroundColorDialogRequested()
{
    QNDEBUG("PanelColorsHandlerWidget::onBackgroundColorDialogRequested");

    if (!m_pBackgroundColorDialog.isNull()) {
        QNDEBUG("Dialog is already opened");
        return;
    }

    auto pDialog = std::make_unique<QColorDialog>(this);
    pDialog->setWindowModality(Qt::WindowModal);
    pDialog->setCurrentColor(backgroundColor());

    m_pBackgroundColorDialog = pDialog.get();
    QObject::connect(
        pDialog.get(),
        &QColorDialog::colorSelected,
        this,
        &PanelColorsHandlerWidget::onBackgroundColorSelected);
    Q_UNUSED(pDialog->exec())
}

void PanelColorsHandlerWidget::onBackgroundColorSelected(const QColor & color)
{
    QNDEBUG("PanelColorsHandlerWidget::onBackgroundColorSelected: " << color.name());

    m_pUi->backgroundColorLineEdit->setText(color.name());
    setBackgroundColorToDemoFrame(color, *m_pUi->backgroundColorDemoFrame);

    QColor prevColor = backgroundColor();
    if (prevColor.isValid() && color.isValid() &&
        (color.name() == prevColor.name()))
    {
        QNDEBUG("Background color did not change");
        return;
    }

    saveBackgroundColor(color);
    Q_EMIT backgroundColorChanged(color);
}

void PanelColorsHandlerWidget::onUseBackgroundGradientRadioButtonToggled()
{
    QNDEBUG("PanelColorsHandlerWidget::onUseBackgroundGradientRadioButtonToggled");
    onUseBackgroundGradientOptionChanged(
        m_pUi->useBackgroundGradientRadioButton->isChecked());
}

void PanelColorsHandlerWidget::onUseBackgroundColorRadioButtonToggled()
{
    QNDEBUG("PanelColorsHandlerWidget::onUseBackgroundColorRadioButtonToggled");
    onUseBackgroundGradientOptionChanged(
        !m_pUi->useBackgroundColorRadioButton->isChecked());
}

void PanelColorsHandlerWidget::onBackgroundGradientBaseColorEntered()
{
    QString colorCode = m_pUi->backgroundGradientBaseColorLineEdit->text();

    QNDEBUG("PanelColorsHandlerWidget::onBackgroundGradientBaseColorEntered: "
        << colorCode);

    QColor prevColor = backgroundGradientBaseColor();

    QColor color(colorCode);
    Q_UNUSED(onColorEnteredImpl(
        color,
        prevColor,
        PANEL_COLORS_BACKGROUND_GRADIENT_BASE_COLOR_SETTINGS_KEY,
        *m_pUi->backgroundGradientBaseColorLineEdit,
                 *m_pUi->backgroundGradientBaseColorDemoFrame))
}

void PanelColorsHandlerWidget::onBackgroundGradientBaseColorDialogRequested()
{
    QNDEBUG("PanelColorsHandlerWidget::onBackgroundGradientBaseColorDialogRequested");

    if (!m_pBackgroundGradientBaseColorDialog.isNull()) {
        QNDEBUG("Dialog is already opened");
        return;
    }

    auto pDialog = std::make_unique<QColorDialog>(this);
    pDialog->setWindowModality(Qt::WindowModal);
    pDialog->setCurrentColor(backgroundGradientBaseColor());

    m_pBackgroundGradientBaseColorDialog = pDialog.get();
    QObject::connect(
        pDialog.get(),
        &QColorDialog::colorSelected,
        this,
        &PanelColorsHandlerWidget::onBackgroundGradientBaseColorSelected);
    Q_UNUSED(pDialog->exec())
}

void PanelColorsHandlerWidget::onBackgroundGradientBaseColorSelected(
    const QColor & color)
{
    QNDEBUG("PanelColorsHandlerWidget::onBackgroundGradientBaseColorSelected: "
        << color.name());

    m_pUi->backgroundGradientBaseColorLineEdit->setText(color.name());
    setBackgroundColorToDemoFrame(color, *m_pUi->backgroundColorDemoFrame);

    QColor prevColor = backgroundGradientBaseColor();
    if (prevColor.isValid() && color.isValid() &&
        (color.name() == prevColor.name()))
    {
        QNDEBUG("Background gradient base color did not change");
        return;
    }

    saveBackgroundGradientBaseColor(color);
}

void PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowValueEdited(double value)
{
    // TODO: implement
    Q_UNUSED(value)
}

void PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowColorEntered()
{
    // TODO: implement
}

void PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowColorDialogRequested()
{
    // TODO: implement
}

bool PanelColorsHandlerWidget::eventFilter(QObject * pObject, QEvent * pEvent)
{
    if (pObject == m_pUi->fontColorDemoFrame)
    {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onFontColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->backgroundColorDemoFrame)
    {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onBackgroundColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->backgroundGradientBaseColorDemoFrame)
    {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onBackgroundGradientBaseColorDialogRequested();
            return true;
        }
    }

    if (!pEvent || (pEvent->type() != QEvent::MouseButtonDblClick)) {
        return false;
    }

    int rowCount = m_pUi->backgroundGradientTableWidget->rowCount();
    for(int i = 0; i < rowCount; ++i)
    {
        auto * pDemoFrame = qobject_cast<QFrame*>(
            m_pUi->backgroundGradientTableWidget->cellWidget(i, 2));
        if (pDemoFrame && (pDemoFrame == pObject)) {
            // TODO: request color dialog for this row
            return true;
        }
    }

    return false;
}

void PanelColorsHandlerWidget::restoreAccountSettings()
{
    QNDEBUG("PanelColorsHandlerWidget::restoreAccountSettings");

    if (m_currentAccount.isEmpty()) {
        QNDEBUG("Account is empty, nothing to restore");
        return;
    }

    ApplicationSettings settings(m_currentAccount, QUENTIER_UI_SETTINGS);
    settings.beginGroup(PANEL_COLORS_SETTINGS_GROUP_NAME);

    ApplicationSettings::GroupCloser groupCloser(settings);

    QString fontColorName = settings.value(
        PANEL_COLORS_FONT_COLOR_SETTINGS_KEY).toString();
    QColor fontColor(fontColorName);
    if (!fontColor.isValid()) {
        fontColor = QColor(Qt::white);
    }
    m_pUi->fontColorLineEdit->setText(fontColor.name());
    setBackgroundColorToDemoFrame(fontColor, *m_pUi->fontColorDemoFrame);

    QString backgroundColorName = settings.value(
        PANEL_COLORS_BACKGROUND_COLOR_SETTINGS_KEY).toString();
    QColor backgroundColor(backgroundColorName);
    if (!backgroundColor.isValid()) {
        backgroundColor = QColor(Qt::darkGray);
    }
    m_pUi->backgroundColorLineEdit->setText(backgroundColor.name());
    setBackgroundColorToDemoFrame(
        backgroundColor,
        *m_pUi->backgroundColorDemoFrame);

    bool useBackgroundGradient = settings.value(
        PANEL_COLORS_USE_BACKGROUND_GRADIENT_SETTINGS_KEY).toBool();
    m_pUi->useBackgroundGradientRadioButton->setChecked(useBackgroundGradient);
    m_pUi->useBackgroundColorRadioButton->setChecked(!useBackgroundGradient);

    QString backgroundGradientBaseColorName = settings.value(
        PANEL_COLORS_BACKGROUND_GRADIENT_BASE_COLOR_SETTINGS_KEY).toString();
    QColor backgroundGradientBaseColor(backgroundGradientBaseColorName);
    if (!backgroundGradientBaseColor.isValid()) {
        backgroundGradientBaseColor = QColor(Qt::darkGray);
    }
    m_pUi->backgroundGradientBaseColorLineEdit->setText(
        backgroundGradientBaseColor.name());
    setBackgroundColorToDemoFrame(
        backgroundGradientBaseColor,
        *m_pUi->backgroundGradientBaseColorDemoFrame);

    struct GradientLine
    {
        GradientLine(double value, QString colorName) :
            m_value(value),
            m_color(std::move(colorName))
        {}

        double  m_value = 0.0;
        QColor m_color;
    };

    int numBackgroundGradientLines = settings.beginReadArray(
        PANEL_COLORS_BACKGROUND_GRADIENT_LINES_SETTINGS_KEY);
    ApplicationSettings::ArrayCloser arrayCloser(settings);

    std::vector<GradientLine> backgroundGradientLines;
    backgroundGradientLines.reserve(
        static_cast<size_t>(std::max(numBackgroundGradientLines, 0)));
    for(size_t i = 0, size = backgroundGradientLines.size(); i < size; ++i)
    {
        settings.setArrayIndex(static_cast<int>(i));

        bool conversionResult = false;
        double value = settings.value(
            PANEL_COLORS_BACKGROUND_GRADIENT_LINE_VALUE_SETTINGS_KEY).toDouble(
                &conversionResult);
        if (!conversionResult) {
            QNWARNING("Failed to read background gradient lines: failed "
                << "to convert line value to double");
            backgroundGradientLines.clear();
            break;
        }

        QString colorName = settings.value(
            PANEL_COLORS_BACKGROUND_GRADIENT_LINE_COLOR_SETTTINGS_KEY).toString();
        QColor color(colorName);
        if (!color.isValid()) {
            QNWARNING("Failed to read background gradient lines: detected "
                << "line color value not representing a valid color: "
                << colorName);
            backgroundGradientLines.clear();
            break;
        }

        backgroundGradientLines.emplace_back(value, std::move(colorName));
    }

    m_pUi->backgroundGradientTableWidget->setRowCount(
        static_cast<int>(backgroundGradientLines.size()));
    m_pUi->backgroundGradientTableWidget->setColumnCount(4);

    int rowIndex = 0;
    for(const auto & gradientLine: backgroundGradientLines)
    {
        auto rowName = QString::number(rowIndex);

        auto * pValueSpinBox = new QDoubleSpinBox;
        pValueSpinBox->setObjectName(rowName);

        auto * pColorNameLineEdit = new QLineEdit;
        pColorNameLineEdit->setObjectName(rowName);

        auto * pColorDemoFrame = new QFrame;
        pColorDemoFrame->setObjectName(rowName);
        pColorDemoFrame->setMinimumSize(24, 24);
        pColorDemoFrame->setMaximumSize(24, 24);
        pColorDemoFrame->setFrameShape(QFrame::Box);
        pColorDemoFrame->setFrameShadow(QFrame::Plain);
        setBackgroundColorToDemoFrame(gradientLine.m_color, *pColorDemoFrame);

        auto * pColorPushButton = new QPushButton;
        pColorPushButton->setObjectName(rowName);

        m_pUi->backgroundGradientTableWidget->setCellWidget(
            rowIndex,
            0,
            pValueSpinBox);

        m_pUi->backgroundGradientTableWidget->setCellWidget(
            rowIndex,
            1,
            pColorNameLineEdit);

        m_pUi->backgroundGradientTableWidget->setCellWidget(
            rowIndex,
            2,
            pColorDemoFrame);

        m_pUi->backgroundGradientTableWidget->setCellWidget(
            rowIndex,
            3,
            pColorPushButton);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        QObject::connect(
            pValueSpinBox,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowValueEdited);
#else
        QObject::connect(
            pValueSpinBox,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(onBackgroundGradientTableWidgetRowValueEdited(double)));
#endif

        QObject::connect(
            pColorNameLineEdit,
            &QLineEdit::editingFinished,
            this,
            &PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowColorEntered);

        pColorDemoFrame->installEventFilter(this);

        QObject::connect(
            pColorPushButton,
            &QPushButton::clicked,
            this,
            &PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowColorDialogRequested);

        ++rowIndex;
    }
}

void PanelColorsHandlerWidget::installEventFilters()
{
    m_pUi->fontColorDemoFrame->installEventFilter(this);
    m_pUi->backgroundColorDemoFrame->installEventFilter(this);
    m_pUi->backgroundGradientBaseColorDemoFrame->installEventFilter(this);
}

QColor PanelColorsHandlerWidget::fontColor()
{
    return colorFromSettingsImpl(
        PANEL_COLORS_FONT_COLOR_SETTINGS_KEY,
        Qt::white);
}

QColor PanelColorsHandlerWidget::backgroundColor()
{
    return colorFromSettingsImpl(
        PANEL_COLORS_BACKGROUND_COLOR_SETTINGS_KEY,
        Qt::darkGray);
}

QColor PanelColorsHandlerWidget::backgroundGradientBaseColor()
{
    return colorFromSettingsImpl(
        PANEL_COLORS_BACKGROUND_GRADIENT_BASE_COLOR_SETTINGS_KEY,
        Qt::darkGray);
}

bool PanelColorsHandlerWidget::useBackgroundGradient()
{
    ApplicationSettings settings(m_currentAccount, QUENTIER_UI_SETTINGS);
    settings.beginGroup(PANEL_COLORS_SETTINGS_GROUP_NAME);
    bool useBackgroundGradient = settings.value(
        PANEL_COLORS_USE_BACKGROUND_GRADIENT_SETTINGS_KEY).toBool();
    settings.endGroup();

    return useBackgroundGradient;
}

QColor PanelColorsHandlerWidget::colorFromSettingsImpl(
    const QString & settingName,
    Qt::GlobalColor defaultColor)
{
    ApplicationSettings settings(m_currentAccount, QUENTIER_UI_SETTINGS);
    settings.beginGroup(PANEL_COLORS_SETTINGS_GROUP_NAME);
    QString colorName = settings.value(settingName).toString();
    settings.endGroup();

    QColor color(colorName);
    if (!color.isValid()) {
        color = QColor(defaultColor);
    }

    return color;
}

bool PanelColorsHandlerWidget::onColorEnteredImpl(
    QColor color,
    QColor prevColor,
    const QString & settingName,
    QLineEdit & colorLineEdit,
    QFrame & colorDemoFrame)
{
    if (!color.isValid()) {
        colorLineEdit.setText(prevColor.name());
        return false;
    }

    if (prevColor.isValid() && (prevColor.name() == color.name())) {
        QNDEBUG("Color did not change");
        return false;
    }

    saveSettingImpl(color.name(), settingName);
    setBackgroundColorToDemoFrame(color, colorDemoFrame);
    return true;
}

void PanelColorsHandlerWidget::onUseBackgroundGradientOptionChanged(bool enabled)
{
    if (enabled == useBackgroundGradient()) {
        QNDEBUG("Option hasn't changed");
        return;
    }

    saveUseBackgroundGradientSetting(enabled);
    Q_EMIT useBackgroundGradientSettingChanged(enabled);
}

void PanelColorsHandlerWidget::saveFontColor(const QColor & color)
{
    saveSettingImpl(color.name(), PANEL_COLORS_FONT_COLOR_SETTINGS_KEY);
}

void PanelColorsHandlerWidget::saveBackgroundColor(const QColor & color)
{
    saveSettingImpl(color.name(), PANEL_COLORS_BACKGROUND_COLOR_SETTINGS_KEY);
}

void PanelColorsHandlerWidget::saveBackgroundGradientBaseColor(
    const QColor & color)
{
    saveSettingImpl(
        color.name(),
        PANEL_COLORS_BACKGROUND_GRADIENT_BASE_COLOR_SETTINGS_KEY);
}

void PanelColorsHandlerWidget::saveUseBackgroundGradientSetting(
    bool useBackgroundGradient)
{
    saveSettingImpl(
        useBackgroundGradient,
        PANEL_COLORS_USE_BACKGROUND_GRADIENT_SETTINGS_KEY);
}

void PanelColorsHandlerWidget::saveSettingImpl(
    const QVariant & value, const QString & settingName)
{
    ApplicationSettings settings(m_currentAccount, QUENTIER_UI_SETTINGS);
    settings.beginGroup(PANEL_COLORS_SETTINGS_GROUP_NAME);
    settings.setValue(settingName, value);
    settings.endGroup();
}

void PanelColorsHandlerWidget::setBackgroundColorToDemoFrame(
    const QColor & color, QFrame & frame)
{
    QPalette pal = frame.palette();
    pal.setColor(QPalette::Background, color);
    frame.setPalette(pal);
}

} // namespace quentier
