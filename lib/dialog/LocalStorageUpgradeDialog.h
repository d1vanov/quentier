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

#ifndef QUENTIER_LIB_DIALOG_LOCAL_STORAGE_UPGRADE_DIALOG_H
#define QUENTIER_LIB_DIALOG_LOCAL_STORAGE_UPGRADE_DIALOG_H

#include <quentier/utility/Macros.h>
#include <quentier/types/Account.h>

#include <QDialog>
#include <QVector>
#include <QSharedPointer>
#include <QFlags>

#include <memory>

namespace Ui {
class LocalStorageUpgradeDialog;
}

QT_FORWARD_DECLARE_CLASS(QItemSelection)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ErrorString)
QT_FORWARD_DECLARE_CLASS(AccountModel)
QT_FORWARD_DECLARE_CLASS(AccountFilterModel)
QT_FORWARD_DECLARE_CLASS(ILocalStoragePatch)

class LocalStorageUpgradeDialog : public QDialog
{
    Q_OBJECT
public:
    struct Option
    {
        enum type
        {
            AddAccount = 1 << 1,
            SwitchToAnotherAccount = 1 << 2
        };
    };
    Q_DECLARE_FLAGS(Options, Option::type)

    explicit LocalStorageUpgradeDialog(
        const Account & currentAccount,
        AccountModel & accountModel,
        const QVector<std::shared_ptr<ILocalStoragePatch> > & patches,
        const Options options, QWidget * parent = nullptr);

    virtual ~LocalStorageUpgradeDialog() override;

    bool isUpgradeDone() const { return m_upgradeDone; }

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
        const QItemSelection & selected,
        const QItemSelection & deselected);

    void onBackupLocalStorageProgressUpdate(double progress);
    void onRestoreLocalStorageFromBackupProgressUpdate(double progress);

private:
    void createConnections();
    void setPatchInfoLabel();
    void setErrorToStatusBar(const ErrorString & error);
    void setPatchDescriptions(const ILocalStoragePatch & patch);

    void lockControls();
    void unlockControls();

private:
    void showAccountInfo(const Account & account);
    void showHideDialogPartsAccordingToOptions();

private:
    virtual void reject() override;

private:
    Ui::LocalStorageUpgradeDialog * m_pUi;

    QVector<std::shared_ptr<ILocalStoragePatch>>    m_patches;

    AccountFilterModel *    m_pAccountFilterModel;

    Options     m_options;

    int         m_currentPatchIndex = 0;
    bool        m_upgradeDone = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(LocalStorageUpgradeDialog::Options)

} // namespace quentier

#endif // QUENTIER_LIB_DIALOG_LOCAL_STORAGE_UPGRADE_DIALOG_H
