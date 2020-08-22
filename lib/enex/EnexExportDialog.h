/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_DIALOG_ENEX_EXPORT_DIALOG_H
#define QUENTIER_LIB_DIALOG_ENEX_EXPORT_DIALOG_H

#include <quentier/types/Account.h>
#include <quentier/utility/Macros.h>

#include <QDialog>

namespace Ui {
class EnexExportDialog;
}

namespace quentier {

class EnexExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EnexExportDialog(
        const Account & account, QWidget * parent = nullptr,
        const QString & suggestedFileName = {});

    virtual ~EnexExportDialog() override;

    bool exportTags() const;
    QString exportEnexFilePath() const;

Q_SIGNALS:
    void exportTagsOptionChanged(bool checked);

private Q_SLOTS:
    void onExportTagsOptionChanged(int state);
    void onBrowseFolderButtonPressed();
    void onFileNameEdited(const QString & name);
    void onFolderEdited(const QString & name);

private:
    void createConnections();
    void persistExportFolderSetting();

    void checkConditionsAndEnableDisableOkButton();

    void setStatusText(const QString & text);
    void clearAndHideStatus();

private:
    Ui::EnexExportDialog * m_pUi;
    Account m_currentAccount;
};

} // namespace quentier

#endif // QUENTIER_LIB_DIALOG_ENEX_EXPORT_DIALOG_H
