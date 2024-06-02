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

#include "LocalStorageUpgradeDialog.h"
#include "ui_LocalStorageUpgradeDialog.h"

#include <lib/account/AccountFilterModel.h>
#include <lib/account/AccountModel.h>
#include <lib/exception/Utils.h>

#include <quentier/local_storage/IPatch.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/threading/Future.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/StandardPaths.h>

#include <QDir>
#include <QFutureWatcher>
#include <QItemSelection>
#include <QTextStream>

#include <cmath>

namespace quentier {

LocalStorageUpgradeDialog::LocalStorageUpgradeDialog(
    const Account & currentAccount, AccountModel & accountModel,
    QList<local_storage::IPatchPtr> patches, const Options options,
    QWidget * parent) :
    QDialog{parent},
    m_ui{new Ui::LocalStorageUpgradeDialog}, m_patches{std::move(patches)},
    m_accountFilterModel{new AccountFilterModel(this)}, m_options{options}
{
    m_ui->setupUi(this);
    m_ui->statusBar->hide();
    m_ui->accountsTableView->verticalHeader()->hide();
    m_ui->switchToAnotherAccountPushButton->setEnabled(false);

    m_ui->backupLocalStorageLabel->hide();
    m_ui->backupLocalStorageProgressBar->hide();
    m_ui->backupLocalStorageProgressBar->setMinimum(0);
    m_ui->backupLocalStorageProgressBar->setMaximum(100);

    m_ui->restoreLocalStorageFromBackupLabel->hide();
    m_ui->restoreLocalStorageFromBackupProgressBar->hide();
    m_ui->restoreLocalStorageFromBackupProgressBar->setMinimum(0);
    m_ui->restoreLocalStorageFromBackupProgressBar->setMaximum(100);

    showAccountInfo(currentAccount);
    showHideDialogPartsAccordingToOptions();

    m_ui->upgradeProgressBar->setMinimum(0);
    m_ui->upgradeProgressBar->setMaximum(100);

    m_accountFilterModel->setSourceModel(&accountModel);
    m_accountFilterModel->addFilteredAccount(currentAccount);
    m_ui->accountsTableView->setModel(m_accountFilterModel);

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

    createConnections();
    setPatchInfoLabel();

    if (!m_patches.isEmpty()) {
        auto * patch = m_patches[m_currentPatchIndex].get();
        setPatchDescriptions(*patch);
    }

    adjustSize();
}

LocalStorageUpgradeDialog::~LocalStorageUpgradeDialog()
{
    delete m_ui;
}

void LocalStorageUpgradeDialog::onSwitchToAccountPushButtonPressed()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::onSwitchToAccountPushButtonPressed");

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
        setErrorToStatusBar(
            ErrorString{QT_TR_NOOP("Internal error: wrong row is selected in "
                                   "the accounts table")});
        return;
    }

    const auto & newAccount = accounts.at(row);
    Q_EMIT shouldSwitchToAccount(newAccount);
    QDialog::accept();
}

void LocalStorageUpgradeDialog::onCreateNewAccountButtonPressed()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::onCreateNewAccountButtonPressed");

    Q_EMIT shouldCreateNewAccount();
    QDialog::accept();
}

void LocalStorageUpgradeDialog::onQuitAppButtonPressed()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::onQuitAppButtonPressed");

    Q_EMIT shouldQuitApp();
    QDialog::accept();
}

void LocalStorageUpgradeDialog::onApplyPatchButtonPressed()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::onApplyPatchButtonPressed");

    m_ui->statusBar->setText(QString{});
    m_ui->statusBar->hide();
    m_ui->restoreLocalStorageFromBackupLabel->hide();

    if (m_currentPatchIndex >= m_patches.size()) {
        setErrorToStatusBar(ErrorString{QT_TR_NOOP("No patch to apply")});
        return;
    }

    lockControls();

    if (m_ui->backupLocalStorageCheckBox->isChecked()) {
        m_ui->backupLocalStorageLabel->show();
        m_ui->backupLocalStorageProgressBar->show();
        startBackup();
        return;
    }

    applyPatch();
}

void LocalStorageUpgradeDialog::onApplyPatchProgressUpdate(double progress)
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::onApplyPatchProgressUpdate: progress = "
            << progress);

    const int value = static_cast<int>(std::floor(progress * 100.0 + 0.5));
    m_ui->upgradeProgressBar->setValue(std::min(value, 100));
}

void LocalStorageUpgradeDialog::onAccountViewSelectionChanged(
    const QItemSelection & selected, const QItemSelection & deselected)
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::onAccountViewSelectionChanged");

    Q_UNUSED(deselected)

    if (!(m_options & Option::SwitchToAnotherAccount)) {
        QNDEBUG(
            "dialog::LocalStorageUpgradeDialog",
            "The option of switching to another account is not available");
        return;
    }

    if (selected.isEmpty()) {
        QNDEBUG(
            "dialog::LocalStorageUpgradeDialog",
            "No selection, disabling switch to selected account button");
        m_ui->switchToAnotherAccountPushButton->setEnabled(false);
        return;
    }

    m_ui->switchToAnotherAccountPushButton->setEnabled(true);
}

void LocalStorageUpgradeDialog::onBackupLocalStorageProgressUpdate(
    double progress)
{
    QNTRACE(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::onBackupLocalStorageProgressUpdate: "
            << progress);

    const int scaledProgress =
        std::min(100, static_cast<int>(std::floor(progress * 100.0 + 0.5)));

    Q_EMIT backupLocalStorageProgress(scaledProgress);

    if (scaledProgress == 100) {
        m_ui->backupLocalStorageLabel->hide();
        m_ui->backupLocalStorageProgressBar->setValue(0);
        m_ui->backupLocalStorageProgressBar->hide();
    }
}

void LocalStorageUpgradeDialog::onRestoreLocalStorageFromBackupProgressUpdate(
    double progress)
{
    QNTRACE(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::"
        "onRestoreLocalStorageFromBackupProgressUpdate: "
            << progress);

    const int scaledProgress =
        std::min(100, static_cast<int>(std::floor(progress * 100.0 + 0.5)));

    Q_EMIT restoreLocalStorageFromBackupProgress(scaledProgress);

    if (scaledProgress == 100) {
        m_ui->restoreLocalStorageFromBackupLabel->hide();
        m_ui->restoreLocalStorageFromBackupProgressBar->setValue(0);
        m_ui->restoreLocalStorageFromBackupProgressBar->hide();
    }
}

void LocalStorageUpgradeDialog::createConnections()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::createConnections");

    if (m_options & Option::SwitchToAnotherAccount) {
        QObject::connect(
            m_ui->switchToAnotherAccountPushButton, &QPushButton::pressed,
            this,
            &LocalStorageUpgradeDialog::onSwitchToAccountPushButtonPressed);

        QObject::connect(
            m_ui->accountsTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &LocalStorageUpgradeDialog::onAccountViewSelectionChanged);
    }

    if (m_options & Option::AddAccount) {
        QObject::connect(
            m_ui->createNewAccountPushButton, &QPushButton::pressed, this,
            &LocalStorageUpgradeDialog::onCreateNewAccountButtonPressed);
    }

    QObject::connect(
        m_ui->quitAppPushButton, &QPushButton::pressed, this,
        &LocalStorageUpgradeDialog::onQuitAppButtonPressed);

    QObject::connect(
        m_ui->applyPatchPushButton, &QPushButton::pressed, this,
        &LocalStorageUpgradeDialog::onApplyPatchButtonPressed);
}

void LocalStorageUpgradeDialog::setPatchInfoLabel()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::setPatchInfoLabel: index = "
            << m_currentPatchIndex);

    if (Q_UNLIKELY(m_currentPatchIndex >= m_patches.size())) {
        m_ui->introInfoLabel->setText(QString());
        QNDEBUG("dialog::LocalStorageUpgradeDialog", "Index out of patches range");
        return;
    }

    QString introInfo;
    QTextStream strm{&introInfo};

    strm << tr(
        "The layout of data kept within the local storage "
        "requires to be upgraded.");

    strm << "\n";

    strm << tr(
        "It means that older versions of Quentier (and/or "
        "libquentier) will no longer be able to work with changed "
        "local storage data layout. But these changes are required "
        "in order to continue using the current versions of "
        "Quentier and libquentier.");

    strm << "\n";

    strm << tr(
        "It is recommended to keep the \"Backup local storage "
        "before the upgrade\" option enabled in order to backup "
        "the local storage at");

    strm << " ";

    strm << QDir::toNativeSeparators(accountPersistentStoragePath(
        m_accountFilterModel->filteredAccounts()[0]));

    strm << " ";
    strm << tr("before performing the upgrade");
    strm << ".\n\n";

    strm << tr("Local storage upgrade info: patch");
    strm << " ";
    strm << m_currentPatchIndex + 1;
    strm << " ";

    // TRANSLATOR: Part of a larger string: "Local storage upgrade: patch N of
    // M"
    strm << tr("of");
    strm << " ";
    strm << m_patches.size();

    strm << ", ";
    strm << tr("upgrade from version");
    strm << " ";
    strm << m_patches[m_currentPatchIndex]->fromVersion();
    strm << " ";
    strm << tr("to version");
    strm << " ";
    strm << m_patches[m_currentPatchIndex]->toVersion();

    strm.flush();

    m_ui->introInfoLabel->setText(introInfo);
}

void LocalStorageUpgradeDialog::setErrorToStatusBar(const ErrorString & error)
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::setErrorToStatusBar: " << error);

    QString message;
    QTextStream strm{&message};

    strm << "<html><head/><body><p><span style=\" font-size:12pt; "
            "color:#ff0000;\">";

    strm << error.localizedString();
    strm << "</span></p><p>";
    strm << tr("Please report issue to");
    strm << " <a href=\"https://github.com/d1vanov/quentier/issues\">";
    strm << tr("Quentier issue tracker");
    strm << "</a></p></body></html>";

    strm.flush();

    m_ui->statusBar->setText(message);
    m_ui->statusBar->show();
}

void LocalStorageUpgradeDialog::setPatchDescriptions(
    const local_storage::IPatch & patch)
{
    m_ui->shortDescriptionLabel->setText(patch.patchShortDescription());
    m_ui->longDescriptionPlainTextEdit->setPlainText(patch.patchLongDescription());
}

void LocalStorageUpgradeDialog::lockControls()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::lockControls");

    m_ui->switchToAnotherAccountPushButton->setEnabled(false);
    m_ui->createNewAccountPushButton->setEnabled(false);
    m_ui->applyPatchPushButton->setEnabled(false);
    m_ui->quitAppPushButton->setEnabled(false);
    m_ui->accountsTableView->setEnabled(false);
}

void LocalStorageUpgradeDialog::unlockControls()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::unlockControls");

    m_ui->switchToAnotherAccountPushButton->setEnabled(true);
    m_ui->createNewAccountPushButton->setEnabled(true);
    m_ui->applyPatchPushButton->setEnabled(true);
    m_ui->quitAppPushButton->setEnabled(true);
    m_ui->accountsTableView->setEnabled(true);
}

void LocalStorageUpgradeDialog::startBackup()
{
    QNINFO(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::startBackup");

    Q_ASSERT(m_currentPatchIndex < m_patches.size());
    auto * patch = m_patches[m_currentPatchIndex].get();

    auto backupLocalStorageFuture = patch->backupLocalStorage();

    QFutureWatcher<void> backupLocalStorageFutureWatcher;
    const int min = backupLocalStorageFuture.progressMinimum();
    const int max = backupLocalStorageFuture.progressMaximum();

    if (min != max) {
        QObject::connect(
            &backupLocalStorageFutureWatcher,
            &QFutureWatcher<void>::progressValueChanged,
            this,
            [this, min, max](const int progressValue) {
                const double progress =
                    static_cast<double>(progressValue) /
                    static_cast<double>(max - min);
                onBackupLocalStorageProgressUpdate(progress);
            });
    }

    QObject::connect(
        this, &LocalStorageUpgradeDialog::backupLocalStorageProgress,
        m_ui->backupLocalStorageProgressBar, &QProgressBar::setValue);

    backupLocalStorageFutureWatcher.setFuture(backupLocalStorageFuture);

    auto backupLocalStorageThenFuture =
        threading::then(std::move(backupLocalStorageFuture), this, [this] {
            QNINFO(
                "dialog::LocalStorageUpgradeDialog",
                "Backup completed successfully");

            finishBackup();
            applyPatch();
        });

    threading::onFailed(
        std::move(backupLocalStorageThenFuture), this,
        [this](const QException & e) {
            finishBackup();

            const auto error = exceptionMessage(e);
            setErrorToStatusBar(error);
            unlockControls();
        });
}

void LocalStorageUpgradeDialog::finishBackup()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::finishBackup");

    m_ui->backupLocalStorageLabel->hide();
    m_ui->backupLocalStorageProgressBar->setValue(0);
    m_ui->backupLocalStorageProgressBar->hide();

    QObject::disconnect(
        this, &LocalStorageUpgradeDialog::backupLocalStorageProgress,
        m_ui->backupLocalStorageProgressBar, &QProgressBar::setValue);
}

void LocalStorageUpgradeDialog::applyPatch()
{
    QNINFO(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::applyPatch");

    Q_ASSERT(m_currentPatchIndex < m_patches.size());
    auto * patch = m_patches[m_currentPatchIndex].get();

    auto applyPatchFuture = patch->apply();

    QFutureWatcher<void> applyPatchFutureWatcher;
    const int min = applyPatchFuture.progressMinimum();
    const int max = applyPatchFuture.progressMaximum();

    if (min != max) {
        QObject::connect(
            &applyPatchFutureWatcher,
            &QFutureWatcher<void>::progressValueChanged,
            this,
            [this, min, max](const int progressValue) {
                const double progress =
                    static_cast<double>(progressValue) /
                    static_cast<double>(max - min);
                onApplyPatchProgressUpdate(progress);
            });
    }

    applyPatchFutureWatcher.setFuture(applyPatchFuture);

    auto applyPatchThenFuture = threading::then(
        std::move(applyPatchFuture), this,
        [this, fromVersion = patch->fromVersion(),
         toVersion = patch->toVersion()] {
            QNINFO(
                "dialog::LocalStorageUpgradeDialog",
                "Successfully applied local storage patch from version "
                    << fromVersion << " to version " << toVersion);

            if (m_ui->removeLocalStorageBackupAfterUpgradeCheckBox->isChecked())
            {
                removeBackup(true);
                return;
            }

            finishPatch();
        });

    threading::onFailed(
        std::move(applyPatchThenFuture), this,
        [this](const QException & e) {
            auto errorDescription = exceptionMessage(e);

            ErrorString error{QT_TR_NOOP("Failed to upgrade local storage")};
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            QNWARNING("dialog::LocalStorageUpgradeDialog", error);
            setErrorToStatusBar(error);

            if (m_ui->backupLocalStorageCheckBox->isChecked()) {
                restoreFromBackup();
                return;
            }

            unlockControls();
        });
}

void LocalStorageUpgradeDialog::removeBackup(const bool shouldFinishPatch)
{
    QNINFO(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::removeBackup: finish patch = "
           << (shouldFinishPatch ? "true" : "false"));

    Q_ASSERT(m_currentPatchIndex < m_patches.size());
    auto * patch = m_patches[m_currentPatchIndex].get();

    auto removeBackupFuture = patch->removeLocalStorageBackup();

    auto removeBackupThenFuture = threading::then(
        std::move(removeBackupFuture), this, [this, shouldFinishPatch] {
            QNINFO(
                "dialog::LocalStorageUpgradeDialog",
                "Successfully removed local storage backup");

            if (shouldFinishPatch) {
                finishPatch();
            }
            else {
                unlockControls();
            }
        });

    threading::onFailed(
        std::move(removeBackupThenFuture), this,
        [this, shouldFinishPatch](const QException & e) {
            auto errorDescription = exceptionMessage(e);

            ErrorString error{
                QT_TR_NOOP("Failed to remove local storage backup")};
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            QNWARNING("dialog::LocalStorageUpgradeDialog", error);
            setErrorToStatusBar(error);

            if (shouldFinishPatch) {
                finishPatch();
            }
            else {
                unlockControls();
            }
        });
}

void LocalStorageUpgradeDialog::restoreFromBackup()
{
    QNINFO(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::restoreFromBackup");

    Q_ASSERT(m_currentPatchIndex < m_patches.size());
    auto * patch = m_patches[m_currentPatchIndex].get();

    auto restoreFromBackupFuture = patch->restoreLocalStorageFromBackup();

    QFutureWatcher<void> restoreFromBackupFutureWatcher;
    const int min = restoreFromBackupFuture.progressMinimum();
    const int max = restoreFromBackupFuture.progressMaximum();

    if (min != max) {
        QObject::connect(
            &restoreFromBackupFutureWatcher,
            &QFutureWatcher<void>::progressValueChanged,
            this,
            [this, min, max](const int progressValue) {
                const double progress =
                    static_cast<double>(progressValue) /
                    static_cast<double>(max - min);
                onRestoreLocalStorageFromBackupProgressUpdate(progress);
            });
    }

    QObject::connect(
        this, &LocalStorageUpgradeDialog::restoreLocalStorageFromBackupProgress,
        m_ui->restoreLocalStorageFromBackupProgressBar,
        &QProgressBar::setValue);

    restoreFromBackupFutureWatcher.setFuture(restoreFromBackupFuture);

    auto restoreFromBackupThenFuture =
        threading::then(std::move(restoreFromBackupFuture), this, [this] {
            QNINFO(
                "dialog::LocalStorageUpgradeDialog",
                "Successfully restored local storage from backup");
            finishRestoringFromBackup();

            if (m_ui->removeLocalStorageBackupAfterUpgradeCheckBox->isChecked())
            {
                removeBackup(false);
                return;
            }
        });

    threading::onFailed(
        std::move(restoreFromBackupThenFuture), this,
        [this](const QException & e) {
            finishRestoringFromBackup();

            const auto error = exceptionMessage(e);
            setErrorToStatusBar(error);
            unlockControls();
        });
}

void LocalStorageUpgradeDialog::finishRestoringFromBackup()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::finishRestoringFromBackup");

    QObject::disconnect(
        this,
        &LocalStorageUpgradeDialog::
        restoreLocalStorageFromBackupProgress,
        m_ui->restoreLocalStorageFromBackupProgressBar,
        &QProgressBar::setValue);

    m_ui->restoreLocalStorageFromBackupLabel->hide();
    m_ui->restoreLocalStorageFromBackupProgressBar->setValue(0);
    m_ui->restoreLocalStorageFromBackupProgressBar->hide();
}

void LocalStorageUpgradeDialog::finishPatch()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::finishPatch");

    ++m_currentPatchIndex;

    if (m_currentPatchIndex == m_patches.size()) {
        QNINFO(
            "dialog::LocalStorageUpgradeDialog",
            "No more local storage patches are required");
        m_upgradeDone = true;
        QDialog::accept();
        return;
    }

    // Otherwise set patch descriptions
    setPatchDescriptions(*m_patches[m_currentPatchIndex].get());
    unlockControls();
}

void LocalStorageUpgradeDialog::showAccountInfo(const Account & account)
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::showAccountInfo: " << account);

    QString message;
    QTextStream strm{&message};

    strm << tr("Account");
    strm << ": ";
    if (account.type() == Account::Type::Evernote) {
        strm << "Evernote";
    }
    else {
        strm << tr("local");
    }

    strm << ", ";
    strm << tr("name");
    strm << " = ";
    strm << account.name();

    if (account.type() == Account::Type::Evernote) {
        strm << ", ";
        strm << tr("server");
        strm << " = ";
        strm << account.evernoteHost();
    }

    m_ui->accountInfoLabel->setText(message);
}

void LocalStorageUpgradeDialog::showHideDialogPartsAccordingToOptions()
{
    QNDEBUG(
        "dialog::LocalStorageUpgradeDialog",
        "LocalStorageUpgradeDialog::showHideDialogPartsAccordingToOptions");

    if (!m_options.testFlag(Option::AddAccount)) {
        m_ui->createNewAccountPushButton->hide();
    }

    if (!m_options.testFlag(Option::SwitchToAnotherAccount)) {
        m_ui->switchToAnotherAccountPushButton->hide();
        m_ui->accountsTableView->hide();
        m_ui->pickAnotherAccountLabel->hide();
    }
}

void LocalStorageUpgradeDialog::reject()
{
    // NOTE: doing nothing intentionally: LocalStorageUpgradeDialog cannot be
    // rejected
}

} // namespace quentier
