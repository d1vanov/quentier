/*
 * Copyright 2018 Dmitry Ivanov
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

#include "MainWindowSideBordersController.h"
#include "MainWindowSideBorderOption.h"

#include "views/FavoriteItemView.h"
#include "views/NotebookItemView.h"
#include "views/TagItemView.h"
#include "views/SavedSearchItemView.h"
#include "views/DeletedNoteItemView.h"
#include "views/NoteListView.h"
#include "widgets/FilterByNotebookWidget.h"
#include "widgets/FilterByTagWidget.h"
#include "widgets/FilterBySavedSearchWidget.h"
#include "widgets/TabWidget.h"

using namespace quentier;
#include "ui_MainWindow.h"

#include "SettingsNames.h"
#include "DefaultSettings.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QColor>

#define BORDER_MIN_WIDTH (4)
#define BORDER_MAX_WIDTH (20)

namespace quentier {

MainWindowSideBordersController::MainWindowSideBordersController(const Account & account,
                                                                 Ui::MainWindow & ui,
                                                                 QObject * parent) :
    QObject(parent),
    m_ui(ui),
    m_currentAccount(account)
{
    initializeBordersState();
}

void MainWindowSideBordersController::toggleLeftBorderDisplay(bool on)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::toggleLeftBorderDisplay: ")
            << (on ? QStringLiteral("on") : QStringLiteral("off")));

    toggleBorderDisplay(on, *m_ui.leftBorderWidget);
}

void MainWindowSideBordersController::toggleRightBorderDisplay(bool on)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::toggleRightBorderDisplay: ")
            << (on ? QStringLiteral("on") : QStringLiteral("off")));

    toggleBorderDisplay(on, *m_ui.rightBorderWidget);
}

void MainWindowSideBordersController::onLeftBorderWidthChanged(int width)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onLeftBorderWidthChanged: ") << width);
    onBorderWidthChanged(width, *m_ui.leftBorderWidget);
}

void MainWindowSideBordersController::onRightBorderWidthChanged(int width)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onRightBorderWidthChanged: ") << width);
    onBorderWidthChanged(width, *m_ui.rightBorderWidget);
}

void MainWindowSideBordersController::onLeftBorderColorChanged(QString colorCode)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onLeftBorderColorChanged: color code = ") << colorCode);
    onBorderColorChanged(colorCode, *m_ui.leftBorderWidget);
}

void MainWindowSideBordersController::onRightBorderColorChanged(QString colorCode)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onRightBorderColorChanged: color code = ") << colorCode);
    onBorderColorChanged(colorCode, *m_ui.rightBorderWidget);
}

void MainWindowSideBordersController::onMainWindowExpanded()
{
    QNDEBUG(QStringLiteral("MainWindowSideBorderOption::onMainWindowExpanded"));
    // TODO: implement
}

void MainWindowSideBordersController::onMainWindowUnexpanded()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onMainWindowUnexpanded"));
    // TODO: implement
}

void MainWindowSideBordersController::setCurrentAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::setCurrentAccount: ") << account);

    if (m_currentAccount == account) {
        QNDEBUG(QStringLiteral("This account is already set, nothing to do"));
        return;
    }

    m_currentAccount = account;
    initializeBordersState();
}

void MainWindowSideBordersController::toggleBorderDisplay(const bool on, QWidget & border)
{
    if (on) {
        border.show();
    }
    else {
        border.hide();
    }
}

void MainWindowSideBordersController::onBorderWidthChanged(const int width, QWidget & border)
{
    int sanitizedWidth = sanitizeWidth(width);
    border.resize(sanitizedWidth, border.height());
}

void MainWindowSideBordersController::onBorderColorChanged(const QString & colorCode, QWidget & border)
{
    QColor color(colorCode);
    if (color.isValid()) {
        setBorderStyleSheet(colorCode, border);
        return;
    }

    initializeBorderColor(border);
}

void MainWindowSideBordersController::setBorderStyleSheet(const QString & colorCode, QWidget & border)
{
    QString styleSheet = QStringLiteral("background-color: ");
    styleSheet += colorCode;
    styleSheet += QStringLiteral("; border-right: 1px solid black; margin-right: 0px;");
    border.setStyleSheet(styleSheet);
}

void MainWindowSideBordersController::initializeBordersState()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::initializeBordersState"));

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

    MainWindowSideBorderOption::type leftBorderOption = MainWindowSideBorderOption::ShowOnlyWhenExpanded;
    bool conversionResult = false;
    int leftBorderOptionInt = appSettings.value(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY).toInt(&conversionResult);
    if (!conversionResult || (leftBorderOptionInt < 0) || (leftBorderOptionInt > 2)) {
        QNINFO(QStringLiteral("Couldn't restore the persistent \"Show left main window border\" option, "
                              "using the default one: show only when expanded"));
    }
    else {
        leftBorderOption = static_cast<MainWindowSideBorderOption::type>(leftBorderOptionInt);
    }

    MainWindowSideBorderOption::type rightBorderOption = MainWindowSideBorderOption::ShowOnlyWhenExpanded;
    conversionResult = false;
    int rightBorderOptionInt = appSettings.value(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY).toInt(&conversionResult);
    if (!conversionResult || (rightBorderOptionInt < 0) || (rightBorderOptionInt > 2)) {
        QNINFO(QStringLiteral("Couldn't restore the persistent \"Show right main window border\" option, "
                              "using the default one: show only when expanded"));
    }
    else {
        rightBorderOption = static_cast<MainWindowSideBorderOption::type>(rightBorderOptionInt);
    }

    conversionResult = false;
    int leftBorderWidth = appSettings.value(LEFT_MAIN_WINDOW_BORDER_WIDTH_KEY).toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNDEBUG(QStringLiteral("Couldn't restore the persistent \"Left main window border width\" option, "
                               "using the default one: 4px"));
        leftBorderWidth = 4;
    }

    conversionResult = false;
    int rightBorderWidth = appSettings.value(RIGHT_MAIN_WINDOW_BORDER_WIDTH_KEY).toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNDEBUG(QStringLiteral("Couldn't restore the persistent \"Right main window border width\" option, "
                               "using the default one: 4px"));
        rightBorderWidth = 4;
    }

    appSettings.endGroup();

    // TODO: apply show option and width option to both left and right borders

    initializeBorderColor(*m_ui.leftBorderWidget);
    initializeBorderColor(*m_ui.rightBorderWidget);
}

void MainWindowSideBordersController::initializeBorderColor(QWidget & border)
{
    QWidget * pParentWidget = qobject_cast<QWidget*>(parent());
    if (Q_UNLIKELY(!pParentWidget)) {
        QNDEBUG(QStringLiteral("Can't cast the parent to QWidget, resetting the color to the default one"));
        setBorderStyleSheet(DEFAULT_MAIN_WINDOW_BORDER_COLOR, border);
        return;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QString panelStyle = appSettings.value(PANELS_STYLE_SETTINGS_KEY).toString();
    appSettings.endGroup();

    QNDEBUG(QStringLiteral("Panel style from app settings: ") << panelStyle);

    if (panelStyle == DARKER_PANEL_STYLE_NAME) {
        QColor color = pParentWidget->palette().color(QPalette::Dark);
        setBorderStyleSheet(color.name(), border);
        return;
    }

    if (panelStyle == LIGHTER_PANEL_STYLE_NAME) {
        QColor color = pParentWidget->palette().color(QPalette::Light);
        setBorderStyleSheet(color.name(), border);
        return;
    }

    setBorderStyleSheet(DEFAULT_MAIN_WINDOW_BORDER_COLOR, border);
}

int MainWindowSideBordersController::sanitizeWidth(const int width) const
{
    int sanitizedWidth = width;

    if (sanitizedWidth < BORDER_MIN_WIDTH) {
        sanitizedWidth = BORDER_MIN_WIDTH;
    }
    else if (sanitizedWidth > BORDER_MAX_WIDTH) {
        sanitizedWidth = BORDER_MAX_WIDTH;
    }

    return sanitizedWidth;
}

} // namespace quentier
