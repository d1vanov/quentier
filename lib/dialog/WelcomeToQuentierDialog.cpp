/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#include <QStringListModel>

namespace quentier {

WelcomeToQuentierDialog::WelcomeToQuentierDialog(QWidget * parent) :
    QDialog{parent}, m_ui{new Ui::WelcomeToQuentierDialog}
{
    m_ui->setupUi(this);
    setWindowTitle(tr("Welcome to Quentier"));

    QStringList evernoteServers;
    evernoteServers.reserve(2);
    evernoteServers << QStringLiteral("Evernote");
    evernoteServers << QStringLiteral("Yinxiang Biji");

    m_ui->evernoteServerComboBox->setModel(
        new QStringListModel(evernoteServers));

    QObject::connect(
        m_ui->continueWithLocalAccountPushButton, &QPushButton::clicked, this,
        &WelcomeToQuentierDialog::onContinueWithLocalAccountPushButtonPressed);

    QObject::connect(
        m_ui->loginToEvernoteAccountPushButton, &QPushButton::clicked, this,
        &WelcomeToQuentierDialog::onLogInToEvernoteAccountPushButtonPressed);
}

WelcomeToQuentierDialog::~WelcomeToQuentierDialog()
{
    delete m_ui;
}

QString WelcomeToQuentierDialog::evernoteServer() const
{
    switch (m_ui->evernoteServerComboBox->currentIndex()) {
    case 1:
        return QStringLiteral("app.yinxiang.com");
    default:
        return QStringLiteral("www.evernote.com");
    }
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
