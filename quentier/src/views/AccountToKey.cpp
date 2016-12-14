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

#include "AccountToKey.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

QString accountToKey(const Account & account)
{
    QNDEBUG(QStringLiteral("accountToKey: ") << account);

    QString accountName = account.name();
    if (accountName.isEmpty()) {
        return QString();
    }

    if (account.type() == Account::Type::Local) {
        return QStringLiteral("local_") + accountName;
    }

    return QStringLiteral("evernote_") + accountName + QStringLiteral("_") + QString::number(account.id()) +
           QStringLiteral("_") + account.evernoteHost();
}

} // namespace quentier
