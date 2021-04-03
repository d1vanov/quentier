/*
 * Copyright 2019-2021 Dmitry Ivanov
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

#include <lib/preferences/keys/Files.h>
#include <lib/preferences/keys/PanelColors.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QTextStream>

#include <iterator>
#include <memory>

namespace quentier {

PanelColorsHandlerWidget::PanelColorsHandlerWidget(QWidget * parent) :
    QWidget(parent), m_pUi(new Ui::PanelColorsHandlerWidget)
{
    m_pUi->setupUi(this);
}

PanelColorsHandlerWidget::~PanelColorsHandlerWidget()
{
    delete m_pUi;
}

void PanelColorsHandlerWidget::initialize(const Account & account)
{
    QNDEBUG("preferences", "PanelColorsHandlerWidget::initialize: " << account);

    if (m_currentAccount == account) {
        QNDEBUG(
            "preferences",
            "Already initialized for this account, nothing to do");
        return;
    }

    m_currentAccount = account;
    restoreAccountSettings();
    setupBackgroundGradientTableWidget();
    updateBackgroundGradientDemoFrameStyleSheet();

    createConnections();
    installEventFilters();
}

void PanelColorsHandlerWidget::onFontColorEntered()
{
    const QString colorCode = m_pUi->fontColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::onFontColorEntered: " << colorCode);

    const QColor prevColor = fontColor();
    const QColor color{colorCode};

    const bool res = onColorEnteredImpl(
        color, prevColor, preferences::keys::panelFontColor,
        *m_pUi->fontColorLineEdit, *m_pUi->fontColorDemoFrame);

    if (!res) {
        return;
    }

    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT fontColorChanged(color);
}

void PanelColorsHandlerWidget::onFontColorDialogRequested()
{
    QNDEBUG(
        "preferences", "PanelColorsHandlerWidget::onFontColorDialogRequested");

    if (!m_pFontColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    const auto pDialog = std::make_unique<QColorDialog>(this);
    pDialog->setWindowModality(Qt::WindowModal);
    pDialog->setCurrentColor(fontColor());

    m_pFontColorDialog = pDialog.get();

    QObject::connect(
        pDialog.get(), &QColorDialog::colorSelected, this,
        &PanelColorsHandlerWidget::onFontColorSelected);

    Q_UNUSED(pDialog->exec())
}

void PanelColorsHandlerWidget::onFontColorSelected(const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::onFontColorSelected: " << color.name());

    m_pUi->fontColorLineEdit->setText(color.name());
    setBackgroundColorToDemoFrame(color, *m_pUi->fontColorDemoFrame);

    const QColor prevColor = fontColor();
    if (prevColor.isValid() && color.isValid() &&
        (color.name() == prevColor.name()))
    {
        QNDEBUG("preferences", "Font color did not change");
        return;
    }

    saveFontColor(color);
    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT fontColorChanged(color);
}

void PanelColorsHandlerWidget::onBackgroundColorEntered()
{
    const QString colorCode = m_pUi->backgroundColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::onBackgroundColorEntered: " << colorCode);

    const QColor prevColor = backgroundColor();
    const QColor color{colorCode};

    const bool res = onColorEnteredImpl(
        color, prevColor, preferences::keys::panelBackgroundColor,
        *m_pUi->backgroundColorLineEdit, *m_pUi->backgroundColorDemoFrame);

    if (!res) {
        return;
    }

    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT backgroundColorChanged(color);
}

void PanelColorsHandlerWidget::onBackgroundColorDialogRequested()
{
    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::onBackgroundColorDialogRequested");

    if (!m_pBackgroundColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    const auto pDialog = std::make_unique<QColorDialog>(this);
    pDialog->setWindowModality(Qt::WindowModal);
    pDialog->setCurrentColor(backgroundColor());

    m_pBackgroundColorDialog = pDialog.get();

    QObject::connect(
        pDialog.get(), &QColorDialog::colorSelected, this,
        &PanelColorsHandlerWidget::onBackgroundColorSelected);

    Q_UNUSED(pDialog->exec())
}

void PanelColorsHandlerWidget::onBackgroundColorSelected(const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::onBackgroundColorSelected: "
            << color.name());

    m_pUi->backgroundColorLineEdit->setText(color.name());
    setBackgroundColorToDemoFrame(color, *m_pUi->backgroundColorDemoFrame);

    const QColor prevColor = backgroundColor();
    if (prevColor.isValid() && color.isValid() &&
        (color.name() == prevColor.name()))
    {
        QNDEBUG("preferences", "Background color did not change");
        return;
    }

    saveBackgroundColor(color);
    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT backgroundColorChanged(color);
}

void PanelColorsHandlerWidget::onUseBackgroundGradientRadioButtonToggled(
    bool checked)
{
    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::onUseBackgroundGradientRadioButtonToggled: "
            << "checked = " << (checked ? "yes" : "no"));

    onUseBackgroundGradientOptionChanged(checked);
}

void PanelColorsHandlerWidget::onUseBackgroundColorRadioButtonToggled(
    bool checked)
{
    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::onUseBackgroundColorRadioButtonToggled: "
            << "checked = " << (checked ? "yes" : "no"));

    onUseBackgroundGradientOptionChanged(!checked);
}

void PanelColorsHandlerWidget::onBackgroundGradientBaseColorEntered()
{
    const QString colorCode =
        m_pUi->backgroundGradientBaseColorLineEdit->text();

    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::onBackgroundGradientBaseColorEntered: "
            << colorCode);

    const QColor prevColor = backgroundGradientBaseColor();
    const QColor color{colorCode};

    Q_UNUSED(onColorEnteredImpl(
        color, prevColor, preferences::keys::panelBackgroundGradientBaseColor,
        *m_pUi->backgroundGradientBaseColorLineEdit,
        *m_pUi->backgroundGradientBaseColorDemoFrame))
}

void PanelColorsHandlerWidget::onBackgroundGradientBaseColorDialogRequested()
{
    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::"
        "onBackgroundGradientBaseColorDialogRequested");

    if (!m_pBackgroundGradientBaseColorDialog.isNull()) {
        QNDEBUG("preferences", "Dialog is already opened");
        return;
    }

    const auto pDialog = std::make_unique<QColorDialog>(this);
    pDialog->setWindowModality(Qt::WindowModal);
    pDialog->setCurrentColor(backgroundGradientBaseColor());

    m_pBackgroundGradientBaseColorDialog = pDialog.get();

    QObject::connect(
        pDialog.get(), &QColorDialog::colorSelected, this,
        &PanelColorsHandlerWidget::onBackgroundGradientBaseColorSelected);

    Q_UNUSED(pDialog->exec())
}

void PanelColorsHandlerWidget::onBackgroundGradientBaseColorSelected(
    const QColor & color)
{
    QNDEBUG(
        "preferences",
        "PanelColorsHandlerWidget::onBackgroundGradientBaseColorSelected: "
            << color.name());

    m_pUi->backgroundGradientBaseColorLineEdit->setText(color.name());

    setBackgroundColorToDemoFrame(
        color, *m_pUi->backgroundGradientBaseColorDemoFrame);

    const QColor prevColor = backgroundGradientBaseColor();
    if (prevColor.isValid() && color.isValid() &&
        (color.name() == prevColor.name()))
    {
        QNDEBUG("preferences", "Background gradient base color did not change");
        return;
    }

    saveBackgroundGradientBaseColor(color);
}

void PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowValueEdited(
    double value)
{
    if (value < 0.0 || value > 1.0) {
        Q_EMIT notifyUserError(
            tr("Background gradient line value should be between 0 and 1"));
        return;
    }

    auto * pSpinBox = qobject_cast<QDoubleSpinBox *>(sender());
    if (Q_UNLIKELY(!pSpinBox)) {
        QNWARNING(
            "preferences",
            "Received background gradient table widget "
                << "row value edited from unidentified source, value = "
                << value);
        return;
    }

    // Spin box's row within the table widget is encoded within its object name
    bool conversionResult = false;
    const int rowIndex = pSpinBox->objectName().toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(
            "preferences",
            "Failed to identify row upon receiving value edit from background "
                << "gradient table widget, value = "
                << value);
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<size_t>(rowIndex) >= m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences",
            "Internal inconsistency detected: bad row "
                << rowIndex << " upon receiving value edit from background "
                << "gradient table widget, value = " << value);
        return;
    }

    bool needsSorting = false;
    if (rowIndex > 0) {
        const int prevRow = rowIndex - 1;

        const double prevValue =
            m_backgroundGradientLines[static_cast<size_t>(prevRow)].m_value;

        if (value < prevValue) {
            needsSorting = true;
        }
    }

    if (!needsSorting &&
        (rowIndex < static_cast<int>(m_backgroundGradientLines.size() - 1)))
    {
        const int nextRow = rowIndex + 1;

        const double nextValue =
            m_backgroundGradientLines[static_cast<size_t>(nextRow)].m_value;

        if (value > nextValue) {
            needsSorting = true;
        }
    }

    m_backgroundGradientLines[static_cast<size_t>(rowIndex)].m_value = value;

    if (!needsSorting) {
        handleBackgroundGradientLinesUpdated();
        return;
    }

    std::sort(
        m_backgroundGradientLines.begin(), m_backgroundGradientLines.end(),
        [](const GradientLine & lhs, const GradientLine & rhs) {
            return lhs.m_value < rhs.m_value;
        });

    setupBackgroundGradientTableWidget();
    handleBackgroundGradientLinesUpdated();
}

void PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowColorEntered()
{
    auto * pLineEdit = qobject_cast<QLineEdit *>(sender());
    if (Q_UNLIKELY(!pLineEdit)) {
        QNWARNING(
            "preferences",
            "Received background gradient table widget "
                << "row color name edited from unidentified source");
        return;
    }

    // Line edit's row within the table widget is encoded within its object name
    bool conversionResult = false;
    const int rowIndex = pLineEdit->objectName().toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(
            "preferences",
            "Failed to identify row upon receiving color "
                << "name edit from background gradient table widget, text = "
                << pLineEdit->text());
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<size_t>(rowIndex) >= m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences",
            "Internal inconsistency detected: bad row "
                << rowIndex
                << " upon receiving color name edit from background "
                << "gradient table widget, text = " << pLineEdit->text());
        return;
    }

    const QString colorName = pLineEdit->text();
    auto & line = m_backgroundGradientLines[static_cast<size_t>(rowIndex)];
    const QColor prevColor = line.m_color;
    line.m_color.setNamedColor(colorName);
    if (!line.m_color.isValid()) {
        line.m_color = prevColor;

        QNINFO(
            "preferences",
            "Rejecting color input for background gradient "
                << "table row: " << colorName << " is not a valid color name");

        Q_EMIT notifyUserError(
            tr("Rejected input color name as it is not valid"));
        return;
    }

    auto * pDemoFrameWidget =
        m_pUi->backgroundGradientTableWidget->cellWidget(rowIndex, 2);

    if (pDemoFrameWidget) {
        auto * pDemoFrame = pDemoFrameWidget->findChild<QFrame *>();
        if (pDemoFrame) {
            setBackgroundColorToDemoFrame(line.m_color, *pDemoFrame);
        }
    }

    handleBackgroundGradientLinesUpdated();
}

void PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowColorSelected(
    const QColor & color)
{
    auto * pDialog = qobject_cast<QColorDialog *>(sender());
    if (Q_UNLIKELY(!pDialog)) {
        QNWARNING(
            "preferences",
            "Received background gradient table widget "
                << "selected color from unidentified source, color = "
                << color.name());
        return;
    }

    // QColorDialog's target row within the table widget is encoded within
    // its object name
    bool conversionResult = false;
    const int rowIndex = pDialog->objectName().toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(
            "preferences",
            "Failed to identify row upon receiving "
                << "selected color from QColorDialog for background gradient "
                   "table "
                << "widget, color = " << color.name());
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<size_t>(rowIndex) >= m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences",
            "Internal inconsistency detected: bad row "
                << rowIndex << " upon receiving selected color for background "
                << "gradient table widget, color = " << color.name());
        return;
    }

    m_backgroundGradientLines[static_cast<size_t>(rowIndex)].m_color = color;

    auto * pLineEdit = qobject_cast<QLineEdit *>(
        m_pUi->backgroundGradientTableWidget->cellWidget(rowIndex, 1));

    if (pLineEdit) {
        pLineEdit->setText(color.name());
    }

    auto * pDemoFrameWidget =
        m_pUi->backgroundGradientTableWidget->cellWidget(rowIndex, 2);

    if (pDemoFrameWidget) {
        auto * pDemoFrame = pDemoFrameWidget->findChild<QFrame *>();
        if (pDemoFrame) {
            setBackgroundColorToDemoFrame(color, *pDemoFrame);
        }
    }

    handleBackgroundGradientLinesUpdated();
}

void PanelColorsHandlerWidget::
    onBackgroundGradientTableWidgetRowColorDialogRequested()
{
    auto * pButton = qobject_cast<QPushButton *>(sender());
    if (Q_UNLIKELY(!pButton)) {
        QNWARNING(
            "preferences",
            "Received background gradient table widget "
                << "row color dialog request from unidentified source");
        return;
    }

    // Button's row within the table widget is encoded within its object name
    bool conversionResult = false;
    const int rowIndex = pButton->objectName().toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(
            "preferences",
            "Failed to identify row upon receiving choose "
                << "color button click from background gradient table widget");
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<size_t>(rowIndex) >= m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences",
            "Internal inconsistency detected: bad row "
                << rowIndex << " upon receiving choose color button click from "
                << "background gradient table widget");
        return;
    }

    openColorDialogForBackgroundGradientTableWidgetRow(rowIndex);
}

void PanelColorsHandlerWidget::onGenerateButtonPressed()
{
    QNDEBUG("preferences", "PanelColorsHandlerWidget::onGenerateButtonPressed");

    const QColor baseColor{m_pUi->backgroundGradientBaseColorLineEdit->text()};
    if (Q_UNLIKELY(!baseColor.isValid())) {
        Q_EMIT notifyUserError(
            tr("Can't generate background gradient colors: no valid base "
               "color"));
        return;
    }

    std::vector<GradientLine> gradientLines;
    gradientLines.reserve(5);

    gradientLines.emplace_back(0.0, baseColor.lighter(150).name());
    gradientLines.emplace_back(0.2, baseColor.lighter(110).name());
    gradientLines.emplace_back(0.45, baseColor.name());
    gradientLines.emplace_back(0.8, baseColor.darker(150).name());
    gradientLines.emplace_back(1.0, baseColor.darker(200).name());

    m_backgroundGradientLines = std::move(gradientLines);

    setupBackgroundGradientTableWidget();
    handleBackgroundGradientLinesUpdated();
}

void PanelColorsHandlerWidget::onAddRowButtonPressed()
{
    const int rowIndex = m_pUi->backgroundGradientTableWidget->rowCount();
    m_pUi->backgroundGradientTableWidget->insertRow(rowIndex);

    GradientLine line{1.0, QColor(Qt::black).name()};
    setupBackgroundGradientTableWidgetRow(line, rowIndex);
    m_backgroundGradientLines.emplace_back(std::move(line));
    handleBackgroundGradientLinesUpdated();
}

void PanelColorsHandlerWidget::onRemoveRowButtonPressed()
{
    auto * pButton = qobject_cast<QPushButton *>(sender());
    if (Q_UNLIKELY(!pButton)) {
        QNWARNING(
            "preferences",
            "Received row removal request from unidentified source");
        return;
    }

    // Push button's row within the table widget is encoded within its object
    // name
    bool conversionResult = false;
    const int rowIndex = pButton->objectName().toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(
            "preferences",
            "Failed to identify row upon receiving row removal request from "
                << "background gradient table widget");
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<size_t>(rowIndex) >= m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences",
            "Internal inconsistency detected: bad row "
                << rowIndex << " upon receiving row removal request from "
                << "background gradient table widget, row = " << rowIndex);
        return;
    }

    const bool isLastRow =
        (rowIndex == (m_pUi->backgroundGradientTableWidget->rowCount() - 1));

    m_pUi->backgroundGradientTableWidget->removeRow(rowIndex);

    if (!isLastRow) {
        // Need to assign correct row names to the widgets of all rows past
        // the removed one
        const int rowCount = m_pUi->backgroundGradientTableWidget->rowCount();
        for (int i = rowIndex; i < rowCount; ++i) {
            setNamesToBackgroundGradientTableWidgetRow(i);
        }
    }

    auto it = m_backgroundGradientLines.begin();
    std::advance(
        it,
        static_cast<std::iterator_traits<decltype(it)>::difference_type>(
            rowIndex));
    m_backgroundGradientLines.erase(it);

    handleBackgroundGradientLinesUpdated();
}

bool PanelColorsHandlerWidget::eventFilter(QObject * pObject, QEvent * pEvent)
{
    if (pObject == m_pUi->fontColorDemoFrame) {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onFontColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->backgroundColorDemoFrame) {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onBackgroundColorDialogRequested();
            return true;
        }
    }
    else if (pObject == m_pUi->backgroundGradientBaseColorDemoFrame) {
        if (pEvent && (pEvent->type() == QEvent::MouseButtonDblClick)) {
            onBackgroundGradientBaseColorDialogRequested();
            return true;
        }
    }

    if (!pEvent || (pEvent->type() != QEvent::MouseButtonDblClick)) {
        return false;
    }

    const int rowCount = m_pUi->backgroundGradientTableWidget->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        auto * pDemoFrameWidget =
            m_pUi->backgroundGradientTableWidget->cellWidget(i, 2);
        if (!pDemoFrameWidget) {
            continue;
        }

        auto * pDemoFrame = pDemoFrameWidget->findChild<QFrame *>();
        if (pDemoFrame && (pDemoFrame == pObject)) {
            openColorDialogForBackgroundGradientTableWidgetRow(i);
            return true;
        }
    }

    return false;
}

void PanelColorsHandlerWidget::restoreAccountSettings()
{
    QNDEBUG("preferences", "PanelColorsHandlerWidget::restoreAccountSettings");

    if (m_currentAccount.isEmpty()) {
        QNDEBUG("preferences", "Account is empty, nothing to restore");
        return;
    }

    ApplicationSettings settings(
        m_currentAccount, preferences::keys::files::userInterface);

    settings.beginGroup(preferences::keys::panelColorsGroup);

    ApplicationSettings::GroupCloser groupCloser(settings);

    const QString fontColorName =
        settings.value(preferences::keys::panelFontColor).toString();

    QColor fontColor{fontColorName};
    if (!fontColor.isValid()) {
        fontColor = QColor(Qt::white);
    }

    m_pUi->fontColorLineEdit->setText(fontColor.name());
    setBackgroundColorToDemoFrame(fontColor, *m_pUi->fontColorDemoFrame);

    const QString backgroundColorName =
        settings.value(preferences::keys::panelBackgroundColor).toString();

    QColor backgroundColor{backgroundColorName};
    if (!backgroundColor.isValid()) {
        backgroundColor = QColor(Qt::darkGray);
    }

    m_pUi->backgroundColorLineEdit->setText(backgroundColor.name());

    setBackgroundColorToDemoFrame(
        backgroundColor, *m_pUi->backgroundColorDemoFrame);

    bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    m_pUi->useBackgroundGradientRadioButton->setChecked(useBackgroundGradient);
    m_pUi->useBackgroundColorRadioButton->setChecked(!useBackgroundGradient);

    const QString backgroundGradientBaseColorName =
        settings.value(preferences::keys::panelBackgroundGradientBaseColor)
            .toString();

    QColor backgroundGradientBaseColor{backgroundGradientBaseColorName};
    if (!backgroundGradientBaseColor.isValid()) {
        backgroundGradientBaseColor = QColor(Qt::darkGray);
    }

    m_pUi->backgroundGradientBaseColorLineEdit->setText(
        backgroundGradientBaseColor.name());

    setBackgroundColorToDemoFrame(
        backgroundGradientBaseColor,
        *m_pUi->backgroundGradientBaseColorDemoFrame);

    const int numBackgroundGradientLines = settings.beginReadArray(
        preferences::keys::panelBackgroundGradientLineCount);

    ApplicationSettings::ArrayCloser arrayCloser(settings);

    m_backgroundGradientLines.clear();

    m_backgroundGradientLines.reserve(
        static_cast<size_t>(std::max(numBackgroundGradientLines, 0)));

    for (size_t i = 0, size = static_cast<size_t>(numBackgroundGradientLines);
         i < size; ++i)
    {
        settings.setArrayIndex(static_cast<int>(i));

        bool conversionResult = false;

        const double value =
            settings.value(preferences::keys::panelBackgroundGradientLineSize)
                .toDouble(&conversionResult);

        if (!conversionResult) {
            QNWARNING(
                "preferences",
                "Failed to read background gradient "
                    << "lines: failed to convert line value to double");
            m_backgroundGradientLines.clear();
            break;
        }

        const QString colorName =
            settings.value(preferences::keys::panelBackgroundGradientLineColor)
                .toString();

        const QColor color{colorName};
        if (!color.isValid()) {
            QNWARNING(
                "preferences",
                "Failed to read background gradient lines: detected line color "
                    << "value not representing a valid color: " << colorName);
            m_backgroundGradientLines.clear();
            break;
        }

        m_backgroundGradientLines.emplace_back(value, colorName);
    }
}

void PanelColorsHandlerWidget::createConnections()
{
    QObject::connect(
        m_pUi->fontColorLineEdit, &QLineEdit::editingFinished, this,
        &PanelColorsHandlerWidget::onFontColorEntered);

    QObject::connect(
        m_pUi->fontColorPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::onFontColorDialogRequested);

    QObject::connect(
        m_pUi->backgroundColorLineEdit, &QLineEdit::editingFinished, this,
        &PanelColorsHandlerWidget::onBackgroundColorEntered);

    QObject::connect(
        m_pUi->backgroundColorPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::onBackgroundColorDialogRequested);

    QObject::connect(
        m_pUi->useBackgroundColorRadioButton, &QRadioButton::toggled, this,
        &PanelColorsHandlerWidget::onUseBackgroundColorRadioButtonToggled);

    QObject::connect(
        m_pUi->useBackgroundGradientRadioButton, &QRadioButton::toggled, this,
        &PanelColorsHandlerWidget::onUseBackgroundGradientRadioButtonToggled);

    QObject::connect(
        m_pUi->backgroundGradientBaseColorLineEdit, &QLineEdit::editingFinished,
        this, &PanelColorsHandlerWidget::onBackgroundGradientBaseColorEntered);

    QObject::connect(
        m_pUi->backgroundGradientBaseColorPushButton, &QPushButton::clicked,
        this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientBaseColorDialogRequested);

    QObject::connect(
        m_pUi->backgroundGradientGeneratePushButton, &QPushButton::clicked,
        this, &PanelColorsHandlerWidget::onGenerateButtonPressed);

    QObject::connect(
        m_pUi->backgroundGradientAddRowPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::onAddRowButtonPressed);
}

void PanelColorsHandlerWidget::setupBackgroundGradientTableWidget()
{
    m_pUi->backgroundGradientTableWidget->clear();

    m_pUi->backgroundGradientTableWidget->setRowCount(
        static_cast<int>(m_backgroundGradientLines.size()));

    m_pUi->backgroundGradientTableWidget->setColumnCount(5);

    m_pUi->backgroundGradientTableWidget->setHorizontalHeaderLabels(
        QStringList() << tr("Value") << tr("Color") << QString() << QString()
                      << QString());

    int rowIndex = 0;
    for (const auto & gradientLine: m_backgroundGradientLines) {
        setupBackgroundGradientTableWidgetRow(gradientLine, rowIndex);
        ++rowIndex;
    }
}

void PanelColorsHandlerWidget::setupBackgroundGradientTableWidgetRow(
    const GradientLine & gradientLine, const int rowIndex)
{
    const auto rowName = QString::number(rowIndex);

    auto * pValueSpinBox = new QDoubleSpinBox;
    pValueSpinBox->setObjectName(rowName);
    pValueSpinBox->setValue(gradientLine.m_value);
    pValueSpinBox->setRange(0.0, 1.0);
    pValueSpinBox->setSingleStep(0.05);

    auto * pColorNameLineEdit = new QLineEdit;
    pColorNameLineEdit->setObjectName(rowName);
    pColorNameLineEdit->setText(gradientLine.m_color.name());

    auto * pColorDemoFrame = new QFrame;
    pColorDemoFrame->setObjectName(rowName);
    pColorDemoFrame->setMinimumSize(24, 24);
    pColorDemoFrame->setMaximumSize(24, 24);
    pColorDemoFrame->setFrameShape(QFrame::Box);
    pColorDemoFrame->setFrameShadow(QFrame::Plain);
    setBackgroundColorToDemoFrame(gradientLine.m_color, *pColorDemoFrame);

    auto * pColorDemoFrameWidget = new QWidget;
    auto * pColorDemoFrameWidgetLayout = new QHBoxLayout(pColorDemoFrameWidget);
    pColorDemoFrameWidgetLayout->addWidget(pColorDemoFrame);
    pColorDemoFrameWidgetLayout->setAlignment(Qt::AlignCenter);
    pColorDemoFrameWidgetLayout->setContentsMargins(0, 0, 0, 0);

    auto * pColorPushButton = new QPushButton;
    pColorPushButton->setObjectName(rowName);
    pColorPushButton->setText(tr("Choose"));
    pColorPushButton->setToolTip(tr("Choose color for row"));

    auto * pRemoveRowPushButton = new QPushButton;
    pRemoveRowPushButton->setObjectName(rowName);

    pRemoveRowPushButton->setIcon(
        QIcon::fromTheme(QStringLiteral("edit-delete")));

    pRemoveRowPushButton->setToolTip(tr("Remove this row"));
    pRemoveRowPushButton->setFlat(true);

    m_pUi->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 0, pValueSpinBox);

    m_pUi->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 1, pColorNameLineEdit);

    m_pUi->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 2, pColorDemoFrameWidget);

    m_pUi->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 3, pColorPushButton);

    m_pUi->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 4, pRemoveRowPushButton);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        pValueSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientTableWidgetRowValueEdited);
#else
    QObject::connect(
        pValueSpinBox, SIGNAL(valueChanged(double)), this,
        SLOT(onBackgroundGradientTableWidgetRowValueEdited(double)));
#endif

    QObject::connect(
        pColorNameLineEdit, &QLineEdit::editingFinished, this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientTableWidgetRowColorEntered);

    pColorDemoFrame->installEventFilter(this);

    QObject::connect(
        pColorPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientTableWidgetRowColorDialogRequested);

    QObject::connect(
        pRemoveRowPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::onRemoveRowButtonPressed);
}

void PanelColorsHandlerWidget::setNamesToBackgroundGradientTableWidgetRow(
    const int rowIndex)
{
    const auto rowName = QString::number(rowIndex);

    auto * pValueSpinBox = qobject_cast<QDoubleSpinBox *>(
        m_pUi->backgroundGradientTableWidget->cellWidget(rowIndex, 0));

    Q_ASSERT(pValueSpinBox);
    pValueSpinBox->setObjectName(rowName);

    auto * pColorNameLineEdit = qobject_cast<QLineEdit *>(
        m_pUi->backgroundGradientTableWidget->cellWidget(rowIndex, 1));

    Q_ASSERT(pColorNameLineEdit);
    pColorNameLineEdit->setObjectName(rowName);

    auto * pColorDemoFrameWidget =
        m_pUi->backgroundGradientTableWidget->cellWidget(rowIndex, 2);

    Q_ASSERT(pColorDemoFrameWidget);

    auto * pColorDemoFrame = pColorDemoFrameWidget->findChild<QFrame *>();
    Q_ASSERT(pColorDemoFrame);
    pColorDemoFrame->setObjectName(rowName);

    auto * pColorPushButton = qobject_cast<QPushButton *>(
        m_pUi->backgroundGradientTableWidget->cellWidget(rowIndex, 3));

    Q_ASSERT(pColorPushButton);
    pColorPushButton->setObjectName(rowName);

    auto * pRemoveRowPushButton = qobject_cast<QPushButton *>(
        m_pUi->backgroundGradientTableWidget->cellWidget(rowIndex, 4));

    Q_ASSERT(pRemoveRowPushButton);
    pRemoveRowPushButton->setObjectName(rowName);
}

void PanelColorsHandlerWidget::installEventFilters()
{
    m_pUi->fontColorDemoFrame->installEventFilter(this);
    m_pUi->backgroundColorDemoFrame->installEventFilter(this);
    m_pUi->backgroundGradientBaseColorDemoFrame->installEventFilter(this);
}

void PanelColorsHandlerWidget::
    openColorDialogForBackgroundGradientTableWidgetRow(int rowIndex)
{
    if (Q_UNLIKELY(rowIndex < 0)) {
        QNWARNING(
            "preferences",
            "Detected attempt to open color dialog for "
                << "background gradient table widget for invalid row: "
                << rowIndex);
        return;
    }

    const auto row = static_cast<std::size_t>(rowIndex);

    if (Q_UNLIKELY(m_backgroundGradientLines.size() <= row)) {
        QNWARNING(
            "preferences",
            "Internal inconsistency detected: attempt to open color dialog for "
                << "background gradient table widget for row "
                << row << " for which no gradient line exists");
        return;
    }

    if (m_backgroundGradientColorDialogs.size() <= row) {
        m_backgroundGradientColorDialogs.resize(row + 1);
    }

    if (!m_backgroundGradientColorDialogs[row].isNull()) {
        QNDEBUG(
            "preferences",
            "Background gradient color dialog for row "
                << row << " is already opened");
        return;
    }

    const auto pDialog = std::make_unique<QColorDialog>(this);
    pDialog->setWindowModality(Qt::WindowModal);
    pDialog->setCurrentColor(m_backgroundGradientLines[row].m_color);
    pDialog->setObjectName(QString::number(rowIndex));

    m_backgroundGradientColorDialogs[row] = pDialog.get();

    QObject::connect(
        pDialog.get(), &QColorDialog::colorSelected, this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientTableWidgetRowColorSelected);

    Q_UNUSED(pDialog->exec())
}

void PanelColorsHandlerWidget::notifyBackgroundGradientUpdated()
{
    QLinearGradient gradient;

    if (m_backgroundGradientLines.empty()) {
        QNDEBUG("preferences", "Background gradient is empty");
        updateBackgroundGradientDemoFrameStyleSheet();
        Q_EMIT backgroundLinearGradientChanged(gradient);
        return;
    }

    gradient.setStart(0, 0);

    for (const auto & line: m_backgroundGradientLines) {
        gradient.setColorAt(line.m_value, line.m_color);
    }

    gradient.setFinalStop(0, 1);

    QNDEBUG("preferences", "Rebuilt background gradient");
    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT backgroundLinearGradientChanged(gradient);
}

void PanelColorsHandlerWidget::updateBackgroundGradientDemoFrameStyleSheet()
{
    QString styleSheet;
    QTextStream strm(&styleSheet);

    strm << "#backgroundGradientDemoFrame {\n"
         << "border: none;\n"
         << "background-color: ";

    if (useBackgroundGradient() && !m_backgroundGradientLines.empty()) {
        strm << "qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n";

        size_t row = 0;
        size_t numRows = m_backgroundGradientLines.size();
        for (const auto & line: m_backgroundGradientLines) {
            strm << "stop: " << line.m_value << " " << line.m_color.name();
            if (row < (numRows - 1)) {
                strm << ",";
            }
            else {
                strm << ");";
            }
            strm << "\n";

            ++row;
        }
    }
    else {
        strm << backgroundColor().name() << ";\n";
    }

    strm << "}\n\n";

    strm << "#backgroundGradientDemoFrameTitleLabel {\n"
         << "color: " << fontColor().name() << ";\n"
         << "}\n";

    m_pUi->backgroundGradientDemoFrame->setStyleSheet(styleSheet);
}

void PanelColorsHandlerWidget::handleBackgroundGradientLinesUpdated()
{
    saveBackgroundGradientLinesToSettings();
    updateBackgroundGradientDemoFrameStyleSheet();
    notifyBackgroundGradientUpdated();
}

QColor PanelColorsHandlerWidget::fontColor()
{
    return colorFromSettingsImpl(preferences::keys::panelFontColor, Qt::white);
}

QColor PanelColorsHandlerWidget::backgroundColor()
{
    return colorFromSettingsImpl(
        preferences::keys::panelBackgroundColor, Qt::darkGray);
}

QColor PanelColorsHandlerWidget::backgroundGradientBaseColor()
{
    return colorFromSettingsImpl(
        preferences::keys::panelBackgroundGradientBaseColor, Qt::darkGray);
}

bool PanelColorsHandlerWidget::useBackgroundGradient()
{
    ApplicationSettings settings(
        m_currentAccount, preferences::keys::files::userInterface);

    settings.beginGroup(preferences::keys::panelColorsGroup);

    const bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    settings.endGroup();

    return useBackgroundGradient;
}

QColor PanelColorsHandlerWidget::colorFromSettingsImpl(
    const char * key, Qt::GlobalColor defaultColor)
{
    ApplicationSettings settings(
        m_currentAccount, preferences::keys::files::userInterface);

    settings.beginGroup(preferences::keys::panelColorsGroup);
    const QString colorName = settings.value(key).toString();
    settings.endGroup();

    QColor color{colorName};
    if (!color.isValid()) {
        color = QColor(defaultColor);
    }

    return color;
}

bool PanelColorsHandlerWidget::onColorEnteredImpl(
    QColor color, QColor prevColor, const char * key, // NOLINT
    QLineEdit & colorLineEdit, QFrame & colorDemoFrame)
{
    if (!color.isValid()) {
        colorLineEdit.setText(prevColor.name());
        return false;
    }

    if (prevColor.isValid() && (prevColor.name() == color.name())) {
        QNDEBUG("preferences", "Color did not change");
        return false;
    }

    saveSettingImpl(color.name(), key);
    setBackgroundColorToDemoFrame(color, colorDemoFrame);
    return true;
}

void PanelColorsHandlerWidget::onUseBackgroundGradientOptionChanged(
    bool enabled)
{
    if (enabled == useBackgroundGradient()) {
        QNDEBUG("preferences", "Option hasn't changed");
        return;
    }

    if (enabled && m_backgroundGradientLines.empty()) {
        Q_EMIT notifyUserError(
            tr("Set up background gradient before switching to it"));

        m_pUi->useBackgroundColorRadioButton->setChecked(true);
        m_pUi->useBackgroundGradientRadioButton->setChecked(false);
        return;
    }

    saveUseBackgroundGradientSetting(enabled);
    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT useBackgroundGradientSettingChanged(enabled);
}

void PanelColorsHandlerWidget::saveFontColor(const QColor & color)
{
    saveSettingImpl(color.name(), preferences::keys::panelFontColor);
}

void PanelColorsHandlerWidget::saveBackgroundColor(const QColor & color)
{
    saveSettingImpl(color.name(), preferences::keys::panelBackgroundColor);
}

void PanelColorsHandlerWidget::saveBackgroundGradientBaseColor(
    const QColor & color)
{
    saveSettingImpl(
        color.name(), preferences::keys::panelBackgroundGradientBaseColor);
}

void PanelColorsHandlerWidget::saveUseBackgroundGradientSetting(
    bool useBackgroundGradient)
{
    saveSettingImpl(
        useBackgroundGradient, preferences::keys::panelUseBackgroundGradient);
}

void PanelColorsHandlerWidget::saveSettingImpl(
    const QVariant & value, const char * key)
{
    ApplicationSettings settings(
        m_currentAccount, preferences::keys::files::userInterface);

    settings.beginGroup(preferences::keys::panelColorsGroup);
    settings.setValue(key, value);
    settings.endGroup();
    settings.sync();
}

void PanelColorsHandlerWidget::saveBackgroundGradientLinesToSettings()
{
    ApplicationSettings settings(
        m_currentAccount, preferences::keys::files::userInterface);

    settings.beginGroup(preferences::keys::panelColorsGroup);

    settings.beginWriteArray(
        preferences::keys::panelBackgroundGradientLineCount);

    int i = 0;
    for (const auto & line: m_backgroundGradientLines) {
        settings.setArrayIndex(i);

        settings.setValue(
            preferences::keys::panelBackgroundGradientLineSize, line.m_value);

        settings.setValue(
            preferences::keys::panelBackgroundGradientLineColor,
            line.m_color.name());

        ++i;
    }

    settings.endArray();
    settings.endGroup();
    settings.sync();
}

void PanelColorsHandlerWidget::setBackgroundColorToDemoFrame(
    const QColor & color, QFrame & frame)
{
    QString styleSheet;
    QTextStream strm(&styleSheet);

    strm << "QFrame {\n"
         << "background-color: " << color.name() << ";\n"
         << "}\n";

    frame.setStyleSheet(styleSheet);
}

} // namespace quentier
