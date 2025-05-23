/*
 * Copyright 2019-2025 Dmitry Ivanov
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
    QWidget{parent}, m_ui{new Ui::PanelColorsHandlerWidget}
{
    m_ui->setupUi(this);
}

PanelColorsHandlerWidget::~PanelColorsHandlerWidget()
{
    delete m_ui;
}

void PanelColorsHandlerWidget::initialize(Account account)
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::initialize: " << account);

    if (m_currentAccount == account) {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Already initialized for this account, nothing to do");
        return;
    }

    m_currentAccount = std::move(account);
    restoreAccountSettings();
    setupBackgroundGradientTableWidget();
    updateBackgroundGradientDemoFrameStyleSheet();

    createConnections();
    installEventFilters();
}

void PanelColorsHandlerWidget::onFontColorEntered()
{
    const QString colorCode = m_ui->fontColorLineEdit->text();

    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onFontColorEntered: " << colorCode);

    QColor prevColor = fontColor();
    QColor color{colorCode};

    if (!onColorEnteredImpl(
            color, std::move(prevColor), preferences::keys::panelFontColor,
            *m_ui->fontColorLineEdit, *m_ui->fontColorDemoFrame))
    {
        return;
    }

    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT fontColorChanged(std::move(color));
}

void PanelColorsHandlerWidget::onFontColorDialogRequested()
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onFontColorDialogRequested");

    if (!m_fontColorDialog.isNull()) {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Dialog is already opened");
        return;
    }

    auto dialog = std::make_unique<QColorDialog>(this);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setCurrentColor(fontColor());

    m_fontColorDialog = dialog.get();

    QObject::connect(
        dialog.get(), &QColorDialog::colorSelected, this,
        &PanelColorsHandlerWidget::onFontColorSelected);

    Q_UNUSED(dialog->exec())
}

void PanelColorsHandlerWidget::onFontColorSelected(const QColor & color)
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onFontColorSelected: " << color.name());

    m_ui->fontColorLineEdit->setText(color.name());
    setBackgroundColorToDemoFrame(color, *m_ui->fontColorDemoFrame);

    const QColor prevColor = fontColor();
    if (prevColor.isValid() && color.isValid() &&
        (color.name() == prevColor.name()))
    {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Font color did not change");
        return;
    }

    saveFontColor(color);
    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT fontColorChanged(color);
}

void PanelColorsHandlerWidget::onBackgroundColorEntered()
{
    const QString colorCode = m_ui->backgroundColorLineEdit->text();

    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onBackgroundColorEntered: " << colorCode);

    QColor prevColor = backgroundColor();
    QColor color{colorCode};

    if (!onColorEnteredImpl(
            color, std::move(prevColor),
            preferences::keys::panelBackgroundColor,
            *m_ui->backgroundColorLineEdit, *m_ui->backgroundColorDemoFrame))
    {
        return;
    }

    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT backgroundColorChanged(std::move(color));
}

void PanelColorsHandlerWidget::onBackgroundColorDialogRequested()
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onBackgroundColorDialogRequested");

    if (!m_backgroundColorDialog.isNull()) {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Dialog is already opened");
        return;
    }

    auto dialog = std::make_unique<QColorDialog>(this);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setCurrentColor(backgroundColor());

    m_backgroundColorDialog = dialog.get();

    QObject::connect(
        dialog.get(), &QColorDialog::colorSelected, this,
        &PanelColorsHandlerWidget::onBackgroundColorSelected);

    Q_UNUSED(dialog->exec())
}

void PanelColorsHandlerWidget::onBackgroundColorSelected(const QColor & color)
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onBackgroundColorSelected: "
            << color.name());

    m_ui->backgroundColorLineEdit->setText(color.name());
    setBackgroundColorToDemoFrame(color, *m_ui->backgroundColorDemoFrame);

    const QColor prevColor = backgroundColor();
    if (prevColor.isValid() && color.isValid() &&
        (color.name() == prevColor.name()))
    {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Background color did not change");
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
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onUseBackgroundGradientRadioButtonToggled: "
            << "checked = " << (checked ? "yes" : "no"));

    onUseBackgroundGradientOptionChanged(checked);
}

void PanelColorsHandlerWidget::onUseBackgroundColorRadioButtonToggled(
    bool checked)
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onUseBackgroundColorRadioButtonToggled: "
            << "checked = " << (checked ? "yes" : "no"));

    onUseBackgroundGradientOptionChanged(!checked);
}

void PanelColorsHandlerWidget::onBackgroundGradientBaseColorEntered()
{
    const QString colorCode = m_ui->backgroundGradientBaseColorLineEdit->text();

    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onBackgroundGradientBaseColorEntered: "
            << colorCode);

    QColor prevColor = backgroundGradientBaseColor();
    QColor color{colorCode};

    Q_UNUSED(onColorEnteredImpl(
        std::move(color), std::move(prevColor),
        preferences::keys::panelBackgroundGradientBaseColor,
        *m_ui->backgroundGradientBaseColorLineEdit,
        *m_ui->backgroundGradientBaseColorDemoFrame))
}

void PanelColorsHandlerWidget::onBackgroundGradientBaseColorDialogRequested()
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::"
        "onBackgroundGradientBaseColorDialogRequested");

    if (!m_backgroundGradientBaseColorDialog.isNull()) {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Dialog is already opened");
        return;
    }

    auto dialog = std::make_unique<QColorDialog>(this);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setCurrentColor(backgroundGradientBaseColor());

    m_backgroundGradientBaseColorDialog = dialog.get();

    QObject::connect(
        dialog.get(), &QColorDialog::colorSelected, this,
        &PanelColorsHandlerWidget::onBackgroundGradientBaseColorSelected);

    Q_UNUSED(dialog->exec())
}

void PanelColorsHandlerWidget::onBackgroundGradientBaseColorSelected(
    const QColor & color)
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onBackgroundGradientBaseColorSelected: "
            << color.name());

    m_ui->backgroundGradientBaseColorLineEdit->setText(color.name());

    setBackgroundColorToDemoFrame(
        color, *m_ui->backgroundGradientBaseColorDemoFrame);

    const QColor prevColor = backgroundGradientBaseColor();
    if (prevColor.isValid() && color.isValid() &&
        (color.name() == prevColor.name()))
    {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Background gradient base color did not change");
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
            "preferences::PanelColorsHandlerWidget",
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
            "preferences::PanelColorsHandlerWidget",
            "Failed to identify row upon receiving value edit from background "
                << "gradient table widget, value = " << value);
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<std::size_t>(rowIndex) >=
                m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Internal inconsistency detected: bad row "
                << rowIndex << " upon receiving value edit from background "
                << "gradient table widget, value = " << value);
        return;
    }

    bool needsSorting = false;
    if (rowIndex > 0) {
        const int prevRow = rowIndex - 1;
        const double prevValue =
            m_backgroundGradientLines[static_cast<std::size_t>(prevRow)]
                .m_value;

        if (value < prevValue) {
            needsSorting = true;
        }
    }

    if (!needsSorting &&
        rowIndex < static_cast<int>(m_backgroundGradientLines.size() - 1))
    {
        const int nextRow = rowIndex + 1;
        const double nextValue =
            m_backgroundGradientLines[static_cast<std::size_t>(nextRow)]
                .m_value;

        if (value > nextValue) {
            needsSorting = true;
        }
    }

    m_backgroundGradientLines[static_cast<std::size_t>(rowIndex)].m_value =
        value;

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
    auto * lineEdit = qobject_cast<QLineEdit *>(sender());
    if (Q_UNLIKELY(!lineEdit)) {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Received background gradient table widget row color name edited "
            "from unidentified source");
        return;
    }

    // Line edit's row within the table widget is encoded within its object name
    bool conversionResult = false;
    const int rowIndex = lineEdit->objectName().toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Failed to identify row upon receiving color name edit from "
                << "background gradient table widget, text = "
                << lineEdit->text());
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<std::size_t>(rowIndex) >=
                m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Internal inconsistency detected: bad row "
                << rowIndex
                << " upon receiving color name edit from background "
                << "gradient table widget, text = " << lineEdit->text());
        return;
    }

    const QString colorName = lineEdit->text();
    auto & line = m_backgroundGradientLines[static_cast<std::size_t>(rowIndex)];
    QColor prevColor = line.m_color;

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    line.m_color = QColor::fromString(colorName);
#else
    line.m_color.setNamedColor(colorName);
#endif

    if (!line.m_color.isValid()) {
        line.m_color = std::move(prevColor);

        QNINFO(
            "preferences::PanelColorsHandlerWidget",
            "Rejecting color input for background gradient table row: "
                << colorName << " is not a valid color name");

        Q_EMIT notifyUserError(
            tr("Rejected input color name as it is not valid"));
        return;
    }

    auto * demoFrameWidget =
        m_ui->backgroundGradientTableWidget->cellWidget(rowIndex, 2);

    if (demoFrameWidget) {
        auto * demoFrame = demoFrameWidget->findChild<QFrame *>();
        if (demoFrame) {
            setBackgroundColorToDemoFrame(line.m_color, *demoFrame);
        }
    }

    handleBackgroundGradientLinesUpdated();
}

void PanelColorsHandlerWidget::onBackgroundGradientTableWidgetRowColorSelected(
    const QColor & color)
{
    auto * dialog = qobject_cast<QColorDialog *>(sender());
    if (Q_UNLIKELY(!dialog)) {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Received background gradient table widget selected color from "
                << "unidentified source, color = " << color.name());
        return;
    }

    // QColorDialog's target row within the table widget is encoded within
    // its object name
    bool conversionResult = false;
    const int rowIndex = dialog->objectName().toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Failed to identify row upon receiving selected color from "
                << "QColorDialog for background gradient table widget, color = "
                << color.name());
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<std::size_t>(rowIndex) >=
                m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Internal inconsistency detected: bad row "
                << rowIndex << " upon receiving selected color for background "
                << "gradient table widget, color = " << color.name());
        return;
    }

    m_backgroundGradientLines[static_cast<std::size_t>(rowIndex)].m_color =
        color;

    auto * lineEdit = qobject_cast<QLineEdit *>(
        m_ui->backgroundGradientTableWidget->cellWidget(rowIndex, 1));

    if (lineEdit) {
        lineEdit->setText(color.name());
    }

    auto * demoFrameWidget =
        m_ui->backgroundGradientTableWidget->cellWidget(rowIndex, 2);

    if (demoFrameWidget) {
        auto * demoFrame = demoFrameWidget->findChild<QFrame *>();
        if (demoFrame) {
            setBackgroundColorToDemoFrame(color, *demoFrame);
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
            "preferences::PanelColorsHandlerWidget",
            "Received background gradient table widget row color dialog "
            "request from unidentified source");
        return;
    }

    // Button's row within the table widget is encoded within its object name
    bool conversionResult = false;
    const int rowIndex = pButton->objectName().toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Failed to identify row upon receiving choose color button click "
            "from background gradient table widget");
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<std::size_t>(rowIndex) >=
                m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Internal inconsistency detected: bad row "
                << rowIndex << " upon receiving choose color button click from "
                << "background gradient table widget");
        return;
    }

    openColorDialogForBackgroundGradientTableWidgetRow(rowIndex);
}

void PanelColorsHandlerWidget::onGenerateButtonPressed()
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::onGenerateButtonPressed");

    QColor baseColor(m_ui->backgroundGradientBaseColorLineEdit->text());
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
    const int rowIndex = m_ui->backgroundGradientTableWidget->rowCount();
    m_ui->backgroundGradientTableWidget->insertRow(rowIndex);

    GradientLine line(1.0, QColor(Qt::black).name());
    setupBackgroundGradientTableWidgetRow(line, rowIndex);
    m_backgroundGradientLines.emplace_back(std::move(line));
    handleBackgroundGradientLinesUpdated();
}

void PanelColorsHandlerWidget::onRemoveRowButtonPressed()
{
    auto * pButton = qobject_cast<QPushButton *>(sender());
    if (Q_UNLIKELY(!pButton)) {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Received row removal request from unidentified source");
        return;
    }

    // Push button's row within the table widget is encoded within its object
    // name
    bool conversionResult = false;
    const int rowIndex = pButton->objectName().toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Failed to identify row upon receiving row removal request from "
            "background gradient table widget");
        return;
    }

    if (Q_UNLIKELY(
            rowIndex < 0 ||
            static_cast<std::size_t>(rowIndex) >=
                m_backgroundGradientLines.size()))
    {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Internal inconsistency detected: bad row "
                << rowIndex << " upon receiving row removal request from "
                << "background gradient table widget, row = " << rowIndex);
        return;
    }

    const bool isLastRow =
        (rowIndex == (m_ui->backgroundGradientTableWidget->rowCount() - 1));

    m_ui->backgroundGradientTableWidget->removeRow(rowIndex);

    if (!isLastRow) {
        // Need to assign correct row names to the widgets of all rows past
        // the removed one
        const int rowCount = m_ui->backgroundGradientTableWidget->rowCount();
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

bool PanelColorsHandlerWidget::eventFilter(QObject * object, QEvent * event)
{
    if (object == m_ui->fontColorDemoFrame) {
        if (event && (event->type() == QEvent::MouseButtonDblClick)) {
            onFontColorDialogRequested();
            return true;
        }
    }
    else if (object == m_ui->backgroundColorDemoFrame) {
        if (event && (event->type() == QEvent::MouseButtonDblClick)) {
            onBackgroundColorDialogRequested();
            return true;
        }
    }
    else if (object == m_ui->backgroundGradientBaseColorDemoFrame) {
        if (event && (event->type() == QEvent::MouseButtonDblClick)) {
            onBackgroundGradientBaseColorDialogRequested();
            return true;
        }
    }

    if (!event || (event->type() != QEvent::MouseButtonDblClick)) {
        return false;
    }

    const int rowCount = m_ui->backgroundGradientTableWidget->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        auto * demoFrameWidget =
            m_ui->backgroundGradientTableWidget->cellWidget(i, 2);
        if (!demoFrameWidget) {
            continue;
        }

        auto * demoFrame = demoFrameWidget->findChild<QFrame *>();
        if (demoFrame && (demoFrame == object)) {
            openColorDialogForBackgroundGradientTableWidgetRow(i);
            return true;
        }
    }

    return false;
}

void PanelColorsHandlerWidget::restoreAccountSettings()
{
    QNDEBUG(
        "preferences::PanelColorsHandlerWidget",
        "PanelColorsHandlerWidget::restoreAccountSettings");

    if (m_currentAccount.isEmpty()) {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Account is empty, nothing to restore");
        return;
    }

    utility::ApplicationSettings settings{
        m_currentAccount, preferences::keys::files::userInterface};

    settings.beginGroup(preferences::keys::panelColorsGroup);
    utility::ApplicationSettings::GroupCloser groupCloser{settings};

    const QString fontColorName =
        settings.value(preferences::keys::panelFontColor).toString();

    QColor fontColor{fontColorName};
    if (!fontColor.isValid()) {
        fontColor = QColor{Qt::white};
    }

    m_ui->fontColorLineEdit->setText(fontColor.name());
    setBackgroundColorToDemoFrame(fontColor, *m_ui->fontColorDemoFrame);

    const QString backgroundColorName =
        settings.value(preferences::keys::panelBackgroundColor).toString();

    QColor backgroundColor{backgroundColorName};
    if (!backgroundColor.isValid()) {
        backgroundColor = QColor{Qt::darkGray};
    }

    m_ui->backgroundColorLineEdit->setText(backgroundColor.name());

    setBackgroundColorToDemoFrame(
        backgroundColor, *m_ui->backgroundColorDemoFrame);

    const bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    m_ui->useBackgroundGradientRadioButton->setChecked(useBackgroundGradient);
    m_ui->useBackgroundColorRadioButton->setChecked(!useBackgroundGradient);

    const QString backgroundGradientBaseColorName =
        settings.value(preferences::keys::panelBackgroundGradientBaseColor)
            .toString();

    QColor backgroundGradientBaseColor{backgroundGradientBaseColorName};
    if (!backgroundGradientBaseColor.isValid()) {
        backgroundGradientBaseColor = QColor{Qt::darkGray};
    }

    m_ui->backgroundGradientBaseColorLineEdit->setText(
        backgroundGradientBaseColor.name());

    setBackgroundColorToDemoFrame(
        backgroundGradientBaseColor,
        *m_ui->backgroundGradientBaseColorDemoFrame);

    const int numBackgroundGradientLines = settings.beginReadArray(
        preferences::keys::panelBackgroundGradientLineCount);

    utility::ApplicationSettings::ArrayCloser arrayCloser{settings};

    m_backgroundGradientLines.clear();
    m_backgroundGradientLines.reserve(
        static_cast<std::size_t>(std::max(numBackgroundGradientLines, 0)));

    for (std::size_t
             i = 0,
             size = static_cast<std::size_t>(numBackgroundGradientLines);
         i < size; ++i)
    {
        settings.setArrayIndex(static_cast<int>(i));

        bool conversionResult = false;

        const double value =
            settings.value(preferences::keys::panelBackgroundGradientLineSize)
                .toDouble(&conversionResult);

        if (!conversionResult) {
            QNWARNING(
                "preferences::PanelColorsHandlerWidget",
                "Failed to read background gradient lines: failed to convert "
                "line value to double");
            m_backgroundGradientLines.clear();
            break;
        }

        QString colorName =
            settings.value(preferences::keys::panelBackgroundGradientLineColor)
                .toString();

        const QColor color{colorName};
        if (!color.isValid()) {
            QNWARNING(
                "preferences::PanelColorsHandlerWidget",
                "Failed to read background gradient lines: detected line color "
                    << "value not representing a valid color: " << colorName);
            m_backgroundGradientLines.clear();
            break;
        }

        m_backgroundGradientLines.emplace_back(value, std::move(colorName));
    }
}

void PanelColorsHandlerWidget::createConnections()
{
    QObject::connect(
        m_ui->fontColorLineEdit, &QLineEdit::editingFinished, this,
        &PanelColorsHandlerWidget::onFontColorEntered);

    QObject::connect(
        m_ui->fontColorPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::onFontColorDialogRequested);

    QObject::connect(
        m_ui->backgroundColorLineEdit, &QLineEdit::editingFinished, this,
        &PanelColorsHandlerWidget::onBackgroundColorEntered);

    QObject::connect(
        m_ui->backgroundColorPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::onBackgroundColorDialogRequested);

    QObject::connect(
        m_ui->useBackgroundColorRadioButton, &QRadioButton::toggled, this,
        &PanelColorsHandlerWidget::onUseBackgroundColorRadioButtonToggled);

    QObject::connect(
        m_ui->useBackgroundGradientRadioButton, &QRadioButton::toggled, this,
        &PanelColorsHandlerWidget::onUseBackgroundGradientRadioButtonToggled);

    QObject::connect(
        m_ui->backgroundGradientBaseColorLineEdit, &QLineEdit::editingFinished,
        this, &PanelColorsHandlerWidget::onBackgroundGradientBaseColorEntered);

    QObject::connect(
        m_ui->backgroundGradientBaseColorPushButton, &QPushButton::clicked,
        this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientBaseColorDialogRequested);

    QObject::connect(
        m_ui->backgroundGradientGeneratePushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::onGenerateButtonPressed);

    QObject::connect(
        m_ui->backgroundGradientAddRowPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::onAddRowButtonPressed);
}

void PanelColorsHandlerWidget::setupBackgroundGradientTableWidget()
{
    m_ui->backgroundGradientTableWidget->clear();

    m_ui->backgroundGradientTableWidget->setRowCount(
        static_cast<int>(m_backgroundGradientLines.size()));

    m_ui->backgroundGradientTableWidget->setColumnCount(5);

    m_ui->backgroundGradientTableWidget->setHorizontalHeaderLabels(
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

    auto * valueSpinBox = new QDoubleSpinBox;
    valueSpinBox->setObjectName(rowName);
    valueSpinBox->setValue(gradientLine.m_value);
    valueSpinBox->setRange(0.0, 1.0);
    valueSpinBox->setSingleStep(0.05);

    auto * colorNameLineEdit = new QLineEdit;
    colorNameLineEdit->setObjectName(rowName);
    colorNameLineEdit->setText(gradientLine.m_color.name());

    auto * colorDemoFrame = new QFrame;
    colorDemoFrame->setObjectName(rowName);
    colorDemoFrame->setMinimumSize(24, 24);
    colorDemoFrame->setMaximumSize(24, 24);
    colorDemoFrame->setFrameShape(QFrame::Box);
    colorDemoFrame->setFrameShadow(QFrame::Plain);
    setBackgroundColorToDemoFrame(gradientLine.m_color, *colorDemoFrame);

    auto * colorDemoFrameWidget = new QWidget;
    auto * colorDemoFrameWidgetLayout = new QHBoxLayout(colorDemoFrameWidget);
    colorDemoFrameWidgetLayout->addWidget(colorDemoFrame);
    colorDemoFrameWidgetLayout->setAlignment(Qt::AlignCenter);
    colorDemoFrameWidgetLayout->setContentsMargins(0, 0, 0, 0);

    auto * colorPushButton = new QPushButton;
    colorPushButton->setObjectName(rowName);
    colorPushButton->setText(tr("Choose"));
    colorPushButton->setToolTip(tr("Choose color for row"));

    auto * removeRowPushButton = new QPushButton;
    removeRowPushButton->setObjectName(rowName);

    removeRowPushButton->setIcon(
        QIcon::fromTheme(QStringLiteral("edit-delete")));

    removeRowPushButton->setToolTip(tr("Remove this row"));
    removeRowPushButton->setFlat(true);

    m_ui->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 0, valueSpinBox);

    m_ui->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 1, colorNameLineEdit);

    m_ui->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 2, colorDemoFrameWidget);

    m_ui->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 3, colorPushButton);

    m_ui->backgroundGradientTableWidget->setCellWidget(
        rowIndex, 4, removeRowPushButton);

    QObject::connect(
        valueSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientTableWidgetRowValueEdited);

    QObject::connect(
        colorNameLineEdit, &QLineEdit::editingFinished, this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientTableWidgetRowColorEntered);

    colorDemoFrame->installEventFilter(this);

    QObject::connect(
        colorPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientTableWidgetRowColorDialogRequested);

    QObject::connect(
        removeRowPushButton, &QPushButton::clicked, this,
        &PanelColorsHandlerWidget::onRemoveRowButtonPressed);
}

void PanelColorsHandlerWidget::setNamesToBackgroundGradientTableWidgetRow(
    const int rowIndex)
{
    const auto rowName = QString::number(rowIndex);

    auto * valueSpinBox = qobject_cast<QDoubleSpinBox *>(
        m_ui->backgroundGradientTableWidget->cellWidget(rowIndex, 0));

    Q_ASSERT(valueSpinBox);
    valueSpinBox->setObjectName(rowName);

    auto * colorNameLineEdit = qobject_cast<QLineEdit *>(
        m_ui->backgroundGradientTableWidget->cellWidget(rowIndex, 1));

    Q_ASSERT(colorNameLineEdit);
    colorNameLineEdit->setObjectName(rowName);

    auto * colorDemoFrameWidget =
        m_ui->backgroundGradientTableWidget->cellWidget(rowIndex, 2);

    Q_ASSERT(colorDemoFrameWidget);

    auto * colorDemoFrame = colorDemoFrameWidget->findChild<QFrame *>();
    Q_ASSERT(colorDemoFrame);
    colorDemoFrame->setObjectName(rowName);

    auto * colorPushButton = qobject_cast<QPushButton *>(
        m_ui->backgroundGradientTableWidget->cellWidget(rowIndex, 3));

    Q_ASSERT(colorPushButton);
    colorPushButton->setObjectName(rowName);

    auto * removeRowPushButton = qobject_cast<QPushButton *>(
        m_ui->backgroundGradientTableWidget->cellWidget(rowIndex, 4));

    Q_ASSERT(removeRowPushButton);
    removeRowPushButton->setObjectName(rowName);
}

void PanelColorsHandlerWidget::installEventFilters()
{
    m_ui->fontColorDemoFrame->installEventFilter(this);
    m_ui->backgroundColorDemoFrame->installEventFilter(this);
    m_ui->backgroundGradientBaseColorDemoFrame->installEventFilter(this);
}

void PanelColorsHandlerWidget::
    openColorDialogForBackgroundGradientTableWidgetRow(int rowIndex)
{
    if (Q_UNLIKELY(rowIndex < 0)) {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Detected attempt to open color dialog for background gradient "
                << "table widget for invalid row: " << rowIndex);
        return;
    }

    const std::size_t row = static_cast<std::size_t>(rowIndex);

    if (Q_UNLIKELY(m_backgroundGradientLines.size() <= row)) {
        QNWARNING(
            "preferences::PanelColorsHandlerWidget",
            "Internal inconsistency detected: attempt to open color dialog for "
                << "background gradient table widget for row " << row
                << " for which no gradient line exists");
        return;
    }

    if (m_backgroundGradientColorDialogs.size() <= row) {
        m_backgroundGradientColorDialogs.resize(row + 1);
    }

    if (!m_backgroundGradientColorDialogs[row].isNull()) {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Background gradient color dialog for row "
                << row << " is already opened");
        return;
    }

    auto dialog = std::make_unique<QColorDialog>(this);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setCurrentColor(m_backgroundGradientLines[row].m_color);
    dialog->setObjectName(QString::number(rowIndex));

    m_backgroundGradientColorDialogs[row] = dialog.get();

    QObject::connect(
        dialog.get(), &QColorDialog::colorSelected, this,
        &PanelColorsHandlerWidget::
            onBackgroundGradientTableWidgetRowColorSelected);

    Q_UNUSED(dialog->exec())
}

void PanelColorsHandlerWidget::notifyBackgroundGradientUpdated()
{
    QLinearGradient gradient;

    if (m_backgroundGradientLines.empty()) {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget",
            "Background gradient is empty");
        updateBackgroundGradientDemoFrameStyleSheet();
        Q_EMIT backgroundLinearGradientChanged(gradient);
        return;
    }

    gradient.setStart(0, 0);

    for (const auto & line: m_backgroundGradientLines) {
        gradient.setColorAt(line.m_value, line.m_color);
    }

    gradient.setFinalStop(0, 1);

    QNDEBUG(
        "preferences::PanelColorsHandlerWidget", "Rebuilt background gradient");
    updateBackgroundGradientDemoFrameStyleSheet();
    Q_EMIT backgroundLinearGradientChanged(gradient);
}

void PanelColorsHandlerWidget::updateBackgroundGradientDemoFrameStyleSheet()
{
    QString styleSheet;
    QTextStream strm{&styleSheet};

    strm << "#backgroundGradientDemoFrame {\n"
         << "border: none;\n"
         << "background-color: ";

    if (useBackgroundGradient() && !m_backgroundGradientLines.empty()) {
        strm << "qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,\n";

        std::size_t row = 0;
        std::size_t numRows = m_backgroundGradientLines.size();
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

    m_ui->backgroundGradientDemoFrame->setStyleSheet(styleSheet);
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
    utility::ApplicationSettings settings{
        m_currentAccount, preferences::keys::files::userInterface};

    settings.beginGroup(preferences::keys::panelColorsGroup);
    utility::ApplicationSettings::GroupCloser groupCloser{settings};

    const bool useBackgroundGradient =
        settings.value(preferences::keys::panelUseBackgroundGradient).toBool();

    return useBackgroundGradient;
}

QColor PanelColorsHandlerWidget::colorFromSettingsImpl(
    const std::string_view key, Qt::GlobalColor defaultColor)
{
    utility::ApplicationSettings settings{
        m_currentAccount, preferences::keys::files::userInterface};

    settings.beginGroup(preferences::keys::panelColorsGroup);
    utility::ApplicationSettings::GroupCloser groupCloser{settings};

    const QString colorName = settings.value(key).toString();

    QColor color{colorName};
    if (!color.isValid()) {
        color = QColor{defaultColor};
    }

    return color;
}

bool PanelColorsHandlerWidget::onColorEnteredImpl(
    QColor color, QColor prevColor, const std::string_view key,
    QLineEdit & colorLineEdit, QFrame & colorDemoFrame)
{
    if (!color.isValid()) {
        colorLineEdit.setText(prevColor.name());
        return false;
    }

    if (prevColor.isValid() && (prevColor.name() == color.name())) {
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget", "Color did not change");
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
        QNDEBUG(
            "preferences::PanelColorsHandlerWidget", "Option hasn't changed");
        return;
    }

    if (enabled && m_backgroundGradientLines.empty()) {
        Q_EMIT notifyUserError(
            tr("Set up background gradient before switching to it"));

        m_ui->useBackgroundColorRadioButton->setChecked(true);
        m_ui->useBackgroundGradientRadioButton->setChecked(false);
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
    const QVariant & value, const std::string_view key)
{
    utility::ApplicationSettings settings{
        m_currentAccount, preferences::keys::files::userInterface};

    settings.beginGroup(preferences::keys::panelColorsGroup);
    utility::ApplicationSettings::GroupCloser groupCloser{settings};

    settings.setValue(key, value);
}

void PanelColorsHandlerWidget::saveBackgroundGradientLinesToSettings()
{
    utility::ApplicationSettings settings{
        m_currentAccount, preferences::keys::files::userInterface};

    settings.beginGroup(preferences::keys::panelColorsGroup);
    utility::ApplicationSettings::GroupCloser groupCloser{settings};

    settings.beginWriteArray(
        preferences::keys::panelBackgroundGradientLineCount);
    utility::ApplicationSettings::ArrayCloser arrayCloser{settings};

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
}

void PanelColorsHandlerWidget::setBackgroundColorToDemoFrame(
    const QColor & color, QFrame & frame)
{
    QString styleSheet;
    QTextStream strm{&styleSheet};

    strm << "QFrame {\n"
         << "background-color: " << color.name() << ";\n"
         << "}\n";

    frame.setStyleSheet(styleSheet);
}

} // namespace quentier
