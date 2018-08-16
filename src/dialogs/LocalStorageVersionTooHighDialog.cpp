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

#include "LocalStorageVersionTooHighDialog.h"
#include "ui_LocalStorageVersionTooHighDialog.h"
#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

LocalStorageVersionTooHighDialog::LocalStorageVersionTooHighDialog(const Account & currentAccount,
                                                                   LocalStorageManager & localStorageManager,
                                                                   QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::LocalStorageVersionTooHighDialog)
{
    m_pUi->setupUi(this);

    QDialog::setModal(true);

#ifdef Q_OS_MAC
    if (parent) {
        QDialog::setWindowModality(Qt::WindowModal);
    }
    else {
        QDialog::setWindowModality(Qt::ApplicationModal);
    }
#else
    QDialog::setWindowModality(Qt::ApplicationModal);
#endif

    QWidget::setWindowFlags(Qt::WindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint));

    initializeDetails(currentAccount, localStorageManager);
}

LocalStorageVersionTooHighDialog::~LocalStorageVersionTooHighDialog()
{
    delete m_pUi;
}

void LocalStorageVersionTooHighDialog::onSwitchToAccountPushButtonPressed()
{
    QNDEBUG(QStringLiteral("LocalStorageVersionTooHighDialog::onSwitchToAccountPushButtonPressed"));

    // TODO: implement
}

void LocalStorageVersionTooHighDialog::reject()
{
    // NOTE: doing nothing intentionally: LocalStorageVersionTooHighDialog cannot be rejected
}

void LocalStorageVersionTooHighDialog::createConnections()
{
    QNDEBUG(QStringLiteral("LocalStorageVersionTooHighDialog::createConnections"));

    QObject::connect(m_pUi->switchToAnotherAccountPushButton, QNSIGNAL(QPushButton,pressed),
                     this, QNSLOT(LocalStorageVersionTooHighDialog,onSwitchToAccountPushButtonPressed));
    // TODO: continue from here
}

void LocalStorageVersionTooHighDialog::initializeDetails(const Account & currentAccount,
                                                         LocalStorageManager & localStorageManager)
{
    QNDEBUG(QStringLiteral("LocalStorageVersionTooHighDialog::initializeDetails: current account = ")
            << currentAccount);

    QString details = tr("Account name");
    details += QStringLiteral(": ");
    details += currentAccount.name();
    details += QStringLiteral("\n");

    details += tr("Account display name");
    details += QStringLiteral(": ");
    details += currentAccount.displayName();
    details += QStringLiteral("\n");

    details += tr("Account type");
    details += QStringLiteral(": ");

    switch(currentAccount.type())
    {
    case Account::Type::Local:
        details += tr("local");
        break;
    case Account::Type::Evernote:
        details += QStringLiteral("Evernote");
        break;
    default:
        details += tr("Unknown");
        details += QStringLiteral(": ");
        details += QString::number(currentAccount.type());
        break;
    }

    details += QStringLiteral("\n");

    details += tr("Current local storage persistence version");
    details += QStringLiteral(": ");

    ErrorString errorDescription;
    int currentLocalStorageVersion = localStorageManager.localStorageVersion(errorDescription);
    if (Q_UNLIKELY(!errorDescription.isEmpty())) {
        details += tr("cannot determine");
        details += QStringLiteral(": ");
        details += errorDescription.localizedString();
    }
    else {
        details += QString::number(currentLocalStorageVersion);
    }

    details += QStringLiteral("\n");

    details += tr("Highest supported local storage persistence version");
    details += QStringLiteral(": ");
    details += QString::number(localStorageManager.highestSupportedLocalStorageVersion());
    details += QStringLiteral("\n");

    m_pUi->detailsPlainTextEdit->setPlainText(details);
}

} // namespace quentier
