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

#ifndef QUENTIER_LIB_ACCOUNT_ACCOUNT_MODEL_H
#define QUENTIER_LIB_ACCOUNT_ACCOUNT_MODEL_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/StringUtils.h>

#include <QAbstractTableModel>
#include <QPointer>

namespace quentier {

/**
 * @brief The AccountModel class wraps a vector of Account objects into a full
 * fledged table model inheriting from QAbstractTableModel.
 *
 * In a way AccountModel is a convenience model akin to QStringListModel -
 * if you have a vector of accounts, you can easily create a model around this
 * vector and use it along with any appropriate view.
 *
 * AccountModel is mostly read-only model: it only allows editing of display
 * names of accounts it works on. The signal is emitted for anyone interested
 * in the change of account display name.
 */
class AccountModel final: public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AccountModel(QObject * parent = nullptr);
    ~AccountModel();

    struct Columns
    {
        enum type
        {
            Type = 0,
            EvernoteHost,
            Username,
            DisplayName,
            Server
        };
    };

    const QVector<Account> & accounts() const
    {
        return m_accounts;
    }
    void setAccounts(const QVector<Account> & accounts);

    bool addAccount(const Account & account);
    bool removeAccount(const Account & account);

Q_SIGNALS:
    /**
     * badAccountDisplayName signal is emitted when the attempt is made to
     * change the account's display name in somewhat inappropriate way so that
     * the display name is not really changed
     */
    void badAccountDisplayName(ErrorString errorDescription, int row);

    /**
     * accountDisplayNameChanged signal is emitted when the display name is
     * changed for some account
     */
    void accountDisplayNameChanged(Account account);

    void accountAdded(const Account & account);
    void accountRemoved(const Account & account);

public:
    virtual Qt::ItemFlags flags(const QModelIndex & index) const override;
    virtual int rowCount(const QModelIndex & parent) const override;
    virtual int columnCount(const QModelIndex & parent) const override;

    virtual QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    virtual QVariant data(
        const QModelIndex & index, int role = Qt::DisplayRole) const override;

    virtual bool setData(
        const QModelIndex & index, const QVariant & value, int role) override;

private:
    Q_DISABLE_COPY(AccountModel)

private:
    QVector<Account> m_accounts;
    StringUtils m_stringUtils;
};

} // namespace quentier

#endif // QUENTIER_LIB_ACCOUNT_ACCOUNT_MODEL_H
