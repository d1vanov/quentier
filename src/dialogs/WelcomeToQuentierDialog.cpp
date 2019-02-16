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

#include "WelcomeToQuentierDialog.h"
#include "ui_WelcomeToQuentierDialog.h"

namespace quentier {

WelcomeToQuentierDialog::WelcomeToQuentierDialog(QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::WelcomeToQuentierDialog)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Welcome to Quentier"));

    QObject::connect(m_pUi->continueWithLocalAccountPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(WelcomeToQuentierDialog,
                            onContinueWithLocalAccountPushButtonPressed));
    QObject::connect(m_pUi->loginToEvernoteAccountPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(WelcomeToQuentierDialog,
                            onLogInToEvernoteAccountPushButtonPressed));
}

WelcomeToQuentierDialog::~WelcomeToQuentierDialog()
{
    delete m_pUi;
}

void WelcomeToQuentierDialog::onContinueWithLocalAccountPushButtonPressed()
{
    QDialog::reject();
}

void WelcomeToQuentierDialog::onLogInToEvernoteAccountPushButtonPressed()
{
    QDialog::accept();
}

} // namespace quentier
