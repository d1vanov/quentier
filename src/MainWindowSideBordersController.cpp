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
#include "SettingsNames.h"
#include "DefaultSettings.h"
#include "dialogs/PreferencesDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QColor>
#include <QWidget>
#include <QMenu>

#define BORDER_MIN_WIDTH (4)
#define BORDER_MAX_WIDTH (20)

namespace quentier {

MainWindowSideBordersController::MainWindowSideBordersController(const Account & account,
                                                                 QWidget & leftBorder, QWidget & rightBorder,
                                                                 QWidget & parent) :
    QObject(&parent),
    m_leftBorder(leftBorder),
    m_rightBorder(rightBorder),
    m_parent(parent),
    m_currentAccount(account),
    m_pLeftBorderContextMenu(Q_NULLPTR),
    m_pRightBorderContextMenu(Q_NULLPTR)
{
    initializeBordersState();
}

void MainWindowSideBordersController::connectToPreferencesDialog(PreferencesDialog & dialog)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::connectToPreferencesDialog"));

    QObject::connect(&dialog, QNSIGNAL(PreferencesDialog,showMainWindowLeftBorderOptionChanged,int),
                     this, QNSLOT(MainWindowSideBordersController,onShowLeftBorderOptionChanged,int),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog, QNSIGNAL(PreferencesDialog,showMainWindowRightBorderOptionChanged,int),
                     this, QNSLOT(MainWindowSideBordersController,onShowRightBorderOptionChanged,int),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog, QNSIGNAL(PreferencesDialog,mainWindowLeftBorderWidthChanged,int),
                     this, QNSLOT(MainWindowSideBordersController,onLeftBorderWidthChanged,int),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog, QNSIGNAL(PreferencesDialog,mainWindowRightBorderWidthChanged,int),
                     this, QNSLOT(MainWindowSideBordersController,onRightBorderWidthChanged,int),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog, QNSIGNAL(PreferencesDialog,mainWindowLeftBorderColorChanged,QString),
                     this, QNSLOT(MainWindowSideBordersController,onLeftBorderColorChanged,QString),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    QObject::connect(&dialog, QNSIGNAL(PreferencesDialog,mainWindowRightBorderColorChanged,QString),
                     this, QNSLOT(MainWindowSideBordersController,onRightBorderColorChanged,QString),
                     Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
}

void MainWindowSideBordersController::onShowLeftBorderOptionChanged(int option)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onShowLeftBorderOptionChanged: ") << option);
    toggleBorderDisplay(option, m_leftBorder);
}

void MainWindowSideBordersController::onShowRightBorderOptionChanged(int option)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onShowRightBorderOptionChanged: ") << option);
    toggleBorderDisplay(option, m_rightBorder);
}

void MainWindowSideBordersController::onLeftBorderWidthChanged(int width)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onLeftBorderWidthChanged: ") << width);
    onBorderWidthChanged(width, m_leftBorder);
}

void MainWindowSideBordersController::onRightBorderWidthChanged(int width)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onRightBorderWidthChanged: ") << width);
    onBorderWidthChanged(width, m_rightBorder);
}

void MainWindowSideBordersController::onLeftBorderColorChanged(QString colorCode)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onLeftBorderColorChanged: color code = ") << colorCode);
    onBorderColorChanged(colorCode, m_leftBorder);
}

void MainWindowSideBordersController::onRightBorderColorChanged(QString colorCode)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onRightBorderColorChanged: color code = ") << colorCode);
    onBorderColorChanged(colorCode, m_rightBorder);
}

void MainWindowSideBordersController::onMainWindowMaximized()
{
    QNDEBUG(QStringLiteral("MainWindowSideBorderOption::onMainWindowMaximized"));
    initializeBordersState();
}

void MainWindowSideBordersController::onMainWindowUnmaximized()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onMainWindowUnmaximized"));
    initializeBordersState();
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

void MainWindowSideBordersController::onPanelStyleChanged(QString panelStyle)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onPanelStyleChanged: ") << panelStyle);

    initializeBorderColor(panelStyle, m_leftBorder);
    initializeBorderColor(panelStyle, m_rightBorder);
}

void MainWindowSideBordersController::onLeftBorderContextMenuRequested(const QPoint & pos)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onLeftBorderContextMenuRequested"));

    delete m_pLeftBorderContextMenu;
    m_pLeftBorderContextMenu = new QMenu(&m_leftBorder);

    onBorderContextMenuRequested(m_leftBorder, *m_pLeftBorderContextMenu, pos);
}

void MainWindowSideBordersController::onRightBorderContextMenuRequested(const QPoint & pos)
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onRightBorderContextMenuRequested"));

    delete m_pRightBorderContextMenu;
    m_pRightBorderContextMenu = new QMenu(&m_rightBorder);

    onBorderContextMenuRequested(m_rightBorder, *m_pRightBorderContextMenu, pos);
}

void MainWindowSideBordersController::onHideBorderRequestFromContextMenu()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onHideBorderRequestFromContextMenu"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        QNWARNING(QStringLiteral("Can't hide the main window border: internal error, can't cast the slot invoker to QAction"));
        return;
    }

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

    if (pAction->parent() == m_pLeftBorderContextMenu) {
        m_leftBorder.hide();
        appSettings.setValue(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY, MainWindowSideBorderOption::NeverShow);
    }
    else if (pAction->parent() == m_pRightBorderContextMenu) {
        m_rightBorder.hide();
        appSettings.setValue(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY, MainWindowSideBorderOption::NeverShow);
    }
    else {
        QNWARNING(QStringLiteral("Can't hide the main window border: internal error, can't figure out which border to hide "
                                 "by QAction causing the slot to invoke"));
    }

    appSettings.endGroup();
}

void MainWindowSideBordersController::onHideBorderWhenNotMaximizedRequestFromContextMenu()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::onHideBorderWhenNotMaximizedRequestFromContextMenu"));

    QAction * pAction = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!pAction)) {
        QNWARNING(QStringLiteral("Can't hide the main window border when it is not maximized: internal error, "
                                 "can't cast the slot invoker to QAction"));
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

        appSettings.setValue(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY, MainWindowSideBorderOption::ShowOnlyWhenExpanded);
    }
    else if (pAction->parent() == m_pRightBorderContextMenu)
    {
        if (!mainWindowIsMaximized) {
            m_rightBorder.hide();
        }

        appSettings.setValue(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY, MainWindowSideBorderOption::ShowOnlyWhenExpanded);
    }
    else
    {
        QNWARNING(QStringLiteral("Can't hide the main window border: internal error, can't figure out which border to hide "
                                 "by QAction causing the slot to invoke"));
    }

    appSettings.endGroup();
}

void MainWindowSideBordersController::toggleBorderDisplay(const int option, QWidget & border)
{
    MainWindowSideBorderOption::type borderOption = MainWindowSideBorderOption::ShowOnlyWhenExpanded;
    if ((option < 0) || (option > 2)) {
        QNWARNING(QStringLiteral("Can't process the toggle border display option, wrong option value: ")
                  << option << QStringLiteral(", fallback to showing only when expanded"));
    }
    else {
        borderOption = static_cast<MainWindowSideBorderOption::type>(option);
    }

    bool mainWindowIsMaximized = m_parent.windowState() & Qt::WindowMaximized;
    setBorderDisplayedState(borderOption, mainWindowIsMaximized, border);
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

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);
    QString panelStyle = appSettings.value(PANELS_STYLE_SETTINGS_KEY).toString();
    appSettings.endGroup();

    initializeBorderColor(panelStyle, border);
}

void MainWindowSideBordersController::setBorderStyleSheet(const QString & colorCode, QWidget & border)
{
    QString styleSheet = QStringLiteral("background-color: ");
    styleSheet += colorCode;
    styleSheet += QStringLiteral("; border-right: 1px solid black; margin-right: 0px;");
    border.setStyleSheet(styleSheet);
}

void MainWindowSideBordersController::onBorderContextMenuRequested(QWidget & border, QMenu & menu, const QPoint & pos)
{
    QAction * pHideAction = new QAction(tr("Hide"), &menu);
    pHideAction->setEnabled(true);
    QObject::connect(pHideAction, QNSIGNAL(QAction,triggered), this, QNSLOT(MainWindowSideBordersController,onHideBorderRequestFromContextMenu));
    menu.addAction(pHideAction);

    bool mainWindowIsMaximized = m_parent.windowState() & Qt::WindowMaximized;
    if (!mainWindowIsMaximized)
    {
        QAction * pHideWhenNotMaximizedAction = new QAction(tr("Hide when not maximized"), &menu);
        pHideWhenNotMaximizedAction->setEnabled(true);
        QObject::connect(pHideWhenNotMaximizedAction, QNSIGNAL(QAction,triggered),
                         this, QNSLOT(MainWindowSideBordersController,onHideBorderWhenNotMaximizedRequestFromContextMenu));
        menu.addAction(pHideWhenNotMaximizedAction);
    }

    menu.show();
    menu.exec(border.mapToGlobal(pos));
}

void MainWindowSideBordersController::initializeBordersState()
{
    QNDEBUG(QStringLiteral("MainWindowSideBordersController::initializeBordersState"));

    ApplicationSettings appSettings(m_currentAccount, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(LOOK_AND_FEEL_SETTINGS_GROUP_NAME);

    MainWindowSideBorderOption::type leftBorderOption = DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION;
    bool conversionResult = false;
    int leftBorderOptionInt = appSettings.value(SHOW_LEFT_MAIN_WINDOW_BORDER_OPTION_KEY).toInt(&conversionResult);
    if (!conversionResult || (leftBorderOptionInt < 0) || (leftBorderOptionInt > 2)) {
        QNINFO(QStringLiteral("Couldn't restore the persistent \"Show left main window border\" option, "
                              "using the default one"));
    }
    else {
        leftBorderOption = static_cast<MainWindowSideBorderOption::type>(leftBorderOptionInt);
    }

    MainWindowSideBorderOption::type rightBorderOption = DEFAULT_SHOW_MAIN_WINDOW_BORDER_OPTION;
    conversionResult = false;
    int rightBorderOptionInt = appSettings.value(SHOW_RIGHT_MAIN_WINDOW_BORDER_OPTION_KEY).toInt(&conversionResult);
    if (!conversionResult || (rightBorderOptionInt < 0) || (rightBorderOptionInt > 2)) {
        QNINFO(QStringLiteral("Couldn't restore the persistent \"Show right main window border\" option, "
                              "using the default one"));
    }
    else {
        rightBorderOption = static_cast<MainWindowSideBorderOption::type>(rightBorderOptionInt);
    }

    conversionResult = false;
    int leftBorderWidth = appSettings.value(LEFT_MAIN_WINDOW_BORDER_WIDTH_KEY).toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNDEBUG(QStringLiteral("Couldn't restore the persistent \"Left main window border width\" option, "
                               "using the default one"));
        leftBorderWidth = 4;
    }

    conversionResult = false;
    int rightBorderWidth = appSettings.value(RIGHT_MAIN_WINDOW_BORDER_WIDTH_KEY).toInt(&conversionResult);
    if (Q_UNLIKELY(!conversionResult)) {
        QNDEBUG(QStringLiteral("Couldn't restore the persistent \"Right main window border width\" option, "
                               "using the default one"));
        rightBorderWidth = 4;
    }

    QString panelStyle = appSettings.value(PANELS_STYLE_SETTINGS_KEY).toString();

    appSettings.endGroup();

    m_leftBorder.resize(sanitizeWidth(leftBorderWidth), m_leftBorder.height());
    m_rightBorder.resize(sanitizeWidth(rightBorderWidth), m_rightBorder.height());

    initializeBorderColor(panelStyle, m_leftBorder);
    initializeBorderColor(panelStyle, m_rightBorder);

    bool mainWindowIsMaximized = m_parent.windowState() & Qt::WindowMaximized;

    setBorderDisplayedState(leftBorderOption, mainWindowIsMaximized, m_leftBorder);
    setBorderDisplayedState(rightBorderOption, mainWindowIsMaximized, m_rightBorder);
}

void MainWindowSideBordersController::initializeBorderColor(const QString & panelStyle, QWidget & border)
{
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

void MainWindowSideBordersController::setBorderDisplayedState(const MainWindowSideBorderOption::type option,
                                                              const bool mainWindowIsMaximized, QWidget & border)
{
    if ( (option == MainWindowSideBorderOption::AlwaysShow) ||
         ((option == MainWindowSideBorderOption::ShowOnlyWhenExpanded) && mainWindowIsMaximized) )
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

    if (sanitizedWidth < BORDER_MIN_WIDTH) {
        sanitizedWidth = BORDER_MIN_WIDTH;
    }
    else if (sanitizedWidth > BORDER_MAX_WIDTH) {
        sanitizedWidth = BORDER_MAX_WIDTH;
    }

    return sanitizedWidth;
}

} // namespace quentier
