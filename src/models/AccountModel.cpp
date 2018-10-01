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

#include "AccountModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QTextStream>
#include <iterator>

#define NUM_ACCOUNTS_MODEL_COLUMNS (4)

namespace quentier {

AccountModel::AccountModel(QObject * parent) :
    QAbstractTableModel(parent),
    m_accounts(),
    m_stringUtils()
{}

AccountModel::~AccountModel()
{}

void AccountModel::setAccounts(const QVector<Account> & accounts)
{
    QNDEBUG(QStringLiteral("AccountModel::setAccounts"));

    if (QuentierIsLogLevelActive(LogLevel::TraceLevel))
    {
        QString str;
        QTextStream strm(&str);

        strm << "\n";
        for(auto it = accounts.constBegin(), end = accounts.constEnd(); it != end; ++it) {
            strm << *it << "\n";
        }

        strm.flush();
        QNTRACE(str);
    }

    if (m_accounts == accounts) {
        QNDEBUG(QStringLiteral("Accounts haven't changed"));
        return;
    }

    Q_EMIT layoutAboutToBeChanged();
    m_accounts = accounts;
    Q_EMIT layoutChanged();
}

bool AccountModel::addAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("AccountModel::addAccount: ") << account);

    // Check whether this account is already within the list of accounts
    bool foundExistingAccount = false;
    Account::Type::type type = account.type();
    bool isLocal = (account.type() == Account::Type::Local);
    for(auto it = m_accounts.constBegin(), end = m_accounts.constEnd(); it != end; ++it)
    {
        const Account & availableAccount = *it;

        if (availableAccount.type() != type) {
            continue;
        }

        if (!isLocal && (availableAccount.id() != account.id())) {
            continue;
        }
        else if (isLocal && (availableAccount.name() != account.name())) {
            continue;
        }

        foundExistingAccount = true;
        break;
    }

    if (foundExistingAccount) {
        QNDEBUG(QStringLiteral("Account already exists"));
        return false;
    }

    int newRow = m_accounts.size();
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_accounts << account;
    endInsertRows();

    Q_EMIT accountAdded(account);
    return true;
}

bool AccountModel::removeAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("AccountModel::removeAccount: ") << account);

    Account::Type::type type = account.type();
    bool isLocal = (account.type() == Account::Type::Local);
    int index = 0;
    for(auto it = m_accounts.constBegin(), end = m_accounts.constEnd(); it != end; ++it, ++index)
    {
        const Account & availableAccount = *it;

        if (availableAccount.type() != type) {
            continue;
        }

        if (!isLocal && (availableAccount.id() != account.id())) {
            continue;
        }
        else if (isLocal && (availableAccount.name() != account.name())) {
            continue;
        }

        beginRemoveRows(QModelIndex(), index, index);
        m_accounts.remove(index);
        endRemoveRows();

        Q_EMIT accountRemoved(account);
        return true;
    }

    return false;
}

Qt::ItemFlags AccountModel::flags(const QModelIndex & index) const
{
    Qt::ItemFlags indexFlags = QAbstractTableModel::flags(index);
    if (!index.isValid()) {
        return indexFlags;
    }

    int row = index.row();
    int column = index.column();

    if ((row < 0) || (row >= m_accounts.size()) ||
        (column < 0) || (column >= NUM_ACCOUNTS_MODEL_COLUMNS))
    {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if (column == AccountModel::Columns::DisplayName) {
        indexFlags |= Qt::ItemIsEditable;
    }

    return indexFlags;
}

int AccountModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_accounts.size();
}

int AccountModel::columnCount(const QModelIndex & parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return NUM_ACCOUNTS_MODEL_COLUMNS;
}

QVariant AccountModel::headerData(int section, Qt::Orientation orientation, int role) const
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
        return tr("Type");
    case Columns::Username:
        return tr("Username");
    case Columns::DisplayName:
        return tr("Display name");
    case Columns::Server:
        return tr("Server");
    default:
        return QVariant();
    }
}

QVariant AccountModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    int row = index.row();
    int column = index.column();

    int numRows = m_accounts.size();
    if (Q_UNLIKELY((row < 0) || (row >= numRows))) {
        return QVariant();
    }

    const Account & account = m_accounts.at(row);

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
    case Columns::Server:
        {
            if (account.type() == Account::Type::Local) {
                return QString();
            }

            if (account.evernoteHost() == QStringLiteral("sandbox.evernote.com")) {
                return QStringLiteral("Evernote sandbox");
            }
            else if (account.evernoteHost() == QStringLiteral("app.yinxiang.com")) {
                return QStringLiteral("Yinxiang Biji");
            }
            else {
                return QStringLiteral("Evernote");
            }
        }
    default:
        return QVariant();
    }
}

bool AccountModel::setData(const QModelIndex & index,
                           const QVariant & value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    int row = index.row();
    int column = index.column();

    int numRows = m_accounts.size();
    if (Q_UNLIKELY((row < 0) || (row >= numRows))) {
        return false;
    }

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

            const Account & account = m_accounts.at(row);

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

            m_accounts[row].setDisplayName(displayName);

            Q_EMIT accountDisplayNameChanged(account);
            return true;
        }
    case Columns::Server:
        return false;
    default:
        return false;
    }
}

} // namespace quentier
