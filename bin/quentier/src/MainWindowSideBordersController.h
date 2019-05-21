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

#ifndef QUENTIER_MAIN_WINDOW_SIDE_BORDERS_CONTROLLER_H
#define QUENTIER_MAIN_WINDOW_SIDE_BORDERS_CONTROLLER_H

#include <lib/utility/MainWindowSideBorderOption.h>

#include <quentier/utility/Macros.h>
#include <quentier/types/Account.h>

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QSplitter)
QT_FORWARD_DECLARE_CLASS(QMenu)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(PreferencesDialog)

/**
 * The MainWindowSideBordersController class is a small helper class
 * managing MainWindow's left and right borders: whether they are displayed
 * and when they are, what width and color should be used for them etc
 *
 * The sole purpose of delegating this into a separate class is to put
 * some load off MainWindow class as it's complex enough as it is
 */
class MainWindowSideBordersController: public QObject
{
    Q_OBJECT
public:
    explicit MainWindowSideBordersController(const Account & account,
                                             QWidget & leftBorder,
                                             QWidget & rightBorder,
                                             QSplitter & horizontalLayoutSplitter,
                                             QWidget & parent);

    void connectToPreferencesDialog(PreferencesDialog & dialog);

    void persistCurrentBordersSizes();

public Q_SLOTS:
    void onShowLeftBorderOptionChanged(int option);
    void onShowRightBorderOptionChanged(int option);

    void onLeftBorderWidthChanged(int width);
    void onRightBorderWidthChanged(int width);

    void onLeftBorderColorChanged(QString colorCode);
    void onRightBorderColorChanged(QString colorCode);

    void onMainWindowMaximized();
    void onMainWindowUnmaximized();

    void setCurrentAccount(const Account & account);
    void onPanelStyleChanged(QString panelStyle);

private Q_SLOTS:
    void onLeftBorderContextMenuRequested(const QPoint & pos);
    void onRightBorderContextMenuRequested(const QPoint & pos);

    void onHideBorderRequestFromContextMenu();
    void onHideBorderWhenNotMaximizedRequestFromContextMenu();

private:
    void toggleBorderDisplay(const int option, QWidget & border);
    void onBorderWidthChanged(const int width, QWidget & border);
    void onBorderColorChanged(const QString & colorCode, QWidget & border);
    void setBorderStyleSheet(const QString & colorCode, QWidget & border);

    void onBorderContextMenuRequested(QWidget & border, QMenu & menu,
                                      const QPoint & pos);

    void initializeBordersState();
    void initializeBorderColor(const QString & panelStyle, QWidget & border);
    void setBorderDisplayedState(const MainWindowSideBorderOption::type option,
                                 const bool mainWindowIsMaximized,
                                 QWidget & border);

    int sanitizeWidth(const int width) const;

private:
    QWidget &               m_leftBorder;
    QWidget &               m_rightBorder;
    QSplitter &             m_horizontalLayoutSplitter;
    QWidget &               m_parent;

    Account                 m_currentAccount;

    QMenu *                 m_pLeftBorderContextMenu;
    QMenu *                 m_pRightBorderContextMenu;
};

} // namespace quentier

#endif // QUENTIER_MAIN_WINDOW_SIDE_BORDERS_CONTROLLER_H
