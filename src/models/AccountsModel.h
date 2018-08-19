/*
 * Copyright 2018 Dmitry Ivanov
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

#ifndef QUENTIER_MODELS_ACCOUNTS_MODEL_H
#define QUENTIER_MODELS_ACCOUNTS_MODEL_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/StringUtils.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <QAbstractTableModel>
#include <QPointer>

namespace quentier {

/**
 * @brief The AccountsModel class wraps a vector of Account objects into a full fledged table model
 * inheriting from QAbstractTableModel.
 *
 * In a way AccountsModel is a convenience model akin to QStringListModel - if you have a vector of accounts,
 * you can easily create a model around this vector and use it along with any appropriate view.
 *
 * AccountsModel is mostly read-only model: it only allows editing of display names of accounts it works on.
 * The signal is emitted for anyone interested in the change of account display name.
 */
class AccountsModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AccountsModel(QObject * parent = Q_NULLPTR);
    ~AccountsModel();

    struct Columns
    {
        enum type
        {
            Type = 0,
            Username,
            DisplayName
        };
    };

    const QVector<Account> & accounts() const { return m_accounts; }
    void setAccounts(const QVector<Account> & accounts);

    bool addAccount(const Account & account);
    bool removeAccount(const Account & account);

Q_SIGNALS:
    /**
     * badAccountDisplayName signal is emitted when the attempt is made to change the account's display name
     * in somewhat inappropriate way so that the display name is not really changed
     */
    void badAccountDisplayName(ErrorString errorDescription, int row);

    /**
     * accountDisplayNameChanged signal is emitted when the display name is changed for some account
     */
    void accountDisplayNameChanged(Account account);

private:
    virtual Qt::ItemFlags flags(const QModelIndex & index) const Q_DECL_OVERRIDE;
    virtual int rowCount(const QModelIndex & parent) const Q_DECL_OVERRIDE;
    virtual int columnCount(const QModelIndex & parent) const Q_DECL_OVERRIDE;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex & index, const QVariant & value, int role) Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(AccountsModel)

private:
    QVector<Account>            m_accounts;
    StringUtils                 m_stringUtils;
};

} // namespace quentier

#endif // QUENTIER_MODELS_ACCOUNTS_MODEL_H
