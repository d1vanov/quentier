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

#pragma once

#include <quentier/local_storage/Fwd.h>
#include <quentier/types/Account.h>

#include <QDialog>
#include <QFlags>
#include <QList>
#include <QSharedPointer>

#include <memory>

namespace Ui {

class LocalStorageUpgradeDialog;

} // namespace Ui

class QItemSelection;

namespace quentier {

class AccountModel;
class AccountFilterModel;
class ErrorString;

class LocalStorageUpgradeDialog final : public QDialog
{
    Q_OBJECT
public:
    enum class Option
    {
        AddAccount = 1 << 1,
        SwitchToAnotherAccount = 1 << 2
    };
    Q_DECLARE_FLAGS(Options, Option)

    explicit LocalStorageUpgradeDialog(
        const Account & currentAccount, AccountModel & accountModel,
        QList<local_storage::IPatchPtr> patches,
        Options options, QWidget * parent = nullptr);

    ~LocalStorageUpgradeDialog() override;

    [[nodiscard]] bool isUpgradeDone() const noexcept
    {
        return m_upgradeDone;
    }

Q_SIGNALS:
    void shouldSwitchToAccount(Account account);
    void shouldCreateNewAccount();
    void shouldQuitApp();

    // private signals:
    void backupLocalStorageProgress(int progress);
    void restoreLocalStorageFromBackupProgress(int progress);

private Q_SLOTS:
    void onSwitchToAccountPushButtonPressed();
    void onCreateNewAccountButtonPressed();
    void onQuitAppButtonPressed();
    void onApplyPatchButtonPressed();

    void onApplyPatchProgressUpdate(double progress);

    void onAccountViewSelectionChanged(
        const QItemSelection & selected, const QItemSelection & deselected);

    void onBackupLocalStorageProgressUpdate(double progress);
    void onRestoreLocalStorageFromBackupProgressUpdate(double progress);

private:
    void createConnections();
    void setPatchInfoLabel();
    void setErrorToStatusBar(const ErrorString & error);
    void setPatchDescriptions(const local_storage::IPatch & patch);

    void lockControls();
    void unlockControls();

    void startBackup();
    void finishBackup();

    void applyPatch();
    void removeBackup(bool shouldFinishPatch);

    void restoreFromBackup();
    void finishRestoringFromBackup();

    void finishPatch();

private:
    void showAccountInfo(const Account & account);
    void showHideDialogPartsAccordingToOptions();

private:
    void reject() override;

private:
    Ui::LocalStorageUpgradeDialog * m_ui;

    QList<local_storage::IPatchPtr> m_patches;

    AccountFilterModel * m_accountFilterModel;

    Options m_options;

    int m_currentPatchIndex = 0;
    bool m_upgradeDone = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(LocalStorageUpgradeDialog::Options)

} // namespace quentier
