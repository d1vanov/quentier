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

#include <quentier/types/Account.h>

#include <QSortFilterProxyModel>
#include <QList>

namespace quentier {

class AccountFilterModel final : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AccountFilterModel(QObject * parent = nullptr);

    [[nodiscard]] const QList<Account> & filteredAccounts() const;
    bool setFilteredAccounts(const QList<Account> & filteredAccounts);

    bool addFilteredAccount(const Account & account);
    bool removeFilteredAccount(const Account & account);

protected: // QSortFilterProxyModel
    [[nodiscard]] bool filterAcceptsRow(
        int sourceRow, const QModelIndex & sourceParent) const override;

private:
    QList<Account> m_filteredAccounts;
};

} // namespace quentier
