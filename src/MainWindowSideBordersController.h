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

#ifndef QUENTIER_MAIN_WINDOW_SIDE_BORDERS_CONTROLLER_H
#define QUENTIER_MAIN_WINDOW_SIDE_BORDERS_CONTROLLER_H

#include <quentier/utility/Macros.h>
#include <quentier/types/Account.h>
#include <QObject>

namespace Ui {
class MainWindow;
}

QT_FORWARD_DECLARE_CLASS(QWidget);

namespace quentier {

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
                                             Ui::MainWindow & ui,
                                             QObject * parent = Q_NULLPTR);

public Q_SLOTS:
    void toggleLeftBorderDisplay(bool on);
    void toggleRightBorderDisplay(bool on);

    void onLeftBorderWidthChanged(int width);
    void onRightBorderWidthChanged(int width);

    void onLeftBorderColorChanged(QString colorCode);
    void onRightBorderColorChanged(QString colorCode);

    void onMainWindowExpanded();
    void onMainWindowUnexpanded();

    void setCurrentAccount(const Account & account);

private:
    void toggleBorderDisplay(const bool on, QWidget & border);
    void onBorderWidthChanged(const int width, QWidget & border);
    void onBorderColorChanged(const QString & colorCode, QWidget & border);
    void setBorderStyleSheet(const QString & colorCode, QWidget & border);

    void initializeBordersState();
    void initializeBorderColor(QWidget & border);

    int sanitizeWidth(const int width) const;

private:
    Ui::MainWindow &        m_ui;
    Account                 m_currentAccount;
};

} // namespace quentier

#endif // QUENTIER_MAIN_WINDOW_SIDE_BORDERS_CONTROLLER_H
