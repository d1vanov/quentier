/*
 * Copyright 2018-2020 Dmitry Ivanov
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

#include <lib/account/AccountFilterModel.h>
#include <lib/account/AccountModel.h>

#include <quentier/local_storage/LocalStorageManager.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>

#include <QItemSelection>

namespace quentier {

LocalStorageVersionTooHighDialog::LocalStorageVersionTooHighDialog(
    const Account & currentAccount, AccountModel & accountModel,
    LocalStorageManager & localStorageManager, QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::LocalStorageVersionTooHighDialog),
    m_pAccountFilterModel(new AccountFilterModel(this))
{
    m_pUi->setupUi(this);
    m_pUi->statusBar->hide();
    m_pUi->accountsTableView->verticalHeader()->hide();
    m_pUi->switchToAnotherAccountPushButton->setEnabled(false);

    m_pAccountFilterModel->setSourceModel(&accountModel);
    m_pAccountFilterModel->addFilteredAccount(currentAccount);
    m_pUi->accountsTableView->setModel(m_pAccountFilterModel);

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

    QWidget::setWindowFlags(Qt::WindowFlags(
        Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint));

    initializeDetails(currentAccount, localStorageManager);
    createConnections();
}

LocalStorageVersionTooHighDialog::~LocalStorageVersionTooHighDialog()
{
    delete m_pUi;
}

void LocalStorageVersionTooHighDialog::onSwitchToAccountPushButtonPressed()
{
    QNDEBUG(
        "dialog",
        "LocalStorageVersionTooHighDialog::onSwitchToAccountPushButtonPressed");

    auto currentAccountIndex = m_pUi->accountsTableView->currentIndex();
    if (!currentAccountIndex.isValid()) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("No account is selected")));
        return;
    }

    auto sourceAccountModelIndex =
        m_pAccountFilterModel->mapToSource(currentAccountIndex);

    if (Q_UNLIKELY(!sourceAccountModelIndex.isValid())) {
        setErrorToStatusBar(ErrorString(
            QT_TR_NOOP("Internal error: could not figure out the selected "
                       "account")));
        return;
    }

    const auto * pAccountModel = qobject_cast<const AccountModel *>(
        m_pAccountFilterModel->sourceModel());

    if (Q_UNLIKELY(!pAccountModel)) {
        setErrorToStatusBar(
            ErrorString(QT_TR_NOOP("Internal error: account "
                                   "model is not set")));
        return;
    }

    const auto & accounts = pAccountModel->accounts();
    int row = sourceAccountModelIndex.row();
    if (Q_UNLIKELY((row < 0) || (row >= accounts.size()))) {
        setErrorToStatusBar(
            ErrorString(QT_TR_NOOP("Internal error: wrong row is "
                                   "selected in the accounts table")));
        return;
    }

    const Account & newAccount = accounts.at(row);
    Q_EMIT shouldSwitchToAccount(newAccount);
    QDialog::accept();
}

void LocalStorageVersionTooHighDialog::onCreateNewAccountButtonPressed()
{
    QNDEBUG(
        "dialog",
        "LocalStorageVersionTooHighDialog::onCreateNewAccountButtonPressed");

    Q_EMIT shouldCreateNewAccount();
    QDialog::accept();
}

void LocalStorageVersionTooHighDialog::onQuitAppButtonPressed()
{
    QNDEBUG(
        "dialog", "LocalStorageVersionTooHighDialog::onQuitAppButtonPressed");

    Q_EMIT shouldQuitApp();
    QDialog::accept();
}

void LocalStorageVersionTooHighDialog::onAccountViewSelectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNDEBUG(
        "dialog",
        "LocalStorageVersionTooHighDialog::onAccountViewSelectionChanged");

    Q_UNUSED(deselected)

    if (selected.isEmpty()) {
        QNDEBUG(
            "dialog",
            "No selection, disabling switch to selected account "
                << "button");
        m_pUi->switchToAnotherAccountPushButton->setEnabled(false);
        return;
    }

    m_pUi->switchToAnotherAccountPushButton->setEnabled(true);
}

void LocalStorageVersionTooHighDialog::reject()
{
    // NOTE: doing nothing intentionally: LocalStorageVersionTooHighDialog
    // cannot be rejected
}

void LocalStorageVersionTooHighDialog::createConnections()
{
    QNDEBUG("dialog", "LocalStorageVersionTooHighDialog::createConnections");

    QObject::connect(
        m_pUi->switchToAnotherAccountPushButton, &QPushButton::pressed, this,
        &LocalStorageVersionTooHighDialog::onSwitchToAccountPushButtonPressed);

    QObject::connect(
        m_pUi->accountsTableView->selectionModel(),
        &QItemSelectionModel::selectionChanged, this,
        &LocalStorageVersionTooHighDialog::onAccountViewSelectionChanged);

    QObject::connect(
        m_pUi->createNewAccountPushButton, &QPushButton::pressed, this,
        &LocalStorageVersionTooHighDialog::onCreateNewAccountButtonPressed);

    QObject::connect(
        m_pUi->quitAppPushButton, &QPushButton::pressed, this,
        &LocalStorageVersionTooHighDialog::onQuitAppButtonPressed);
}

void LocalStorageVersionTooHighDialog::initializeDetails(
    const Account & currentAccount, LocalStorageManager & localStorageManager)
{
    QNDEBUG(
        "dialog",
        "LocalStorageVersionTooHighDialog::initializeDetails: "
            << "current account = " << currentAccount);

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

    details += ToString(currentAccount.type());
    details += QStringLiteral("\n");

    details += tr("Current local storage persistence version");
    details += QStringLiteral(": ");

    ErrorString errorDescription;

    int currentLocalStorageVersion =
        localStorageManager.localStorageVersion(errorDescription);

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

    details += QString::number(
        localStorageManager.highestSupportedLocalStorageVersion());

    details += QStringLiteral("\n");

    m_pUi->detailsPlainTextEdit->setPlainText(details);
}

void LocalStorageVersionTooHighDialog::setErrorToStatusBar(
    const ErrorString & error)
{
    QNDEBUG(
        "dialog",
        "LocalStorageVersionTooHighDialog::setErrorToStatusBar: " << error);

    m_pUi->statusBar->setText(error.localizedString());
    m_pUi->statusBar->show();
}

} // namespace quentier
