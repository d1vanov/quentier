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

#include "AccountFilterModel.h"
#include "AccountModel.h"

#include <quentier/logging/QuentierLogger.h>

#include <utility>

namespace quentier {

AccountFilterModel::AccountFilterModel(QObject * parent) :
    QSortFilterProxyModel{parent}
{}

const QList<Account> & AccountFilterModel::filteredAccounts() const
{
    return m_filteredAccounts;
}

bool AccountFilterModel::setFilteredAccounts(
    const QList<Account> & filteredAccounts)
{
    QNDEBUG("account", "AccountFilterModel::setFilteredAccounts");

    if (filteredAccounts.size() == m_filteredAccounts.size()) {
        bool changed = false;
        for (const auto & account: std::as_const(filteredAccounts)) {
            if (!m_filteredAccounts.contains(account)) {
                changed = true;
                break;
            }
        }

        if (!changed) {
            QNDEBUG(
                "account",
                "Filtered accounts haven't changed, nothing to do");
            return false;
        }
    }

    m_filteredAccounts = filteredAccounts;
    QSortFilterProxyModel::invalidateFilter();
    return true;
}

bool AccountFilterModel::addFilteredAccount(const Account & account)
{
    QNDEBUG("account", "AccountFilterModel::addFilteredAccount: " << account);

    if (m_filteredAccounts.contains(account)) {
        QNDEBUG(
            "account",
            "The account is already present within the list of filtered "
            "accounts");
        return false;
    }

    m_filteredAccounts << account;
    QSortFilterProxyModel::invalidateFilter();
    return true;
}

bool AccountFilterModel::removeFilteredAccount(const Account & account)
{
    QNDEBUG(
        "account", "AccountFilterModel::removeFilteredAccount: " << account);

    int index = m_filteredAccounts.indexOf(account);
    if (index < 0) {
        QNDEBUG(
            "account",
            "Coulnd't find the account to remove within the list of filtered "
            "accounts");
        return false;
    }

    m_filteredAccounts.removeAt(index);
    QSortFilterProxyModel::invalidateFilter();
    return true;
}

bool AccountFilterModel::filterAcceptsRow(
    int sourceRow, const QModelIndex & sourceParent) const
{
    Q_UNUSED(sourceParent)

    const auto * pAccountModel =
        qobject_cast<const AccountModel *>(sourceModel());

    if (!pAccountModel) {
        return false;
    }

    const auto & accounts = pAccountModel->accounts();
    if ((sourceRow < 0) || (sourceRow >= accounts.size())) {
        return false;
    }

    const auto & checkedAccount = accounts.at(sourceRow);
    if (m_filteredAccounts.contains(checkedAccount)) {
        return false;
    }

    return true;
}

} // namespace quentier
