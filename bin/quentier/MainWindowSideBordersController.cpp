/*
 * Copyright 2018-2019 Dmitry Ivanov
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

#include <lib/preferences/SettingsNames.h>
#include <lib/preferences/DefaultSettings.h>
#include <lib/preferences/PreferencesDialog.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QColor>
#include <QWidget>
#include <QSplitter>
#include <QMenu>

#include <cmath>
#include <cstdlib>

namespace quentier {

MainWindowSideBordersController::MainWindowSideBordersController(
        const Account & account, QWidget & leftBorder, QWidget & rightBorder,
        QSplitter & horizontalLayoutSplitter, QWidget & parent) :
    QObject(&parent),
    m_leftBorder(leftBorder),
    m_rightBorder(rightBorder),
    m_horizontalLayoutSplitter(horizontalLayoutSplitter),
    m_parent(parent),
    m_currentAccount(account),
    m_pLeftBorderContextMenu(Q_NULLPTR),
    m_pRightBorderContextMenu(Q_NULLPTR)
{
    initializeBordersState();

    QObject::connect(&m_leftBorder,
                     QNSIGNAL(QWidget,customContextMenuRequested,QPoint),
                     this,
                     QNSLOT(MainWindowSideBordersController,
                            onLeftBorderContextMenuRequested,QPoint));
    QObject::connect(&m_rightBorder,
                     QNSIGNAL(QWidget,customContextMenuRequested,QPoint),
                     this,
                     QNSLOT(MainWindowSideBordersController,
                            onRightBorderContextMenuRequested,QPoint));
}

void MainWindowSideBordersController::connectToPreferencesDialog(
    PreferencesDialog & dialog)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::"
                           "connectToPreferencesDialog"));

    QObject::connect(&dialog,
                     QNSIGNAL(PreferencesDialog,
                              showMainWindowLeftBorderOptionChanged,int),
                     this,
                     QNSLOT(MainWindowSideBordersController,
                            onShowLeftBorderOptionChanged,int),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog,
                     QNSIGNAL(PreferencesDialog,
                              showMainWindowRightBorderOptionChanged,int),
                     this,
                     QNSLOT(MainWindowSideBordersController,
                            onShowRightBorderOptionChanged,int),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog,
                     QNSIGNAL(PreferencesDialog,
                              mainWindowLeftBorderWidthChanged,int),
                     this,
                     QNSLOT(MainWindowSideBordersController,
                            onLeftBorderWidthChanged,int),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog,
                     QNSIGNAL(PreferencesDialog,
                              mainWindowRightBorderWidthChanged,int),
                     this,
                     QNSLOT(MainWindowSideBordersController,
                            onRightBorderWidthChanged,int),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog,
                     QNSIGNAL(PreferencesDialog,
                              mainWindowLeftBorderColorChanged,QString),
                     this,
                     QNSLOT(MainWindowSideBordersController,
                            onLeftBorderColorChanged,QString),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog,
                     QNSIGNAL(PreferencesDialog,
                              mainWindowRightBorderColorChanged,QString),
                     this,
                     QNSLOT(MainWindowSideBordersController,
                            onRightBorderColorChanged,QString),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
}

void MainWindowSideBordersController::persistCurrentBordersSizes()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::"
                           "persistCurrentBordersSizes"));

    QList<int> splitterSizes = m_horizontalLayoutSplitter.sizes();
    int splitterSizesCount = splitterSizes.count();
    if (Q_UNLIKELY(splitterSizesCount != 5))
    {
        QNWARNING(QStringLiteral("Internal error: can't persist the current main ")
                  << QStringLiteral("window's left & right borders size: expected 5 ")
                  << QStringLiteral("items within the horizontal splitter but got ")
                  << splitterSizesCount);
        return;
    }

    bool mainWindowIsMaximized = m_parent.windowState() & Qt::WindowMaximized;

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

    MainWindowSideBorderOption::type leftBorderOption =
        DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION;
    bool conversionResult = false;
    int leftBorderOptionInt =
        appSettings.value(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY)
        .toInt(&conversionResult);
    if (!conversionResult ||
        (leftBorderOptionInt < 0) ||
        (leftBorderOptionInt > 2))
    {
        QNINFO(QStringLiteral("Couldn't restore the persistent "
                              "\"Show left main window border\" option, "
                              "using the default one"));
    }
    else {
        leftBorderOption =
            static_cast<MainWindowSideBorderOption::type>(leftBorderOptionInt);
    }

    bool leftBorderIsDisplayed = false;
    if (leftBorderOption == MainWindowSideBorderOption::AlwaysShow) {
        leftBorderIsDisplayed = true;
    }
    else if (leftBorderOption == MainWindowSideBorderOption::ShowOnlyWhenMaximized) {
        leftBorderIsDisplayed = mainWindowIsMaximized;
    }

    MainWindowSideBorderOption::type rightBorderOption =
        DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION;
    conversionResult = false;
    int rightBorderOptionInt =
        appSettings.value(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY)
        .toInt(&conversionResult);
    if (!conversionResult ||
        (rightBorderOptionInt < 0) ||
        (rightBorderOptionInt > 2))
    {
        QNINFO(QStringLiteral("Couldn't restore the persistent "
                              "\"Show right main window border\" option, "
                              "using the default one"));
    }
    else {
        rightBorderOption =
            static_cast<MainWindowSideBorderOption::type>(rightBorderOptionInt);
    }

    bool rightBorderIsDisplayed = false;
    if (rightBorderOption == MainWindowSideBorderOption::AlwaysShow) {
        rightBorderIsDisplayed = true;
    }
    else if (rightBorderOption == MainWindowSideBorderOption::ShowOnlyWhenMaximized) {
        rightBorderIsDisplayed = mainWindowIsMaximized;
    }

    QNDEBUG(QStringLiteral("Show left border option = ") << leftBorderOption
            << QStringLiteral(", left border is displayed = ")
            << (leftBorderIsDisplayed
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", show right border option = ") << rightBorderOption
            << QStringLiteral(", right border is displayed = ")
            << (rightBorderIsDisplayed
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", main window is maximized = ")
            << (mainWindowIsMaximized
                ? QStringLiteral("true")
                : QStringLiteral("false"))
            << QStringLiteral(", left border splitter size = ") << splitterSizes[0]
            << QStringLiteral(", right border splitter size = ") << splitterSizes[4]);

    if (leftBorderIsDisplayed) {
        appSettings.setValue(LEFT_MAIN_WINDOW_BORDER_WIDTH_KEY, splitterSizes[0]);
    }

    if (rightBorderIsDisplayed) {
        appSettings.setValue(RIGHT_MAIN_WINDOW_BORDER_WIDTH_KEY, splitterSizes[4]);
    }

    appSettings.endGroup();
}

void MainWindowSideBordersController::onShowLeftBorderOptionChanged(int option)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::")
            << QStringLiteral("onShowLeftBorderOptionChanged: ") << option);
    toggleBorderDisplay(option, m_leftBorder);
}

void MainWindowSideBordersController::onShowRightBorderOptionChanged(int option)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::")
            << QStringLiteral("onShowRightBorderOptionChanged: ") << option);
    toggleBorderDisplay(option, m_rightBorder);
}

void MainWindowSideBordersController::onLeftBorderWidthChanged(int width)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::")
            << QStringLiteral("onLeftBorderWidthChanged: ") << width);
    onBorderWidthChanged(width, m_leftBorder);
}

void MainWindowSideBordersController::onRightBorderWidthChanged(int width)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::")
            << QStringLiteral("onRightBorderWidthChanged: ") << width);
    onBorderWidthChanged(width, m_rightBorder);
}

void MainWindowSideBordersController::onLeftBorderColorChanged(QString colorCode)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::")
            << QStringLiteral("onLeftBorderColorChanged: color code = ")
            << colorCode);
    onBorderColorChanged(colorCode, m_leftBorder);
}

void MainWindowSideBordersController::onRightBorderColorChanged(QString colorCode)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::")
            << QStringLiteral("onRightBorderColorChanged: color code = ")
            << colorCode);
    onBorderColorChanged(colorCode, m_rightBorder);
}

void MainWindowSideBordersController::onMainWindowMaximized()
{
    QNDEBUG(QStringLiteral("MainWindowSideBorderOption::onMainWindowMaximized"));
    initializeBordersState();
}

void MainWindowSideBordersController::onMainWindowUnmaximized()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::"
                           "onMainWindowUnmaximized"));
    initializeBordersState();
}

void MainWindowSideBordersController::setCurrentAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::setCurrentAccount: ")
            << account);

    if (m_currentAccount == account) {
        QNDEBUG(QStringLiteral("This account is already set, nothing to do"));
        return;
    }

    m_currentAccount = account;
    initializeBordersState();
}

void MainWindowSideBordersController::onPanelStyleChanged(QString panelStyle)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onPanelStyleChanged: ")
            << panelStyle);

    initializeBorderColor(panelStyle, m_leftBorder);
    initializeBorderColor(panelStyle, m_rightBorder);
}

void MainWindowSideBordersController::onLeftBorderContextMenuRequested(
    const QPoint & pos)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::"
                           "onLeftBorderContextMenuRequested"));

    delete m_pLeftBorderContextMenu;
    m_pLeftBorderContextMenu = new QMenu(&m_parent);

    onBorderContextMenuRequested(m_leftBorder, *m_pLeftBorderContextMenu, pos);
}

void MainWindowSideBordersController::onRightBorderContextMenuRequested(
    const QPoint & pos)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::"
                           "onRightBorderContextMenuRequested"));

    delete m_pRightBorderContextMenu;
    m_pRightBorderContextMenu = new QMenu(&m_parent);

    onBorderContextMenuRequested(m_rightBorder, *m_pRightBorderContextMenu, pos);
}

void MainWindowSideBordersController::onHideBorderRequestFromContextMenu()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::"
                           "onHideBorderRequestFromContextMenu"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        QNWARNING(QStringLiteral("Can't hide the main window border: internal "
                                 "error, can't cast the slot invoker to QAction"));
        return;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

    if (pAction->parent() == m_pLeftBorderContextMenu) {
        m_leftBorder.hide();
        appSettings.setValue(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY,
                             MainWindowSideBorderOption::NeverShow);
    }
    else if (pAction->parent() == m_pRightBorderContextMenu) {
        m_rightBorder.hide();
        appSettings.setValue(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY,
                             MainWindowSideBorderOption::NeverShow);
    }
    else {
        QNWARNING(QStringLiteral("Can't hide the main window border: internal "
                                 "error, can't figure out which border to hide "
                                 "by QAction causing the slot to invoke"));
    }

    appSettings.endGroup();
}

void MainWindowSideBordersController::onHideBorderWhenNotMaximizedRequestFromContextMenu()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::"
                           "onHideBorderWhenNotMaximizedRequestFromContextMenu"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        QNWARNING(QStringLiteral("Can't hide the main window border when it is "
                                 "not maximized: internal error, can't cast "
                                 "the slot invoker to QAction"));
        return;
    }

    bool mainWindowIsMaximized = m_parent.windowState() & Qt::WindowMaximized;

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

    if (pAction->parent() == m_pLeftBorderContextMenu)
    {
        if (!mainWindowIsMaximized) {
            m_leftBorder.hide();
        }

        appSettings.setValue(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY,
                             MainWindowSideBorderOption::ShowOnlyWhenMaximized);
    }
    else if (pAction->parent() == m_pRightBorderContextMenu)
    {
        if (!mainWindowIsMaximized) {
            m_rightBorder.hide();
        }

        appSettings.setValue(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY,
                             MainWindowSideBorderOption::ShowOnlyWhenMaximized);
    }
    else
    {
        QNWARNING(QStringLiteral("Can't hide the main window border: internal "
                                 "error, can't figure out which border to hide "
                                 "by QAction causing the slot to invoke"));
    }

    appSettings.endGroup();
}

void MainWindowSideBordersController::toggleBorderDisplay(const int option,
                                                          QWidget & border)
{
    MainWindowSideBorderOption::type borderOption =
        DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION;
    if ((option < 0) || (option > 2)) {
        QNWARNING(QStringLiteral("Can't process the toggle border display option, "
                                 "wrong option value: ") << option);
    }
    else {
        borderOption = static_cast<MainWindowSideBorderOption::type>(option);
    }

    bool mainWindowIsMaximized = m_parent.windowState() & Qt::WindowMaximized;
    setBorderDisplayedState(borderOption, mainWindowIsMaximized, border);
}

void MainWindowSideBordersController::onBorderWidthChanged(const int width,
                                                           QWidget & border)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onBorderWidthChanged: ")
            << QStringLiteral("width = ") << width);

    int sanitizedWidth = sanitizeWidth(width);

    QList<int> splitterSizes = m_horizontalLayoutSplitter.sizes();
    int splitterSizesCount = splitterSizes.count();
    if (Q_UNLIKELY(splitterSizesCount != 5))
    {
        QNWARNING(QStringLiteral("Internal error: can't set up the proper border ")
                  << QStringLiteral("width: expected 5 items within the ")
                  << QStringLiteral("horizontal splitter but got ")
                  << splitterSizesCount);
        return;
    }

    int originalBorderWidth = 0;
    if (&border == &m_leftBorder) {
        originalBorderWidth = splitterSizes[0];
    }
    else {
        originalBorderWidth = splitterSizes[4];
    }

    // Item with index 3 within the splitter is the widget containing note editor
    // tabs; will adjust its size in response to changing the border's width,
    // whether it's the left or right border
    int widthDiff = originalBorderWidth - sanitizedWidth;
    if (widthDiff == 0) {
        QNDEBUG(QStringLiteral("Border's width hasn't actually changed, "
                               "nothing to do"));
        return;
    }

    if (Q_UNLIKELY(std::abs(widthDiff) >= splitterSizes[3])) {
        QNWARNING(QStringLiteral("Detected inappropriate change in the width of "
                                 "the border: the diff is larger than the "
                                 "supposedly largert part of the main window's "
                                 "horizontal layout"));
        return;
    }

    QNDEBUG(QStringLiteral("Original left border width = ") << splitterSizes[0]
            << QStringLiteral(", original right border width = ") << splitterSizes[4]
            << QStringLiteral(", original note editor width = ") << splitterSizes[3]);

    splitterSizes[3] += widthDiff;

    if (&border == &m_leftBorder) {
        splitterSizes[0] = sanitizedWidth;
    }
    else {
        splitterSizes[4] = sanitizedWidth;
    }

    QNDEBUG(QStringLiteral("New left border width = ") << splitterSizes[0]
            << QStringLiteral(", new right border width = ") << splitterSizes[4]
            << QStringLiteral(", new note editor width = ") << splitterSizes[3]);

    m_horizontalLayoutSplitter.setSizes(splitterSizes);
}

void MainWindowSideBordersController::onBorderColorChanged(const QString & colorCode,
                                                           QWidget & border)
{
    QColor color(colorCode);
    if (color.isValid()) {
        setBorderStyleSheet(colorCode, border);
        return;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QString panelStyle = appSettings.value(PANELS_STYLE_SETTINGS_KEY).toString();
    appSettings.endGroup();

    initializeBorderColor(panelStyle, border);
}

void MainWindowSideBordersController::setBorderStyleSheet(const QString & colorCode,
                                                          QWidget & border)
{
    QString styleSheet = QStringLiteral("background-color: ");
    styleSheet += colorCode;
    styleSheet += QStringLiteral(";");

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    if (&border == &m_leftBorder) {
        styleSheet += QStringLiteral("border-right: 1px solid black;");
    }
    else {
        styleSheet += QStringLiteral("border-left: 1px solid black;");
    }
#endif

    if (&border == &m_leftBorder) {
        styleSheet += QStringLiteral("margin-right: 0px;");
    }
    else {
        styleSheet += QStringLiteral("margin-left: 0px;");
    }

    styleSheet += QStringLiteral("border-bottom: 1px solid black;");
    border.setStyleSheet(styleSheet);
}

void MainWindowSideBordersController::onBorderContextMenuRequested(
    QWidget & border, QMenu & menu, const QPoint & pos)
{
    QAction * pHideAction = new QAction(tr("Hide"), &menu);
    pHideAction->setEnabled(true);
    QObject::connect(pHideAction,
                     QNSIGNAL(QAction,triggered),
                     this,
                     QNSLOT(MainWindowSideBordersController,
                            onHideBorderRequestFromContextMenu));
    menu.addAction(pHideAction);

    bool mainWindowIsMaximized = m_parent.windowState() & Qt::WindowMaximized;
    if (!mainWindowIsMaximized)
    {
        QAction * pHideWhenNotMaximizedAction =
            new QAction(tr("Hide when not maximized"), &menu);
        pHideWhenNotMaximizedAction->setEnabled(true);
        QObject::connect(pHideWhenNotMaximizedAction,
                         QNSIGNAL(QAction,triggered),
                         this,
                         QNSLOT(MainWindowSideBordersController,
                                onHideBorderWhenNotMaximizedRequestFromContextMenu));
        menu.addAction(pHideWhenNotMaximizedAction);
    }

    menu.show();
    menu.exec(border.mapToGlobal(pos));
}

void MainWindowSideBordersController::initializeBordersState()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::"
                           "initializeBordersState"));

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

    MainWindowSideBorderOption::type leftBorderOption =
        DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION;
    bool conversionResult = false;
    int leftBorderOptionInt =
        appSettings.value(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY)
        .toInt(&conversionResult);
    if (!conversionResult ||
        (leftBorderOptionInt < 0) ||
        (leftBorderOptionInt > 2))
    {
        QNINFO(QStringLiteral("Couldn't restore the persistent "
                              "\"Show left main window border\" option, "
                              "using the default one"));
    }
    else {
        leftBorderOption =
            static_cast<MainWindowSideBorderOption::type>(leftBorderOptionInt);
    }

    MainWindowSideBorderOption::type rightBorderOption =
        DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION;
    conversionResult = false;
    int rightBorderOptionInt =
        appSettings.value(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY)
        .toInt(&conversionResult);
    if (!conversionResult ||
        (rightBorderOptionInt < 0) ||
        (rightBorderOptionInt > 2))
    {
        QNINFO(QStringLiteral("Couldn't restore the persistent "
                              "\"Show right main window border\" option, "
                              "using the default one"));
    }
    else {
        rightBorderOption =
            static_cast<MainWindowSideBorderOption::type>(rightBorderOptionInt);
    }

    conversionResult = false;
    int leftBorderWidth =
        appSettings.value(LEFT_MAIN_WINDOW_BORDER_WIDTH_KEY)
        .toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNDEBUG(QStringLiteral("Couldn't restore the persistent "
                               "\"Left main window border width\" option, "
                               "using the default one"));
        leftBorderWidth = 4;
    }

    conversionResult = false;
    int rightBorderWidth =
        appSettings.value(RIGHT_MAIN_WINDOW_BORDER_WIDTH_KEY)
        .toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNDEBUG(QStringLiteral("Couldn't restore the persistent "
                               "\"Right main window border width\" option, "
                               "using the default one"));
        rightBorderWidth = 4;
    }

    QString panelStyle = appSettings.value(PANELS_STYLE_SETTINGS_KEY).toString();

    appSettings.endGroup();

    leftBorderWidth = sanitizeWidth(leftBorderWidth);
    rightBorderWidth = sanitizeWidth(rightBorderWidth);

    QList<int> splitterSizes = m_horizontalLayoutSplitter.sizes();
    int splitterSizesCount = splitterSizes.count();
    if (splitterSizesCount == 5)
    {
        QNDEBUG(QStringLiteral("Original left border size = ") << splitterSizes[0]
                << QStringLiteral(", original right border size = ") << splitterSizes[4]
                << QStringLiteral(", original note editor size = ") << splitterSizes[3]
                << QStringLiteral(", new left border width = ") << leftBorderWidth
                << QStringLiteral(", new right border width = ") << rightBorderWidth);

        int leftBorderWidthDiff = splitterSizes[0] - leftBorderWidth;
        int rightBorderWidthDiff = splitterSizes[4] - rightBorderWidth;

        // Item with index 3 within the splitter is the widget containing note
        // editor tabs; will adjust its size in response to changing the width
        // or borders
        splitterSizes[3] += leftBorderWidthDiff;
        splitterSizes[3] += rightBorderWidthDiff;

        splitterSizes[0] = leftBorderWidth;
        splitterSizes[4] = rightBorderWidth;

        QNDEBUG(QStringLiteral("Left border splitter size after: ")
                << splitterSizes[0]
                << QStringLiteral(", right border splitter size after: ")
                << splitterSizes[4]
                << QStringLiteral(", note editor splitter size after: ")
                << splitterSizes[3]);

        m_horizontalLayoutSplitter.setSizes(splitterSizes);
    }
    else
    {
        QNWARNING(QStringLiteral("Internal error: can't set up the proper border ")
                  << QStringLiteral("width: expected 5 items within the ")
                  << QStringLiteral("horizontal splitter but got ")
                  << splitterSizesCount);
    }

    initializeBorderColor(panelStyle, m_leftBorder);
    initializeBorderColor(panelStyle, m_rightBorder);

    bool mainWindowIsMaximized = m_parent.windowState() & Qt::WindowMaximized;

    setBorderDisplayedState(leftBorderOption, mainWindowIsMaximized, m_leftBorder);
    setBorderDisplayedState(rightBorderOption, mainWindowIsMaximized, m_rightBorder);
}

void MainWindowSideBordersController::initializeBorderColor(
    const QString & panelStyle, QWidget & border)
{
    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

    QString overrideColorCode;
    if (&border == &m_leftBorder) {
        overrideColorCode =
            appSettings.value(LEFT_MAIN_WINDOW_BORDER_OVERRIDE_COLOR).toString();
    }
    else {
        overrideColorCode =
            appSettings.value(RIGHT_MAIN_WINDOW_BORDER_OVERRIDE_COLOR).toString();
    }

    appSettings.endGroup();

    if (!overrideColorCode.isEmpty() && QColor::isValidColor(overrideColorCode)) {
        setBorderStyleSheet(overrideColorCode, border);
        return;
    }

    if (panelStyle == DARKER_PANEL_STYLE_NAME) {
        QColor color = m_parent.palette().color(QPalette::Dark);
        setBorderStyleSheet(color.name(), border);
        return;
    }

    if (panelStyle == LIGHTER_PANEL_STYLE_NAME) {
        QColor color = m_parent.palette().color(QPalette::Light);
        setBorderStyleSheet(color.name(), border);
        return;
    }

    setBorderStyleSheet(DEFAULT_MAIN_WINDOW_BORDER_COLOR, border);
}

void MainWindowSideBordersController::setBorderDisplayedState(
    const MainWindowSideBorderOption::type option,
    const bool mainWindowIsMaximized, QWidget & border)
{
    if ( (option == MainWindowSideBorderOption::AlwaysShow) ||
         ((option == MainWindowSideBorderOption::ShowOnlyWhenMaximized) &&
          mainWindowIsMaximized) )
    {
        border.show();
    }
    else
    {
        border.hide();
    }
}

int MainWindowSideBordersController::sanitizeWidth(const int width) const
{
    int sanitizedWidth = width;

    if (sanitizedWidth < 0) {
        sanitizedWidth = 0;
    }
    else if (sanitizedWidth > MAX_MAIN_WINDOW_BORDER_SIZE) {
        sanitizedWidth = MAX_MAIN_WINDOW_BORDER_SIZE;
    }

    return sanitizedWidth;
}

} // namespace quentier
