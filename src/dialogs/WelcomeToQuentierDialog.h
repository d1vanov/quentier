/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#ifndef QUENTIER_DIALOGS_WELCOME_TO_QUENTIER_DIALOG_H
#define QUENTIER_DIALOGS_WELCOME_TO_QUENTIER_DIALOG_H

#include <quentier/utility/Macros.h>
#include <QDialog>

namespace Ui {
class WelcomeToQuentierDialog;
}

namespace quentier {

class WelcomeToQuentierDialog: public QDialog
{
    Q_OBJECT
public:
    explicit WelcomeToQuentierDialog(QWidget * parent = Q_NULLPTR);
    ~WelcomeToQuentierDialog();

private Q_SLOTS:
    void onContinueWithLocalAccountPushButtonPressed();
    void onLogInToEvernoteAccountPushButtonPressed();

private:
    Ui::WelcomeToQuentierDialog *   m_pUi;
};

} // namespace quentier

#endif // QUENTIER_DIALOGS_WELCOME_TO_QUENTIER_DIALOG_H
