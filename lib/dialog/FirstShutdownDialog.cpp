/*
 * Copyright 2018-2024 Dmitry Ivanov
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
    QDialog{parent}, m_ui{new Ui::FirstShutdownDialog}
{
    m_ui->setupUi(this);
    setWindowTitle(tr("Keep the app running or quit?"));

    QObject::connect(
        m_ui->closeToTrayPushButton, &QPushButton::clicked, this,
        &FirstShutdownDialog::onCloseToTrayPushButtonPressed);

    QObject::connect(
        m_ui->closePushButton, &QPushButton::clicked, this,
        &FirstShutdownDialog::onClosePushButtonPressed);
}

FirstShutdownDialog::~FirstShutdownDialog()
{
    delete m_ui;
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
