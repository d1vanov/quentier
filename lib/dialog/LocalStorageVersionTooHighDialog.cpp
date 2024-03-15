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

#include "LocalStorageVersionTooHighDialog.h"
#include "ui_LocalStorageVersionTooHighDialog.h"

#include <lib/account/AccountFilterModel.h>
#include <lib/account/AccountModel.h>
#include <lib/exception/Utils.h>

#include <quentier/exception/InvalidArgument.h>
#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/types/ErrorString.h>

#include <QItemSelection>
#include <QTextStream>

namespace quentier {

LocalStorageVersionTooHighDialog::LocalStorageVersionTooHighDialog(
    const Account & currentAccount, AccountModel & accountModel,
    local_storage::ILocalStoragePtr localStorage, QWidget * parent) :
    QDialog{parent},
    m_localStorage{std::move(localStorage)},
    m_ui{new Ui::LocalStorageVersionTooHighDialog},
    m_accountFilterModel{new AccountFilterModel(this)}
{
    if (Q_UNLIKELY(!m_localStorage)) {
        throw InvalidArgument{ErrorString{
            "LocalStorageVersionTooHighDialog ctor: local storage is null"}};
    }

    m_ui->setupUi(this);
    m_ui->statusBar->hide();
    m_ui->accountsTableView->verticalHeader()->hide();
    m_ui->switchToAnotherAccountPushButton->setEnabled(false);

    m_accountFilterModel->setSourceModel(&accountModel);
    m_accountFilterModel->addFilteredAccount(currentAccount);
    m_ui->accountsTableView->setModel(m_accountFilterModel);

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

    QWidget::setWindowFlags(Qt::WindowFlags{
        Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint});

    initializeDetails(currentAccount);
    createConnections();
}

LocalStorageVersionTooHighDialog::~LocalStorageVersionTooHighDialog()
{
    delete m_ui;
}

void LocalStorageVersionTooHighDialog::onSwitchToAccountPushButtonPressed()
{
    QNDEBUG(
        "dialog::LocalStorageVersionTooHighDialog",
        "LocalStorageVersionTooHighDialog::onSwitchToAccountPushButtonPressed");

    const auto currentAccountIndex = m_ui->accountsTableView->currentIndex();
    if (!currentAccountIndex.isValid()) {
        setErrorToStatusBar(ErrorString{QT_TR_NOOP("No account is selected")});
        return;
    }

    const auto sourceAccountModelIndex =
        m_accountFilterModel->mapToSource(currentAccountIndex);

    if (Q_UNLIKELY(!sourceAccountModelIndex.isValid())) {
        setErrorToStatusBar(ErrorString{
            QT_TR_NOOP("Internal error: could not figure out the selected "
                       "account")});
        return;
    }

    const auto * accountModel = qobject_cast<const AccountModel *>(
        m_accountFilterModel->sourceModel());

    if (Q_UNLIKELY(!accountModel)) {
        setErrorToStatusBar(ErrorString{
            QT_TR_NOOP("Internal error: account model is not set")});
        return;
    }

    const auto & accounts = accountModel->accounts();
    const int row = sourceAccountModelIndex.row();
    if (Q_UNLIKELY((row < 0) || (row >= accounts.size()))) {
        setErrorToStatusBar(ErrorString{
            QT_TR_NOOP("Internal error: wrong row is selected in the accounts "
                       "table")});
        return;
    }

    const Account & newAccount = accounts.at(row);
    Q_EMIT shouldSwitchToAccount(newAccount);
    QDialog::accept();
}

void LocalStorageVersionTooHighDialog::onCreateNewAccountButtonPressed()
{
    QNDEBUG(
        "dialog::LocalStorageVersionTooHighDialog",
        "LocalStorageVersionTooHighDialog::onCreateNewAccountButtonPressed");

    Q_EMIT shouldCreateNewAccount();
    QDialog::accept();
}

void LocalStorageVersionTooHighDialog::onQuitAppButtonPressed()
{
    QNDEBUG(
        "dialog::LocalStorageVersionTooHighDialog",
        "LocalStorageVersionTooHighDialog::onQuitAppButtonPressed");

    Q_EMIT shouldQuitApp();
    QDialog::accept();
}

void LocalStorageVersionTooHighDialog::onAccountViewSelectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNDEBUG(
        "dialog::LocalStorageVersionTooHighDialog",
        "LocalStorageVersionTooHighDialog::onAccountViewSelectionChanged");

    Q_UNUSED(deselected)

    if (selected.isEmpty()) {
        QNDEBUG(
            "dialog::LocalStorageVersionTooHighDialog",
            "No selection, disabling switch to selected account button");
        m_ui->switchToAnotherAccountPushButton->setEnabled(false);
        return;
    }

    m_ui->switchToAnotherAccountPushButton->setEnabled(true);
}

void LocalStorageVersionTooHighDialog::reject()
{
    // NOTE: doing nothing intentionally: LocalStorageVersionTooHighDialog
    // cannot be rejected
}

void LocalStorageVersionTooHighDialog::createConnections()
{
    QNDEBUG(
        "dialog::LocalStorageVersionTooHighDialog",
        "LocalStorageVersionTooHighDialog::createConnections");

    QObject::connect(
        m_ui->switchToAnotherAccountPushButton, &QPushButton::pressed, this,
        &LocalStorageVersionTooHighDialog::onSwitchToAccountPushButtonPressed);

    QObject::connect(
        m_ui->accountsTableView->selectionModel(),
        &QItemSelectionModel::selectionChanged, this,
        &LocalStorageVersionTooHighDialog::onAccountViewSelectionChanged);

    QObject::connect(
        m_ui->createNewAccountPushButton, &QPushButton::pressed, this,
        &LocalStorageVersionTooHighDialog::onCreateNewAccountButtonPressed);

    QObject::connect(
        m_ui->quitAppPushButton, &QPushButton::pressed, this,
        &LocalStorageVersionTooHighDialog::onQuitAppButtonPressed);
}

void LocalStorageVersionTooHighDialog::initializeDetails(
    const Account & currentAccount)
{
    QNDEBUG(
        "dialog::LocalStorageVersionTooHighDialog",
        "LocalStorageVersionTooHighDialog::initializeDetails: "
            << "current account = " << currentAccount);

    QString details;
    QTextStream strm{&details};

    strm << tr("Account name");
    strm << ": ";
    strm << currentAccount.name();
    strm << "\n";

    strm << tr("Account display name");
    strm << ": ";
    strm << currentAccount.displayName();
    strm << "\n";

    strm << tr("Account type");
    strm << ": ";

    strm << ToString(currentAccount.type());
    strm << "\n";

    strm << tr("Current local storage persistence version");
    strm << ": ";

    auto currentLocalStorageVersionFuture = m_localStorage->version();

    ErrorString errorDescription;
    try {
        currentLocalStorageVersionFuture.waitForFinished();
    }
    catch (const QException & e) {
        errorDescription = exceptionMessage(e);
    }

    if (Q_UNLIKELY(!errorDescription.isEmpty())) {
        strm << tr("cannot determine");
        strm << ": ";
        strm << errorDescription.localizedString();
    }
    else {
        Q_ASSERT(currentLocalStorageVersionFuture.resultCount() == 1);
        strm << currentLocalStorageVersionFuture.result();
    }

    strm << "\n";

    strm << tr("Highest supported local storage persistence version");
    strm << ": ";

    auto highestSupportedLocalStorageVersionFuture =
        m_localStorage->highestSupportedVersion();

    errorDescription.clear();
    try {
        highestSupportedLocalStorageVersionFuture.waitForFinished();
    }
    catch (const QException & e) {
        errorDescription = exceptionMessage(e);
    }

    if (Q_UNLIKELY(!errorDescription.isEmpty())) {
        strm << tr("cannot determine");
        strm << ": ";
        strm << errorDescription.localizedString();
    }
    else {
        Q_ASSERT(highestSupportedLocalStorageVersionFuture.resultCount() == 1);
        strm << highestSupportedLocalStorageVersionFuture.result();
    }

    strm << "\n";
    strm.flush();

    m_ui->detailsPlainTextEdit->setPlainText(details);
}

void LocalStorageVersionTooHighDialog::setErrorToStatusBar(
    const ErrorString & error)
{
    QNDEBUG(
        "dialog::LocalStorageVersionTooHighDialog",
        "LocalStorageVersionTooHighDialog::setErrorToStatusBar: " << error);

    m_ui->statusBar->setText(error.localizedString());
    m_ui->statusBar->show();
}

} // namespace quentier
