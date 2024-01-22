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

#pragma once

#include <quentier/types/Account.h>

#include <QDialog>

namespace Ui {
class EnexExportDialog;
} // namespace Ui

namespace quentier {

class EnexExportDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit EnexExportDialog(
        Account account, QWidget * parent = nullptr,
        const QString & suggestedFileName = {});

    ~EnexExportDialog() override;

    [[nodiscard]] bool exportTags() const noexcept;
    [[nodiscard]] QString exportEnexFilePath() const;

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
    const Account m_currentAccount;
    Ui::EnexExportDialog * m_ui;
};

} // namespace quentier
