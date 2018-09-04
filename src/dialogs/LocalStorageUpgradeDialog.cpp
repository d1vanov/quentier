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

#include "LocalStorageUpgradeDialog.h"
#include "ui_LocalStorageUpgradeDialog.h"
#include "../models/AccountFilterModel.h"
#include "../models/AccountModel.h"
#include <quentier/types/ErrorString.h>
#include <quentier/local_storage/ILocalStoragePatch.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/logging/QuentierLogger.h>
#include <QDir>
#include <cmath>

namespace quentier {

LocalStorageUpgradeDialog::LocalStorageUpgradeDialog(const Account & currentAccount,
                                                     AccountModel & accountModel,
                                                     const QVector<ILocalStoragePatch *> & patches,
                                                     QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::LocalStorageUpgradeDialog),
    m_patches(patches),
    m_pAccountFilterModel(new AccountFilterModel(this)),
    m_currentPatchIndex(0)
{
    m_pUi->setupUi(this);
    m_pUi->statusBar->hide();
    m_pUi->accountsTableView->verticalHeader()->hide();

    m_pUi->upgradeProgressBar->setMinimum(0);
    m_pUi->upgradeProgressBar->setMaximum(100);

    m_pAccountFilterModel->setSourceModel(&accountModel);
    m_pAccountFilterModel->addFilteredAccount(currentAccount);
    m_pUi->accountsTableView->setModel(m_pAccountFilterModel);

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

    createConnections();
    setPatchInfoLabel();
}

LocalStorageUpgradeDialog::~LocalStorageUpgradeDialog()
{
    delete m_pUi;
}

void LocalStorageUpgradeDialog::onSwitchToAccountPushButtonPressed()
{
    QNDEBUG(QStringLiteral("LocalStorageUpgradeDialog::onSwitchToAccountPushButtonPressed"));

    QModelIndex currentAccountIndex = m_pUi->accountsTableView->currentIndex();
    if (!currentAccountIndex.isValid()) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("No account is selected")));
        return;
    }

    QModelIndex sourceAccountModelIndex = m_pAccountFilterModel->mapToSource(currentAccountIndex);
    if (Q_UNLIKELY(!sourceAccountModelIndex.isValid())) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("Internal error: could not figure out the selected account")));
        return;
    }

    const AccountModel * pAccountModel = qobject_cast<const AccountModel*>(m_pAccountFilterModel->sourceModel());
    if (Q_UNLIKELY(!pAccountModel)) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("Internal error: account model is not set")));
        return;
    }

    const QVector<Account> & accounts = pAccountModel->accounts();
    int row = sourceAccountModelIndex.row();
    if (Q_UNLIKELY((row < 0) || (row >= accounts.size()))) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("Internal error: wrong row is selected in the accounts table")));
        return;
    }

    const Account & newAccount = accounts.at(row);
    Q_EMIT shouldSwitchToAccount(newAccount);
    QDialog::accept();
}

void LocalStorageUpgradeDialog::onCreateNewAccountButtonPressed()
{
    QNDEBUG(QStringLiteral("LocalStorageUpgradeDialog::onCreateNewAccountButtonPressed"));

    Q_EMIT shouldCreateNewAccount();
    QDialog::accept();
}

void LocalStorageUpgradeDialog::onQuitAppButtonPressed()
{
    QNDEBUG(QStringLiteral("LocalStorageUpgradeDialog::onQuitAppButtonPressed"));

    Q_EMIT shouldQuitApp();
    QDialog::accept();
}

void LocalStorageUpgradeDialog::onApplyPatchButtonPressed()
{
    QNDEBUG(QStringLiteral("LocalStorageUpgradeDialog::onApplyPatchButtonPressed"));

    if (m_currentPatchIndex >= m_patches.size()) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("No patch to apply")));
        return;
    }

    ILocalStoragePatch * pPatch = m_patches[m_currentPatchIndex];
    QObject::connect(pPatch, QNSIGNAL(ILocalStoragePatch,progress,double),
                     this, QNSLOT(LocalStorageUpgradeDialog,onApplyPatchProgressUpdate,double),
                     Qt::ConnectionType(Qt::DirectConnection | Qt::UniqueConnection));

    ErrorString errorDescription;
    bool res = pPatch->apply(errorDescription);

    QObject::disconnect(pPatch, QNSIGNAL(ILocalStoragePatch,progress,double),
                        this, QNSLOT(LocalStorageUpgradeDialog,onApplyPatchProgressUpdate,double));

    if (Q_UNLIKELY(!res)) {
        ErrorString error(QT_TR_NOOP("Failed to upgrade local storage, please report issue to "
                                     "<a href=\"https://github.com/d1vanov/quentier/issues/new\">Quentier issue tracker</a>"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING(error);
        setErrorToStatusBar(error);
        return;
    }

    QNINFO(QStringLiteral("Successfully applied local storage patch from version ")
           << pPatch->fromVersion() << QStringLiteral(" to version ") << pPatch->toVersion());

    ++m_currentPatchIndex;
    if (m_currentPatchIndex == m_patches.size()) {
        QNINFO(QStringLiteral("No more local storage patches are required"));
        QDialog::accept();
    }
}

void LocalStorageUpgradeDialog::onApplyPatchProgressUpdate(double progress)
{
    QNDEBUG(QStringLiteral("LocalStorageUpgradeDialog::onApplyPatchProgressUpdate: progress = ") << progress);

    int value = static_cast<int>(std::floor(progress * 100.0 + 0.5));
    m_pUi->upgradeProgressBar->setValue(std::min(value, 100));
}

void LocalStorageUpgradeDialog::createConnections()
{
    QNDEBUG(QStringLiteral("LocalStorageUpgradeDialog::createConnections"));

    QObject::connect(m_pUi->switchToAnotherAccountPushButton, QNSIGNAL(QPushButton,pressed),
                     this, QNSLOT(LocalStorageUpgradeDialog,onSwitchToAccountPushButtonPressed));
    QObject::connect(m_pUi->createNewAccountPushButton, QNSIGNAL(QPushButton,pressed),
                     this, QNSLOT(LocalStorageUpgradeDialog,onCreateNewAccountButtonPressed));
    QObject::connect(m_pUi->quitAppPushButton, QNSIGNAL(QPushButton,pressed),
                     this, QNSLOT(LocalStorageUpgradeDialog,onQuitAppButtonPressed));
    QObject::connect(m_pUi->applyPatchPushButton, QNSIGNAL(QPushButton,pressed),
                     this, QNSLOT(LocalStorageUpgradeDialog,onApplyPatchButtonPressed));
}

void LocalStorageUpgradeDialog::setPatchInfoLabel()
{
    QNDEBUG(QStringLiteral("LocalStorageUpgradeDialog::setPatchInfoLabel: index = ") << m_currentPatchIndex);

    if (Q_UNLIKELY(m_currentPatchIndex >= m_patches.size())) {
        m_pUi->introInfoLabel->setText(QString());
        QNDEBUG(QStringLiteral("Index out of patches range"));
        return;
    }

    QString introInfo = tr("Please backup your account's local storage at");
    introInfo += QStringLiteral(" ");
    introInfo += QDir::toNativeSeparators(accountPersistentStoragePath(m_pAccountFilterModel->filteredAccounts()[0]));
    introInfo += QStringLiteral("\n\n");

    introInfo += tr("Local storage upgrade info: patch");
    introInfo += QStringLiteral(" ");
    introInfo += QString::number(m_currentPatchIndex + 1);
    introInfo += QStringLiteral(" ");

    // TRANSLATOR: Part of a larger string: "Local storage upgrade: patch N of M"
    introInfo += tr("of");
    introInfo += QStringLiteral(" ");
    introInfo += QString::number(m_patches.size());

    introInfo += QStringLiteral(", ");
    introInfo += tr("from version");
    introInfo += QStringLiteral(" ");
    introInfo += QString::number(m_patches[m_currentPatchIndex]->fromVersion());
    introInfo += QStringLiteral(" ");
    introInfo += tr("to version");
    introInfo += QStringLiteral(" ");
    introInfo += QString::number(m_patches[m_currentPatchIndex]->toVersion());

    m_pUi->introInfoLabel->setText(introInfo);
}

void LocalStorageUpgradeDialog::setErrorToStatusBar(const ErrorString & error)
{
    QNDEBUG(QStringLiteral("LocalStorageUpgradeDialog::setErrorToStatusBar: ") << error);
    m_pUi->statusBar->setText(error.localizedString());
    m_pUi->statusBar->show();
}

void LocalStorageUpgradeDialog::reject()
{
    // NOTE: doing nothing intentionally: LocalStorageUpgradeDialog cannot be rejected
}

} // namespace quentier
