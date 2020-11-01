/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#include "ParseStartupAccount.h"

#include <QDebug>

namespace quentier {

bool parseStartupAccount(
    const QString & accountStr, bool & isLocal, qevercloud::UserID & userId,
    QString & evernoteHost, QString & accountName,
    ErrorString & errorDescription)
{
    QString str = accountStr;

    isLocal = false;
    userId = -1;
    evernoteHost.resize(0);
    accountName.resize(0);

    if (str.startsWith(QStringLiteral("local_"))) {
        isLocal = true;
        str.remove(0, 6);
    }
    else if (str.startsWith(QStringLiteral("evernote_"))) {
        isLocal = false;
        evernoteHost = QStringLiteral("www.evernote.com");
        str.remove(0, 9);
    }
    else if (str.startsWith(QStringLiteral("evernotesandbox_"))) {
        isLocal = false;
        evernoteHost = QStringLiteral("sandbox.evernote.com");
        str.remove(0, 16);
    }
    else if (str.startsWith(QStringLiteral("yinxiangbiji_"))) {
        isLocal = false;
        evernoteHost = QStringLiteral("app.yinxiang.com");
        str.remove(0, 13);
    }
    else {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "ParseStartupAccount",
            "Wrong account specification on the command "
            "line: can't deduce the type of the account"));
        errorDescription.details() = accountStr;
        qWarning() << errorDescription.localizedString();
        return false;
    }

    if (!isLocal) {
        int nextUnderlineIndex = str.indexOf(QStringLiteral("_"));
        if (Q_UNLIKELY(nextUnderlineIndex < 0)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP(
                "ParseStartupAccount",
                "Wrong account specification on the command "
                "line: can't parse user id"));
            errorDescription.details() = accountStr;
            qWarning() << errorDescription.localizedString();
            return false;
        }

        QStringRef userIdStr = str.midRef(0, nextUnderlineIndex);
        bool conversionResult = false;
        userId =
            static_cast<qevercloud::UserID>(userIdStr.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP(
                "ParseStartupAccount",
                "Wrong account specification on the command "
                "line: can't parse user id, failed to "
                "convert from string to integer"));
            errorDescription.details() = accountStr;
            qWarning() << errorDescription.localizedString()
                       << ", user id str = " << userIdStr.toString();
            return false;
        }

        if (Q_UNLIKELY(userId < 0)) {
            errorDescription.setBase(QT_TRANSLATE_NOOP(
                "ParseStartupAccount",
                "Wrong account specification on the command "
                "line: parsed user id is negative"));
            errorDescription.details() = accountStr;
            qWarning() << errorDescription.localizedString()
                       << ", user id = " << userId;
            return false;
        }

        str.remove(0, nextUnderlineIndex + 1);
    }

    accountName = str;
    if (Q_UNLIKELY(accountName.isEmpty())) {
        errorDescription.setBase(QT_TRANSLATE_NOOP(
            "ParseStartupAccount",
            "Wrong account specification on the command "
            "line: account name is empty"));
        errorDescription.details() = accountStr;
        qWarning() << errorDescription.localizedString();
        return false;
    }

    qDebug() << "Parsed startup account: is local = "
             << (isLocal ? "true" : "false") << ", user id = " << userId
             << ", evernote host = " << evernoteHost
             << ", account name = " << accountName;

    return true;
}

} // namespace quentier
