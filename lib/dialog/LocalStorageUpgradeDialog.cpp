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

#include "LocalStorageUpgradeDialog.h"
#include "ui_LocalStorageUpgradeDialog.h"

#include <lib/account/AccountFilterModel.h>
#include <lib/account/AccountModel.h>

#include <quentier/types/ErrorString.h>
#include <quentier/local_storage/ILocalStoragePatch.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/logging/QuentierLogger.h>

#include <QDir>
#include <QItemSelection>

#include <cmath>

namespace quentier {

LocalStorageUpgradeDialog::LocalStorageUpgradeDialog(
        const Account & currentAccount, AccountModel & accountModel,
        const QVector<QSharedPointer<ILocalStoragePatch> > & patches,
        const Options options, QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::LocalStorageUpgradeDialog),
    m_patches(patches),
    m_pAccountFilterModel(new AccountFilterModel(this)),
    m_options(options),
    m_currentPatchIndex(0),
    m_upgradeDone(false)
{
    m_pUi->setupUi(this);
    m_pUi->statusBar->hide();
    m_pUi->accountsTableView->verticalHeader()->hide();
    m_pUi->switchToAnotherAccountPushButton->setEnabled(false);

    m_pUi->backupLocalStorageLabel->hide();
    m_pUi->backupLocalStorageProgressBar->hide();
    m_pUi->backupLocalStorageProgressBar->setMinimum(0);
    m_pUi->backupLocalStorageProgressBar->setMaximum(100);

    m_pUi->restoreLocalStorageFromBackupLabel->hide();
    m_pUi->restoreLocalStorageFromBackupProgressBar->hide();
    m_pUi->restoreLocalStorageFromBackupProgressBar->setMinimum(0);
    m_pUi->restoreLocalStorageFromBackupProgressBar->setMaximum(100);

    showAccountInfo(currentAccount);
    showHideDialogPartsAccordingToOptions();

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

    QWidget::setWindowFlags(Qt::WindowFlags(Qt::Window |
                                            Qt::WindowTitleHint |
                                            Qt::CustomizeWindowHint));

    createConnections();
    setPatchInfoLabel();

    if (!m_patches.isEmpty()) {
        ILocalStoragePatch * pPatch = m_patches[m_currentPatchIndex].data();
        setPatchDescriptions(*pPatch);
    }

    adjustSize();
}

LocalStorageUpgradeDialog::~LocalStorageUpgradeDialog()
{
    delete m_pUi;
}

void LocalStorageUpgradeDialog::onSwitchToAccountPushButtonPressed()
{
    QNDEBUG("LocalStorageUpgradeDialog::onSwitchToAccountPushButtonPressed");

    QModelIndex currentAccountIndex = m_pUi->accountsTableView->currentIndex();
    if (!currentAccountIndex.isValid()) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("No account is selected")));
        return;
    }

    QModelIndex sourceAccountModelIndex =
        m_pAccountFilterModel->mapToSource(currentAccountIndex);
    if (Q_UNLIKELY(!sourceAccountModelIndex.isValid())) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("Internal error: could not "
                                                   "figure out the selected "
                                                   "account")));
        return;
    }

    const AccountModel * pAccountModel =
        qobject_cast<const AccountModel*>(m_pAccountFilterModel->sourceModel());
    if (Q_UNLIKELY(!pAccountModel)) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("Internal error: account "
                                                   "model is not set")));
        return;
    }

    const QVector<Account> & accounts = pAccountModel->accounts();
    int row = sourceAccountModelIndex.row();
    if (Q_UNLIKELY((row < 0) || (row >= accounts.size()))) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("Internal error: wrong row is "
                                                   "selected in the accounts table")));
        return;
    }

    const Account & newAccount = accounts.at(row);
    Q_EMIT shouldSwitchToAccount(newAccount);
    QDialog::accept();
}

void LocalStorageUpgradeDialog::onCreateNewAccountButtonPressed()
{
    QNDEBUG("LocalStorageUpgradeDialog::onCreateNewAccountButtonPressed");

    Q_EMIT shouldCreateNewAccount();
    QDialog::accept();
}

void LocalStorageUpgradeDialog::onQuitAppButtonPressed()
{
    QNDEBUG("LocalStorageUpgradeDialog::onQuitAppButtonPressed");

    Q_EMIT shouldQuitApp();
    QDialog::accept();
}

void LocalStorageUpgradeDialog::onApplyPatchButtonPressed()
{
    QNDEBUG("LocalStorageUpgradeDialog::onApplyPatchButtonPressed");

    m_pUi->statusBar->setText(QString());
    m_pUi->statusBar->hide();
    m_pUi->restoreLocalStorageFromBackupLabel->hide();

    if (m_currentPatchIndex >= m_patches.size()) {
        setErrorToStatusBar(ErrorString(QT_TR_NOOP("No patch to apply")));
        return;
    }

    lockControls();

    ILocalStoragePatch * pPatch = m_patches[m_currentPatchIndex].data();

    if (m_pUi->backupLocalStorageCheckBox->isChecked())
    {
        QObject::connect(pPatch,
                         QNSIGNAL(ILocalStoragePatch,backupProgress,double),
                         this,
                         QNSLOT(LocalStorageUpgradeDialog,
                                onBackupLocalStorageProgressUpdate,double));
        QObject::connect(this,
                         QNSIGNAL(LocalStorageUpgradeDialog,
                                  backupLocalStorageProgress,int),
                         m_pUi->backupLocalStorageProgressBar,
                         QNSLOT(QProgressBar,setValue,int));
        m_pUi->backupLocalStorageLabel->show();
        m_pUi->backupLocalStorageProgressBar->show();

        ErrorString errorDescription;
        bool res = pPatch->backupLocalStorage(errorDescription);

        QObject::disconnect(pPatch,
                            QNSIGNAL(ILocalStoragePatch,backupProgress,double),
                            this,
                            QNSLOT(LocalStorageUpgradeDialog,
                                   onBackupLocalStorageProgressUpdate,double));
        QObject::disconnect(this,
                            QNSIGNAL(LocalStorageUpgradeDialog,
                                     backupLocalStorageProgress,int),
                            m_pUi->backupLocalStorageProgressBar,
                            QNSLOT(QProgressBar,setValue,int));
        m_pUi->backupLocalStorageLabel->hide();
        m_pUi->backupLocalStorageProgressBar->setValue(0);
        m_pUi->backupLocalStorageProgressBar->hide();

        if (Q_UNLIKELY(!res)) {
            setErrorToStatusBar(errorDescription);
            unlockControls();
            return;
        }
    }

    QObject::connect(pPatch,
                     QNSIGNAL(ILocalStoragePatch,progress,double),
                     this,
                     QNSLOT(LocalStorageUpgradeDialog,
                            onApplyPatchProgressUpdate,double),
                     Qt::ConnectionType(Qt::DirectConnection | Qt::UniqueConnection));

    ErrorString errorDescription;
    bool res = pPatch->apply(errorDescription);

    QObject::disconnect(pPatch,
                        QNSIGNAL(ILocalStoragePatch,progress,double),
                        this,
                        QNSLOT(LocalStorageUpgradeDialog,
                               onApplyPatchProgressUpdate,double));

    if (Q_UNLIKELY(!res))
    {
        ErrorString error(QT_TR_NOOP("Failed to upgrade local storage"));
        error.appendBase(errorDescription.base());
        error.appendBase(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING(error);
        setErrorToStatusBar(error);

        if (m_pUi->backupLocalStorageCheckBox->isChecked())
        {
            QObject::connect(pPatch,
                             QNSIGNAL(ILocalStoragePatch,
                                      restoreBackupProgress,double),
                             this,
                             QNSLOT(LocalStorageUpgradeDialog,
                                    onRestoreLocalStorageFromBackupProgressUpdate,
                                    double));
            QObject::connect(this,
                             QNSIGNAL(LocalStorageUpgradeDialog,
                                      restoreLocalStorageFromBackupProgress,int),
                             m_pUi->restoreLocalStorageFromBackupProgressBar,
                             QNSLOT(QProgressBar,setValue,int));
            m_pUi->restoreLocalStorageFromBackupLabel->show();
            m_pUi->restoreLocalStorageFromBackupProgressBar->show();

            errorDescription.clear();
            res = pPatch->restoreLocalStorageFromBackup(errorDescription);

            QObject::disconnect(pPatch,
                                QNSIGNAL(ILocalStoragePatch,
                                         restoreBackupProgress,double),
                                this,
                                QNSLOT(LocalStorageUpgradeDialog,
                                       onRestoreLocalStorageFromBackupProgressUpdate,
                                       double));
            QObject::disconnect(this,
                                QNSIGNAL(LocalStorageUpgradeDialog,
                                         restoreLocalStorageFromBackupProgress,
                                         int),
                                m_pUi->restoreLocalStorageFromBackupProgressBar,
                                QNSLOT(QProgressBar,setValue,int));

            m_pUi->restoreLocalStorageFromBackupLabel->hide();
            m_pUi->restoreLocalStorageFromBackupProgressBar->setValue(0);
            m_pUi->restoreLocalStorageFromBackupProgressBar->hide();

            if (Q_UNLIKELY(!res))
            {
                QString message = QStringLiteral("<html><head/><body><p>"
                                                 "<span style=\" "
                                                 "font-size:12pt; "
                                                 "color:#ff0000;\">");
                message += errorDescription.localizedString();
                message += QStringLiteral("</span></p></body></html>");
                m_pUi->restoreLocalStorageFromBackupLabel->setText(message);
                m_pUi->restoreLocalStorageFromBackupLabel->show();
            }

            if (m_pUi->removeLocalStorageBackupAfterUpgradeCheckBox->isChecked()) {
                errorDescription.clear();
                Q_UNUSED(!pPatch->removeLocalStorageBackup(errorDescription))
            }
        }

        unlockControls();
        return;
    }

    if (m_pUi->removeLocalStorageBackupAfterUpgradeCheckBox->isChecked()) {
        errorDescription.clear();
        Q_UNUSED(pPatch->removeLocalStorageBackup(errorDescription))
    }

    QNINFO("Successfully applied local storage patch from version "
           << pPatch->fromVersion() << " to version " << pPatch->toVersion());

    ++m_currentPatchIndex;

    if (m_currentPatchIndex == m_patches.size()) {
        QNINFO("No more local storage patches are required");
        m_upgradeDone = true;
        QDialog::accept();
        return;
    }

    // Otherwise set patch descriptions
    setPatchDescriptions(*m_patches[m_currentPatchIndex].data());
    unlockControls();
}

void LocalStorageUpgradeDialog::onApplyPatchProgressUpdate(double progress)
{
    QNDEBUG("LocalStorageUpgradeDialog::onApplyPatchProgressUpdate: progress = "
            << progress);

    int value = static_cast<int>(std::floor(progress * 100.0 + 0.5));
    m_pUi->upgradeProgressBar->setValue(std::min(value, 100));
}

void LocalStorageUpgradeDialog::onAccountViewSelectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNDEBUG("LocalStorageUpgradeDialog::onAccountViewSelectionChanged");

    Q_UNUSED(deselected)

    if (!(m_options & Option::SwitchToAnotherAccount)) {
        QNDEBUG("The option of switching to another account is not available");
        return;
    }

    if (selected.isEmpty()) {
        QNDEBUG("No selection, disabling switch to selected account button");
        m_pUi->switchToAnotherAccountPushButton->setEnabled(false);
        return;
    }

    m_pUi->switchToAnotherAccountPushButton->setEnabled(true);
}

void LocalStorageUpgradeDialog::onBackupLocalStorageProgressUpdate(
    double progress)
{
    QNTRACE("LocalStorageUpgradeDialog::onBackupLocalStorageProgressUpdate: "
            << progress);

    int scaledProgress =
        std::min(100, static_cast<int>(std::floor(progress * 100.0 + 0.5)));
    Q_EMIT backupLocalStorageProgress(scaledProgress);

    if (scaledProgress == 100) {
        m_pUi->backupLocalStorageLabel->hide();
        m_pUi->backupLocalStorageProgressBar->setValue(0);
        m_pUi->backupLocalStorageProgressBar->hide();
    }
}

void LocalStorageUpgradeDialog::onRestoreLocalStorageFromBackupProgressUpdate(
    double progress)
{
    QNTRACE("LocalStorageUpgradeDialog::"
            << "onRestoreLocalStorageFromBackupProgressUpdate: "
            << progress);

    int scaledProgress =
        std::min(100, static_cast<int>(std::floor(progress * 100.0 + 0.5)));
    Q_EMIT restoreLocalStorageFromBackupProgress(scaledProgress);

    if (scaledProgress == 100) {
        m_pUi->restoreLocalStorageFromBackupLabel->hide();
        m_pUi->restoreLocalStorageFromBackupProgressBar->setValue(0);
        m_pUi->restoreLocalStorageFromBackupProgressBar->hide();
    }
}

void LocalStorageUpgradeDialog::createConnections()
{
    QNDEBUG("LocalStorageUpgradeDialog::createConnections");

    if (m_options & Option::SwitchToAnotherAccount)
    {
        QObject::connect(m_pUi->switchToAnotherAccountPushButton,
                         QNSIGNAL(QPushButton,pressed),
                         this,
                         QNSLOT(LocalStorageUpgradeDialog,
                                onSwitchToAccountPushButtonPressed));
        QObject::connect(m_pUi->accountsTableView->selectionModel(),
                         QNSIGNAL(QItemSelectionModel,selectionChanged,
                                  QItemSelection,QItemSelection),
                         this,
                         QNSLOT(LocalStorageUpgradeDialog,
                                onAccountViewSelectionChanged,
                                QItemSelection,QItemSelection));
    }

    if (m_options & Option::AddAccount)
    {
        QObject::connect(m_pUi->createNewAccountPushButton,
                         QNSIGNAL(QPushButton,pressed),
                         this,
                         QNSLOT(LocalStorageUpgradeDialog,
                                onCreateNewAccountButtonPressed));
    }

    QObject::connect(m_pUi->quitAppPushButton,
                     QNSIGNAL(QPushButton,pressed),
                     this,
                     QNSLOT(LocalStorageUpgradeDialog,onQuitAppButtonPressed));
    QObject::connect(m_pUi->applyPatchPushButton,
                     QNSIGNAL(QPushButton,pressed),
                     this,
                     QNSLOT(LocalStorageUpgradeDialog,onApplyPatchButtonPressed));
}

void LocalStorageUpgradeDialog::setPatchInfoLabel()
{
    QNDEBUG("LocalStorageUpgradeDialog::setPatchInfoLabel: index = "
            << m_currentPatchIndex);

    if (Q_UNLIKELY(m_currentPatchIndex >= m_patches.size())) {
        m_pUi->introInfoLabel->setText(QString());
        QNDEBUG("Index out of patches range");
        return;
    }

    QString introInfo = tr("The layout of data kept within the local storage "
                           "requires to be upgraded.");
    introInfo += QStringLiteral("\n");
    introInfo += tr("It means that older versions of Quentier (and/or libquentier) "
                    "will no longer be able to work with changed local storage "
                    "data layout. But these changes are required in order to "
                    "continue using the current versions of Quentier and "
                    "libquentier.");
    introInfo += QStringLiteral("\n");
    introInfo += tr("It is recommended to keep the \"Backup local storage before "
                    "the upgrade\" option enabled in order to backup the local "
                    "storage at");
    introInfo += QStringLiteral(" ");
    introInfo += QDir::toNativeSeparators(
        accountPersistentStoragePath(m_pAccountFilterModel->filteredAccounts()[0]));
    introInfo += QStringLiteral(" ");
    introInfo += tr("before performing the upgrade");
    introInfo += QStringLiteral(".\n\n");

    introInfo += tr("Local storage upgrade info: patch");
    introInfo += QStringLiteral(" ");
    introInfo += QString::number(m_currentPatchIndex + 1);
    introInfo += QStringLiteral(" ");

    // TRANSLATOR: Part of a larger string: "Local storage upgrade: patch N of M"
    introInfo += tr("of");
    introInfo += QStringLiteral(" ");
    introInfo += QString::number(m_patches.size());

    introInfo += QStringLiteral(", ");
    introInfo += tr("upgrade from version");
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
    QNDEBUG("LocalStorageUpgradeDialog::setErrorToStatusBar: " << error);
    QString message = QStringLiteral("<html><head/><body><p>"
                                     "<span style=\" font-size:12pt; "
                                     "color:#ff0000;\">");
    message += error.localizedString();
    message += QStringLiteral("</span></p><p>");
    message += tr("Please report issue to");
    message +=
        QStringLiteral(" <a href=\"https://github.com/d1vanov/quentier/issues\">");
    message += tr("Quentier issue tracker");
    message += QStringLiteral("</a></p></body></html>");

    m_pUi->statusBar->setText(message);
    m_pUi->statusBar->show();
}

void LocalStorageUpgradeDialog::setPatchDescriptions(
    const ILocalStoragePatch & patch)
{
    m_pUi->shortDescriptionLabel->setText(patch.patchShortDescription());
    m_pUi->longDescriptionLabel->setText(patch.patchLongDescription());
}

void LocalStorageUpgradeDialog::lockControls()
{
    QNDEBUG("LocalStorageUpgradeDialog::lockControls");

    m_pUi->switchToAnotherAccountPushButton->setEnabled(false);
    m_pUi->createNewAccountPushButton->setEnabled(false);
    m_pUi->applyPatchPushButton->setEnabled(false);
    m_pUi->quitAppPushButton->setEnabled(false);
    m_pUi->accountsTableView->setEnabled(false);
}

void LocalStorageUpgradeDialog::unlockControls()
{
    QNDEBUG("LocalStorageUpgradeDialog::unlockControls");

    m_pUi->switchToAnotherAccountPushButton->setEnabled(true);
    m_pUi->createNewAccountPushButton->setEnabled(true);
    m_pUi->applyPatchPushButton->setEnabled(true);
    m_pUi->quitAppPushButton->setEnabled(true);
    m_pUi->accountsTableView->setEnabled(true);
}

void LocalStorageUpgradeDialog::showAccountInfo(const Account & account)
{
    QNDEBUG("LocalStorageUpgradeDialog::showAccountInfo: " << account);

    QString message = tr("Account");
    message += QStringLiteral(": ");
    if (account.type() == Account::Type::Evernote) {
        message += QStringLiteral("Evernote");
    }
    else {
        message += tr("local");
    }

    message += QStringLiteral(", ");
    message += tr("name");
    message += QStringLiteral(" = ");
    message += account.name();

    if (account.type() == Account::Type::Evernote) {
        message += QStringLiteral(", ");
        message += tr("server");
        message += QStringLiteral(" = ");
        message += account.evernoteHost();
    }

    m_pUi->accountInfoLabel->setText(message);
}

void LocalStorageUpgradeDialog::showHideDialogPartsAccordingToOptions()
{
    QNDEBUG("LocalStorageUpgradeDialog::showHideDialogPartsAccordingToOptions");

    if (!(m_options & Option::AddAccount)) {
        m_pUi->createNewAccountPushButton->hide();
    }

    if (!(m_options & Option::SwitchToAnotherAccount)) {
        m_pUi->switchToAnotherAccountPushButton->hide();
        m_pUi->accountsTableView->hide();
        m_pUi->pickAnotherAccountLabel->hide();
    }
}

void LocalStorageUpgradeDialog::reject()
{
    // NOTE: doing nothing intentionally: LocalStorageUpgradeDialog cannot be rejected
}

} // namespace quentier
