/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "UserStore.h"
#include <quentier/types/UserWrapper.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/QuentierCheckPtr.h>

namespace quentier {

#define SET_EDAM_USER_EXCEPTION_ERROR(userException) \
    errorDescription = QT_TR_NOOP("caught EDAM user exception, error code"); \
    errorDescription += QStringLiteral(" "); \
    errorDescription += ToString(userException.errorCode); \
    if (!userException.exceptionData().isNull()) { \
        errorDescription += QStringLiteral(": "); \
        errorDescription += userException.exceptionData()->errorMessage; \
    }

UserStore::UserStore(QSharedPointer<qevercloud::UserStore> pQecUserStore) :
    m_pQecUserStore(pQecUserStore)
{
    QUENTIER_CHECK_PTR(m_pQecUserStore)
}

QSharedPointer<qevercloud::UserStore> UserStore::getQecUserStore()
{
    return m_pQecUserStore;
}

QString UserStore::authenticationToken() const
{
    return m_pQecUserStore->authenticationToken();
}

void UserStore::setAuthenticationToken(const QString & authToken)
{
    m_pQecUserStore->setAuthenticationToken(authToken);
}

qint32 UserStore::getUser(UserWrapper & user, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        user = m_pQecUserStore->getUser(m_pQecUserStore->authenticationToken());
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserException(userException, errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 UserStore::processEdamUserException(const qevercloud::EDAMUserException & userException, QNLocalizedString & errorDescription) const
{
    switch(userException.errorCode)
    {
    case qevercloud::EDAMErrorCode::BAD_DATA_FORMAT:
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT");
        break;
    case qevercloud::EDAMErrorCode::INTERNAL_ERROR:
        errorDescription = QT_TR_NOOP("INTERNAL_ERROR");
        break;
    case qevercloud::EDAMErrorCode::TAKEN_DOWN:
        errorDescription = QT_TR_NOOP("TAKEN_DOWN");
        break;
    case qevercloud::EDAMErrorCode::INVALID_AUTH:
        errorDescription = QT_TR_NOOP("INVALID_AUTH");
        break;
    case qevercloud::EDAMErrorCode::AUTH_EXPIRED:
        errorDescription = QT_TR_NOOP("AUTH_EXPIRED");
        break;
    case qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED:
        errorDescription = QT_TR_NOOP("RATE_LIMIT_REACHED");
        break;
    default:
        errorDescription = QT_TR_NOOP("Error code");
        errorDescription += QStringLiteral(": ");
        errorDescription += QString::number(userException.errorCode);
        break;
    }

    const auto exceptionData = userException.exceptionData();

    if (userException.parameter.isSet()) {
        errorDescription += QStringLiteral(", ");
        errorDescription += QT_TR_NOOP("parameter");
        errorDescription += QStringLiteral(": ");
        errorDescription += userException.parameter.ref();
    }

    if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
        errorDescription += QStringLiteral(", ");
        errorDescription += QT_TR_NOOP("error description");
        errorDescription += QStringLiteral(": ");
        errorDescription += exceptionData->errorMessage;
    }

    return userException.errorCode;
}

qint32 UserStore::processEdamSystemException(const qevercloud::EDAMSystemException & systemException,
                                             QNLocalizedString & errorDescription, qint32 & rateLimitSeconds) const
{
    rateLimitSeconds = -1;

    if (systemException.errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (!systemException.rateLimitDuration.isSet()) {
            errorDescription = QT_TR_NOOP("Evernote API rate limit exceeded but no rate limit duration is available");
        }
        else {
            errorDescription = QT_TR_NOOP("Evernote API rate limit exceeded, retry in");
            errorDescription += QStringLiteral(" ");
            errorDescription += QString::number(systemException.rateLimitDuration.ref());
            errorDescription += QStringLiteral(" ");
            errorDescription += QT_TR_NOOP("seconds");
            rateLimitSeconds = systemException.rateLimitDuration.ref();
        }
    }
    else
    {
        errorDescription = QT_TR_NOOP("Caught EDAM system exception, error code ");
        errorDescription += ToString(systemException.errorCode);
        if (systemException.message.isSet() && !systemException.message->isEmpty()) {
            errorDescription += QStringLiteral(", ");
            errorDescription += QT_TR_NOOP("message");
            errorDescription += QStringLiteral(": ");
            errorDescription += systemException.message.ref();
        }
    }

    return systemException.errorCode;
}

} // namespace quentier
