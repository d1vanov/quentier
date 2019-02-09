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

#include "FirstShutdownDialog.h"
#include "ui_FirstShutdownDialog.h"

namespace quentier {

FirstShutdownDialog::FirstShutdownDialog(QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::FirstShutdownDialog)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Keep the app running or quit?"));

    QObject::connect(m_pUi->closeToTrayPushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(FirstShutdownDialog,onCloseToTrayPushButtonPressed));
    QObject::connect(m_pUi->closePushButton,
                     QNSIGNAL(QPushButton,clicked),
                     this,
                     QNSLOT(FirstShutdownDialog,onClosePushButtonPressed));
}

FirstShutdownDialog::~FirstShutdownDialog()
{
    delete m_pUi;
}

void FirstShutdownDialog::onCloseToTrayPushButtonPressed()
{
    QDialog::accept();
}

void FirstShutdownDialog::onClosePushButtonPressed()
{
    QDialog::reject();
}

} // namespace quentier
