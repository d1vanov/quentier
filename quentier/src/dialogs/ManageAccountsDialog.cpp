/*
 * Copyright 2016 Dmitry Ivanov
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

#include "ManageAccountsDialog.h"
#include "ui_ManageAccountsDialog.h"

ManageAccountsDialog::ManageAccountsDialog(QVector<AvailableAccount> availableAccounts,
                                           QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::ManageAccountsDialog),
    m_availableAccounts(availableAccounts),
    m_accountListModel()
{
    m_pUi->setupUi(this);

    int numAvailableAccounts = m_availableAccounts.size();
    QStringList accountsList;
    accountsList.reserve(numAvailableAccounts);

    for(int i = 0; i < numAvailableAccounts; ++i)
    {
        const AvailableAccount & availableAccount = m_availableAccounts[i];
        QString displayedAccountName = availableAccount.username();
        if (availableAccount.isLocal()) {
            displayedAccountName += QStringLiteral(" (");
            // TRANSLATOR: the "local" word here means the local account i.e. the one not synchronized with Evernote
            displayedAccountName += tr("local");
            displayedAccountName += QStringLiteral(")");
        }

        accountsList << displayedAccountName;
    }

    m_accountListModel.setStringList(accountsList);
    m_pUi->listView->setModel(&m_accountListModel);
}

ManageAccountsDialog::~ManageAccountsDialog()
{
    delete m_pUi;
}
