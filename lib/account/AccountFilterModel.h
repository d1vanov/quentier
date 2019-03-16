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

#ifndef QUENTIER_LIB_ACCOUNT_ACCOUNT_FILTER_MODEL_H
#define QUENTIER_LIB_ACCOUNT_ACCOUNT_FILTER_MODEL_H

#include <quentier/utility/Macros.h>
#include <quentier/types/Account.h>

#include <QSortFilterProxyModel>
#include <QVector>

namespace quentier {

class AccountFilterModel: public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AccountFilterModel(QObject * parent = Q_NULLPTR);

    const QVector<Account> & filteredAccounts() const;
    bool setFilteredAccounts(const QVector<Account> & filteredAccounts);

    bool addFilteredAccount(const Account & account);
    bool removeFilteredAccount(const Account & account);

protected:
    virtual bool filterAcceptsRow(int sourceRow,
                                  const QModelIndex & sourceParent) const Q_DECL_OVERRIDE;

private:
    QVector<Account>    m_filteredAccounts;
};

} // namespace quentier

#endif // QUENTIER_LIB_ACCOUNT_ACCOUNT_FILTER_MODEL_H
