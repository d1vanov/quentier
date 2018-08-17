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

#include "AvailableAccountsModel.h"
#include "../AccountManager.h"
#include <quentier/logging/QuentierLogger.h>

#define NUM_ACCOUNTS_MODEL_COLUMNS (3)

namespace quentier {

AvailableAccountsModel::AvailableAccountsModel(AccountManager & accountManager, QObject * parent) :
    QAbstractTableModel(parent),
    m_pAccountManager(&accountManager),
    m_stringUtils()
{}

AvailableAccountsModel::~AvailableAccountsModel()
{}

Qt::ItemFlags AvailableAccountsModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractTableModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    if (m_pAccountManager.isNull()) {
        return indexFlags;
    }

    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();

    int row = index.row();
    int column = index.column();

    if ((row < 0) || (row >= availableAccounts.size()) ||
        (column < 0) || (column >= NUM_ACCOUNTS_MODEL_COLUMNS))
    {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if (column == AvailableAccountsModel::Columns::DisplayName) {
        indexFlags |= Qt::ItemIsEditable;
    }

    return indexFlags;
}

int AvailableAccountsModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    if (m_pAccountManager.isNull()) {
        return 0;
    }

    return m_pAccountManager->availableAccounts().size();
}

int AvailableAccountsModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return NUM_ACCOUNTS_MODEL_COLUMNS;
}

QVariant AvailableAccountsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch(section)
    {
    case Columns::Type:
        return QStringLiteral("Type");
    case Columns::Username:
        return QStringLiteral("Username");
    case Columns::DisplayName:
        return QStringLiteral("Display name");
    default:
        return QVariant();
    }
}

QVariant AvailableAccountsModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (m_pAccountManager.isNull()) {
        return QVariant();
    }

    const QVector<Account> & availableAccounts = m_pAccountManager->availableAccounts();

    int row = index.row();
    int column = index.column();

    int numRows = availableAccounts.size();
    if (Q_UNLIKELY((row < 0) || (row >= numRows))) {
        return QVariant();
    }

    const Account & account = availableAccounts[row];

    switch(column)
    {
    case Columns::Type:
        {
            if (account.type() == Account::Type::Local) {
                return QStringLiteral("Local");
            }
            else {
                return QStringLiteral("Evernote");
            }
        }
    case Columns::Username:
        return account.name();
    case Columns::DisplayName:
        return account.displayName();
    default:
        return QVariant();
    }
}

bool AvailableAccountsModel::setData(const QModelIndex & index,
                                     const QVariant & value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    if (m_pAccountManager.isNull()) {
        return false;
    }

    QVector<Account> availableAccounts = m_pAccountManager->availableAccounts();

    int row = index.row();
    int column = index.column();

    int numRows = availableAccounts.size();
    if (Q_UNLIKELY((row < 0) || (row >= numRows))) {
        return false;
    }

    Account & account = availableAccounts[row];

    switch(column)
    {
    case Columns::Type:
        return false;
    case Columns::Username:
        return false;
    case Columns::DisplayName:
        {
            QString displayName = value.toString().trimmed();
            m_stringUtils.removeNewlines(displayName);

            if (account.type() == Account::Type::Evernote)
            {
                int displayNameSize = displayName.size();

                if (displayNameSize < qevercloud::EDAM_USER_NAME_LEN_MIN) {
                    ErrorString error(QT_TR_NOOP("Account name length is below the acceptable level"));
                    error.details() = QString::number(qevercloud::EDAM_USER_NAME_LEN_MIN);
                    Q_EMIT badAccountDisplayName(error, row);
                    return false;
                }

                if (displayNameSize > qevercloud::EDAM_USER_NAME_LEN_MAX) {
                    ErrorString error(QT_TR_NOOP("Account name length is above the acceptable level"));
                    error.details() = QString::number(qevercloud::EDAM_USER_NAME_LEN_MAX);
                    Q_EMIT badAccountDisplayName(error, row);
                    return false;
                }

                QRegExp regex(qevercloud::EDAM_USER_NAME_REGEX);
                int matchIndex = regex.indexIn(displayName);
                if (matchIndex < 0) {
                    ErrorString error(QT_TR_NOOP("Account name doesn't match the Evernote's regular expression "
                                                 "for user names; consider simplifying the entered name"));
                    error.details() = QString::number(qevercloud::EDAM_USER_NAME_LEN_MAX);
                    Q_EMIT badAccountDisplayName(error, row);
                    return false;
                }
            }

            if (account.displayName() == displayName) {
                QNDEBUG(QStringLiteral("The account display name has not really changed"));
                return true;
            }

            account.setDisplayName(displayName);
            Q_EMIT accountDisplayNameChanged(account);
            return true;
        }
    default:
        return false;
    }
}


} // namespace quentier
