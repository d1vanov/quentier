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

#ifndef QUENTIER_MODELS_AVAILABLE_ACCOUNTS_MODEL_H
#define QUENTIER_MODELS_AVAILABLE_ACCOUNTS_MODEL_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/StringUtils.h>
#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <QAbstractTableModel>
#include <QPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AccountManager)

class AvailableAccountsModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AvailableAccountsModel(AccountManager & accountManager,
                                    QObject * parent = Q_NULLPTR);
    ~AvailableAccountsModel();

    struct Columns
    {
        enum type
        {
            Type = 0,
            Username,
            DisplayName
        };
    };

Q_SIGNALS:
    void badAccountDisplayName(ErrorString errorDescription, int row);
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
    Q_DISABLE_COPY(AvailableAccountsModel)

private:
    QPointer<AccountManager>   m_pAccountManager;
    StringUtils                m_stringUtils;
};

} // namespace quentier

#endif // QUENTIER_MODELS_AVAILABLE_ACCOUNTS_MODEL_H
