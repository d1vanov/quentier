/*
 * Copyright 2018-2021 Dmitry Ivanov
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

AccountModel::AccountModel(QObject * parent) : QAbstractTableModel(parent) {}

AccountModel::~AccountModel() = default;

const QVector<Account> & AccountModel::accounts() const noexcept
{
    return m_accounts;
}

void AccountModel::setAccounts(const QVector<Account> & accounts)
{
    QNDEBUG("account", "AccountModel::setAccounts");

    if (QuentierIsLogLevelActive(LogLevel::Trace)) {
        QString str;
        QTextStream strm(&str);

        strm << "\n";
        for (auto it = accounts.constBegin(), end = accounts.constEnd();
             it != end; ++it)
        {
            strm << *it << "\n";
        }

        strm.flush();
        if (!str.isEmpty()) {
            QNTRACE("account", str);
        }
    }

    if (m_accounts == accounts) {
        QNDEBUG("account", "Accounts haven't changed");
        return;
    }

    Q_EMIT layoutAboutToBeChanged();
    m_accounts = accounts;
    Q_EMIT layoutChanged();
}

bool AccountModel::addAccount(const Account & account)
{
    QNDEBUG("account", "AccountModel::addAccount: " << account);

    // Check whether this account is already within the list of accounts
    bool foundExistingAccount = false;
    Account::Type type = account.type();
    const bool isLocal = (account.type() == Account::Type::Local);
    for (auto it = m_accounts.constBegin(), end = m_accounts.constEnd();
         it != end; ++it)
    {
        const Account & availableAccount = *it;

        if (availableAccount.type() != type) {
            continue;
        }

        if (!isLocal && (availableAccount.id() != account.id())) {
            continue;
        }

        if (isLocal && (availableAccount.name() != account.name())) {
            continue;
        }

        foundExistingAccount = true;
        break;
    }

    if (foundExistingAccount) {
        QNDEBUG("account", "Account already exists");
        return false;
    }

    const int newRow = m_accounts.size();
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_accounts << account;
    endInsertRows();

    Q_EMIT accountAdded(account);
    return true;
}

bool AccountModel::removeAccount(const Account & account)
{
    QNDEBUG("account", "AccountModel::removeAccount: " << account);

    const Account::Type type = account.type();
    const bool isLocal = (account.type() == Account::Type::Local);
    int index = 0;
    for (auto it = m_accounts.constBegin(), end = m_accounts.constEnd();
         it != end; ++it, ++index)
    {
        const Account & availableAccount = *it;

        if (availableAccount.type() != type) {
            continue;
        }

        if (!isLocal && (availableAccount.id() != account.id())) {
            continue;
        }

        if (isLocal && (availableAccount.name() != account.name())) {
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

    const int row = index.row();
    const int column = index.column();

    if ((row < 0) || (row >= m_accounts.size()) || (column < 0) ||
        (column >= NUM_ACCOUNTS_MODEL_COLUMNS))
    {
        return indexFlags;
    }

    indexFlags |= Qt::ItemIsSelectable;
    indexFlags |= Qt::ItemIsEnabled;

    if (column == static_cast<int>(AccountModel::Column::DisplayName)) {
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

QVariant AccountModel::headerData(
    int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }

    switch (static_cast<Column>(section)) {
    case Column::Type:
        return tr("Type");
    case Column::EvernoteHost:
        return tr("Evernote host");
    case Column::Username:
        return tr("Username");
    case Column::DisplayName:
        return tr("Display name");
    case Column::Server:
        return tr("Server");
    default:
        return {};
    }
}

QVariant AccountModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (role != Qt::DisplayRole) {
        return {};
    }

    int row = index.row();
    int column = index.column();

    int numRows = m_accounts.size();
    if (Q_UNLIKELY((row < 0) || (row >= numRows))) {
        return {};
    }

    const Account & account = m_accounts.at(row);

    switch (static_cast<Column>(column)) {
    case Column::Type:
    {
        if (account.type() == Account::Type::Local) {
            return QStringLiteral("Local");
        }

        return QStringLiteral("Evernote");
    }
    case Column::EvernoteHost:
    {
        if (account.type() == Account::Type::Evernote) {
            return account.evernoteHost();
        }

        return {};
    }
    case Column::Username:
        return account.name();
    case Column::DisplayName:
        return account.displayName();
    case Column::Server:
    {
        if (account.type() == Account::Type::Local) {
            return {};
        }

        if (account.evernoteHost() == QStringLiteral("sandbox.evernote.com")) {
            return QStringLiteral("Evernote sandbox");
        }

        if (account.evernoteHost() == QStringLiteral("app.yinxiang.com")) {
            return QStringLiteral("Yinxiang Biji");
        }

        return QStringLiteral("Evernote");
    }
    default:
        return {};
    }
}

bool AccountModel::setData(
    const QModelIndex & index, const QVariant & value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role != Qt::EditRole) {
        return false;
    }

    const int row = index.row();
    const int column = index.column();

    const int numRows = m_accounts.size();
    if (Q_UNLIKELY((row < 0) || (row >= numRows))) {
        return false;
    }

    if (column != static_cast<int>(Column::DisplayName)) {
        return false;
    }

    QString displayName = value.toString().trimmed();
    m_stringUtils.removeNewlines(displayName);

    const auto & account = m_accounts.at(row);

    if (account.type() == Account::Type::Evernote) {
        const int displayNameSize = displayName.size();
        if (displayNameSize < qevercloud::EDAM_USER_NAME_LEN_MIN) {
            ErrorString error(
                QT_TR_NOOP("Account name length is below "
                            "the acceptable level"));

            error.details() =
                QString::number(qevercloud::EDAM_USER_NAME_LEN_MIN);

            Q_EMIT badAccountDisplayName(error, row);
            return false;
        }

        if (displayNameSize > qevercloud::EDAM_USER_NAME_LEN_MAX) {
            ErrorString error(
                QT_TR_NOOP("Account name length is above "
                            "the acceptable level"));

            error.details() =
                QString::number(qevercloud::EDAM_USER_NAME_LEN_MAX);

            Q_EMIT badAccountDisplayName(error, row);
            return false;
        }

        const QRegExp regex(qevercloud::EDAM_USER_NAME_REGEX);
        const int matchIndex = regex.indexIn(displayName);
        if (matchIndex < 0) {
            ErrorString error(
                QT_TR_NOOP("Account name doesn't match the Evernote's "
                            "regular expression for user names; consider "
                            "simplifying the entered name"));

            error.details() =
                QString::number(qevercloud::EDAM_USER_NAME_LEN_MAX);

            Q_EMIT badAccountDisplayName(error, row);
            return false;
        }
    }

    if (account.displayName() == displayName) {
        QNDEBUG(
            "account",
            "The account display name has not really changed");
        return true;
    }

    m_accounts[row].setDisplayName(displayName);

    Q_EMIT accountDisplayNameChanged(account);
    return true;
}

} // namespace quentier
