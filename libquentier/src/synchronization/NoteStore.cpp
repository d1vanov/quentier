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

#include "NoteStore.h"
#include <quentier/types/Notebook.h>
#include <quentier/types/Note.h>
#include <quentier/types/Tag.h>
#include <quentier/types/SavedSearch.h>
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

NoteStore::NoteStore(QSharedPointer<qevercloud::NoteStore> pQecNoteStore) :
    m_pQecNoteStore(pQecNoteStore)
{
    QUENTIER_CHECK_PTR(m_pQecNoteStore)
}

QSharedPointer<qevercloud::NoteStore> NoteStore::getQecNoteStore()
{
    return m_pQecNoteStore;
}

QString NoteStore::noteStoreUrl() const
{
    return m_pQecNoteStore->noteStoreUrl();
}

void NoteStore::setNoteStoreUrl(const QString & noteStoreUrl)
{
    m_pQecNoteStore->setNoteStoreUrl(noteStoreUrl);
}

QString NoteStore::authenticationToken() const
{
    return m_pQecNoteStore->authenticationToken();
}

void NoteStore::setAuthenticationToken(const QString & authToken)
{
    m_pQecNoteStore->setAuthenticationToken(authToken);
}

qint32 NoteStore::createNotebook(Notebook & notebook, QNLocalizedString & errorDescription,
                                 qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        notebook = m_pQecNoteStore->createNotebook(notebook, linkedNotebookAuthToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForNotebook(notebook, userException,
                                                   UserExceptionSource::Creation,
                                                   errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateNotebook(Notebook & notebook, QNLocalizedString & errorDescription,
                                 qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        qint32 usn = m_pQecNoteStore->updateNotebook(notebook, linkedNotebookAuthToken);
        notebook.setUpdateSequenceNumber(usn);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForNotebook(notebook, userException,
                                                   UserExceptionSource::Update,
                                                   errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::createNote(Note & note, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        note =  m_pQecNoteStore->createNote(note, linkedNotebookAuthToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForNote(note, userException, UserExceptionSource::Creation,
                                               errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateNote(Note & note, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        note = m_pQecNoteStore->updateNote(note, linkedNotebookAuthToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForNote(note, userException, UserExceptionSource::Update,
                                               errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::createTag(Tag & tag, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        tag = m_pQecNoteStore->createTag(tag, linkedNotebookAuthToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForTag(tag, userException,
                                              UserExceptionSource::Creation,
                                              errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateTag(Tag & tag, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds, const QString & linkedNotebookAuthToken)
{
    try
    {
        qint32 usn = m_pQecNoteStore->updateTag(tag, linkedNotebookAuthToken);
        tag.setUpdateSequenceNumber(usn);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForTag(tag, userException,
                                              UserExceptionSource::Update,
                                              errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::createSavedSearch(SavedSearch & savedSearch, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        savedSearch = m_pQecNoteStore->createSearch(savedSearch);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        processEdamUserExceptionForSavedSearch(savedSearch, userException,
                                               UserExceptionSource::Creation,
                                               errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateSavedSearch(SavedSearch & savedSearch, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        qint32 usn = m_pQecNoteStore->updateSearch(savedSearch);
        savedSearch.setUpdateSequenceNumber(usn);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        processEdamUserExceptionForSavedSearch(savedSearch, userException,
                                               UserExceptionSource::Update,
                                               errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getSyncState(qevercloud::SyncState & syncState, QNLocalizedString & errorDescription,
                               qint32 & rateLimitSeconds)
{
    try
    {
        syncState = m_pQecNoteStore->getSyncState();
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        SET_EDAM_USER_EXCEPTION_ERROR(userException)
        return userException.errorCode;
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getSyncChunk(const qint32 afterUSN, const qint32 maxEntries,
                               const qevercloud::SyncChunkFilter & filter,
                               qevercloud::SyncChunk & syncChunk,
                               QNLocalizedString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        syncChunk = m_pQecNoteStore->getFilteredSyncChunk(afterUSN, maxEntries, filter);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForGetSyncChunk(userException, afterUSN,
                                                       maxEntries, errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getLinkedNotebookSyncState(const qevercloud::LinkedNotebook & linkedNotebook,
                                             const QString & authToken, qevercloud::SyncState & syncState,
                                             QNLocalizedString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        syncState = m_pQecNoteStore->getLinkedNotebookSyncState(linkedNotebook, authToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        SET_EDAM_USER_EXCEPTION_ERROR(userException)
        return userException.errorCode;
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        errorDescription = QT_TR_NOOP("caught EDAM not found exception, could not find "
                                      "linked notebook to get the sync state for");
        if (!notFoundException.exceptionData().isNull()) {
            errorDescription += QStringLiteral(": ");
            errorDescription += ToString(notFoundException.exceptionData()->errorMessage);
        }

        return qevercloud::EDAMErrorCode::UNKNOWN;
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getLinkedNotebookSyncChunk(const qevercloud::LinkedNotebook & linkedNotebook,
                                             const qint32 afterUSN, const qint32 maxEntries,
                                             const QString & linkedNotebookAuthToken,
                                             const bool fullSyncOnly, qevercloud::SyncChunk & syncChunk,
                                             QNLocalizedString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        syncChunk = m_pQecNoteStore->getLinkedNotebookSyncChunk(linkedNotebook, afterUSN,
                                                                maxEntries, fullSyncOnly,
                                                                linkedNotebookAuthToken);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForGetSyncChunk(userException, afterUSN,
                                                       maxEntries, errorDescription);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        errorDescription = QT_TR_NOOP("caught EDAM not found exception while attempting to "
                                      "download the sync chunk for linked notebook");
        if (!notFoundException.exceptionData().isNull())
        {
            errorDescription += QStringLiteral(": ");
            const QString & errorMessage = notFoundException.exceptionData()->errorMessage;
            if (errorMessage == QStringLiteral("LinkedNotebook")) {
                errorDescription += QT_TR_NOOP("the provided information doesn't match any valid notebook");
            }
            else if (errorMessage == QStringLiteral("LinkedNotebook.uri")) {
                errorDescription += QT_TR_NOOP("the provided public URI doesn't match any valid notebook");
            }
            else if (errorMessage == QStringLiteral("SharedNotebook.id")) {
                errorDescription += QT_TR_NOOP("the provided information indicates the shared notebook no longer exists");
            }
            else {
                errorDescription += QT_TR_NOOP("unknown error");
                errorDescription += QStringLiteral(": ");
                errorDescription += errorMessage;
            }
        }

        return qevercloud::EDAMErrorCode::UNKNOWN;
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getNote(const bool withContent, const bool withResourcesData,
                          const bool withResourcesRecognition, const bool withResourceAlternateData,
                          Note & note, QNLocalizedString & errorDescription, qint32 & rateLimitSeconds)
{
    if (!note.hasGuid()) {
        errorDescription = QT_TR_NOOP("can't get note: note's guid is empty");
        return qevercloud::EDAMErrorCode::UNKNOWN;
    }

    try
    {
        note = m_pQecNoteStore->getNote(note.guid(), withContent, withResourcesData,
                                        withResourcesRecognition, withResourceAlternateData);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForGetNote(note, userException, errorDescription);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        processEdamNotFoundException(notFoundException, errorDescription);
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription, rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::authenticateToSharedNotebook(const QString & shareKey, qevercloud::AuthenticationResult & authResult,
                                               QNLocalizedString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        authResult = m_pQecNoteStore->authenticateToSharedNotebook(shareKey);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_REQUIRED) {
            errorDescription = QT_TR_NOOP("no valid authentication token for current user");
        }
        else if (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED) {
            errorDescription = QT_TR_NOOP("share requires login, and another username has already been bound "
                                          "to this notebook");
        }
        else {
            errorDescription = QT_TR_NOOP("unexpected EDAM user exception, error code");
            errorDescription += QStringLiteral(": ");
            errorDescription += ToString(userException.errorCode);
        }

        return userException.errorCode;
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        // NOTE: this means that shared notebook no longer exists. It can happen with
        // shared/linked notebooks from time to time so it shouldn't be really considered
        // an error. Instead, the method would return empty auth result which should indicate
        // the fact of missing shared notebook to the caller
        Q_UNUSED(notFoundException)
        authResult = qevercloud::AuthenticationResult();
        return 0;
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        if (systemException.errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED) {
            if (!systemException.rateLimitDuration.isSet()) {
                errorDescription = QT_TR_NOOP("QEverCloud error: RATE_LIMIT_REACHED exception was caught but rateLimitDuration is not set");
                return qevercloud::EDAMErrorCode::UNKNOWN;
            }

            rateLimitSeconds = systemException.rateLimitDuration;
        }
        else if (systemException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT) {
            errorDescription = QT_TR_NOOP("invalid share key");
        }
        else if (systemException.errorCode == qevercloud::EDAMErrorCode::INVALID_AUTH) {
            errorDescription = QT_TR_NOOP("bad signature of share key");
        }
        else {
            errorDescription = QT_TR_NOOP("unexpected EDAM system exception, error code");
            errorDescription += QStringLiteral(": ");
            errorDescription += ToString(systemException.errorCode);
        }

        return systemException.errorCode;
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::processEdamUserExceptionForTag(const Tag & tag, const qevercloud::EDAMUserException & userException,
                                                 const NoteStore::UserExceptionSource::type & source,
                                                 QNLocalizedString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("tag");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Tag.name")) {
            if (tag.hasName()) {
                errorDescription += QT_TR_NOOP("invalid length or pattern of tag's name: ");
                errorDescription += tag.name();
            }
            else {
                errorDescription += QT_TR_NOOP("tag has no name");
            }
        }
        else if (userException.parameter.ref() == "Tag.parentGuid") {
            if (tag.hasParentGuid()) {
                errorDescription += QT_TR_NOOP("malformed parent guid of tag");
                errorDescription += QStringLiteral(": ");
                errorDescription += tag.parentGuid();
            }
            else {
                errorDescription += QT_TR_NOOP("error code indicates malformed parent guid but it is empty");
            }
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        errorDescription = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("tag");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Tag.name")) {
            if (tag.hasName()) {
                errorDescription += QT_TR_NOOP("invalid length or pattern of tag's name: ");
                errorDescription += tag.name();
            }
            else {
                errorDescription += QT_TR_NOOP("tag has no name");
            }
        }

        if (!thrownOnCreation && (userException.parameter.ref() == "Tag.parentGuid")) {
            if (tag.hasParentGuid()) {
                errorDescription += QT_TR_NOOP("can't set parent for tag: circular parent-child correlation detected");
                errorDescription += QStringLiteral(": ");
                errorDescription += tag.parentGuid();
            }
            else {
                errorDescription += QT_TR_NOOP("error code indicates the problem with circular parent-child correlation "
                                               "but tag's parent guid is empty");
            }
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED))
    {
        errorDescription = QT_TR_NOOP("LIMIT_REACHED exception during the attempt to create tag");

        if (userException.parameter.isSet() && (userException.parameter.ref() == "Tag")) {
            errorDescription += QStringLiteral(": ");
            errorDescription += QT_TR_NOOP("already at max number of tags, please remove some of them");
        }

        return userException.errorCode;
    }
    else if (!thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED))
    {
        errorDescription = QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to update tag");

        if (userException.parameter.isSet() && (userException.parameter.ref() == "Tag")) {
            errorDescription += QStringLiteral(": ");
            errorDescription += QT_TR_NOOP("user doesn't own the tag, it can't be updated");
        }

        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QT_TR_NOOP("tag"), userException, source, errorDescription);
}

qint32 NoteStore::processEdamUserExceptionForSavedSearch(const SavedSearch & search,
                                                         const qevercloud::EDAMUserException & userException,
                                                         const NoteStore::UserExceptionSource::type & source,
                                                         QNLocalizedString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("saved search");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("SavedSearch.name")) {
            if (search.hasName()) {
                errorDescription += QT_TR_NOOP("invalid length or pattern of saved search's name");
                errorDescription += QStringLiteral(": ");
                errorDescription += search.name();
            }
            else {
                errorDescription += QT_TR_NOOP("saved search has no name");
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("SavedSearch.query")) {
            if (search.hasQuery()) {
                errorDescription += QT_TR_NOOP("invalid length of saved search's query");
                errorDescription += QStringLiteral(": ");
                errorDescription += QString::number(search.query().length());
                QNWARNING(errorDescription << QStringLiteral(", query: ") << search.query());
            }
            else {
                errorDescription += QT_TR_NOOP("saved search has no query");
            }
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        errorDescription = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("saved search");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("SavedSearch.name")) {
            if (search.hasName()) {
                errorDescription += QT_TR_NOOP("saved search's name is already in use");
                errorDescription += QStringLiteral(": ");
                errorDescription += search.name();
            }
            else {
                errorDescription += QT_TR_NOOP("saved search has no name");
            }
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED))
    {
        errorDescription = QT_TR_NOOP("LIMIT_REACHED exception during the attempt to create saved search: "
                                      "already at max number of saved searches");
        return userException.errorCode;
    }
    else if (!thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED))
    {
        errorDescription = QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to update saved search: "
                                      "user doesn't own saved search");
        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QT_TR_NOOP("saved search"), userException,
                                              source, errorDescription);
}

qint32 NoteStore::processEdamUserExceptionForGetSyncChunk(const qevercloud::EDAMUserException & userException,
                                                          const qint32 afterUSN, const qint32 maxEntries,
                                                          QNLocalizedString & errorDescription) const
{
    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to get sync chunk");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("afterUSN")) {
            errorDescription += QT_TR_NOOP("afterUSN is negative");
            errorDescription += QStringLiteral(": ");
            errorDescription += QString::number(afterUSN);
        }
        else if (userException.parameter.ref() == QStringLiteral("maxEntries")) {
            errorDescription += QT_TR_NOOP("maxEntries is less than 1");
            errorDescription += QStringLiteral(": ");
            errorDescription += QString::number(maxEntries);
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }
    }
    else
    {
        errorDescription += QT_TR_NOOP("Unknown EDAM user exception on attempt to get filtered sync chunk");
    }

    return userException.errorCode;
}

qint32 NoteStore::processEdamUserExceptionForGetNote(const Note & note, const qevercloud::EDAMUserException & userException,
                                                     QNLocalizedString & errorDescription) const
{
    Q_UNUSED(note);     // Maybe it'd be actually used in future

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to get note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note.guid")) {
            errorDescription += QT_TR_NOOP("note's guid is missing");
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED)
    {
        errorDescription = QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to get note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note")) {
            errorDescription += QT_TR_NOOP("note is not owned by user");
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }

    errorDescription = QT_TR_NOOP("Unexpected EDAM user exception on attempt to get note: error code");
    errorDescription += QStringLiteral(": ");
    errorDescription += ToString(userException.errorCode);

    if (userException.parameter.isSet()) {
        errorDescription += QStringLiteral("; ");
        errorDescription += QT_TR_NOOP("parameter");
        errorDescription += QStringLiteral(": ");
        errorDescription += userException.parameter.ref();
    }

    return userException.errorCode;
}

qint32 NoteStore::processEdamUserExceptionForNotebook(const Notebook & notebook,
                                                      const qevercloud::EDAMUserException & userException,
                                                      const NoteStore::UserExceptionSource::type & source,
                                                      QNLocalizedString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("notebook");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Notebook.name")) {
            if (notebook.hasName()) {
                errorDescription += QT_TR_NOOP("invalid length or pattern of notebook's name");
                errorDescription += QStringLiteral(": ");
                errorDescription += notebook.name();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no name");
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Notebook.stack")) {
            if (notebook.hasStack()) {
                errorDescription += QT_TR_NOOP("invalid length or pattern of notebook's stack");
                errorDescription += QStringLiteral(": ");
                errorDescription += notebook.stack();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no stack");
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Publishing.uri")) {
            if (notebook.hasPublishingUri()) {
                errorDescription += QT_TR_NOOP("invalid publishing uri for notebook");
                errorDescription += QStringLiteral(": ");
                errorDescription += notebook.publishingUri();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no publishing uri");
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Publishing.publicDescription"))
        {
            if (notebook.hasPublishingPublicDescription()) {
                errorDescription += QT_TR_NOOP("public description for notebook is too long");
                errorDescription += QStringLiteral(": ");
                errorDescription += notebook.publishingPublicDescription();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no public description");
            }
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        errorDescription = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("notebook");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Notebook.name")) {
            if (notebook.hasName()) {
                errorDescription += QT_TR_NOOP("notebook's name is already in use");
                errorDescription += QStringLiteral(": ");
                errorDescription += notebook.name();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no name");
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Publishing.uri")) {
            if (notebook.hasPublishingUri()) {
                errorDescription += QT_TR_NOOP("notebook's publishing uri is already in use");
                errorDescription += QStringLiteral(": ");
                errorDescription += notebook.publishingUri();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no publishing uri");
            }
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED))
    {
        errorDescription = QT_TR_NOOP("LIMIT_REACHED exception durig the attempt to create notebook");

        if (userException.parameter.isSet() && (userException.parameter.ref() == "Notebook")) {
            errorDescription += QStringLiteral(": ");
            errorDescription += QT_TR_NOOP("already at max number of notebooks, please remove some of them");
        }

        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QT_TR_NOOP("notebook"), userException, source, errorDescription);
}

qint32 NoteStore::processEdamUserExceptionForNote(const Note & note, const qevercloud::EDAMUserException & userException,
                                                  const NoteStore::UserExceptionSource::type & source,
                                                  QNLocalizedString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note.title")) {
            if (note.hasTitle()) {
                errorDescription += QT_TR_NOOP("invalid length or pattetn of note's title");
                errorDescription += QStringLiteral(": ");
                errorDescription += note.title();
            }
            else {
                errorDescription += QT_TR_NOOP("note has no title");
            }
        }
        else if (userException.parameter.ref() == QStringLiteral("Note.content")) {
            if (note.hasContent()) {
                errorDescription += QT_TR_NOOP("invalid lenfth for note's ENML content");
                errorDescription += QStringLiteral(": ");
                errorDescription += QString::number(note.content().length());
                QNWARNING(errorDescription << QStringLiteral(", note's content: ") << note.content());
            }
            else {
                errorDescription += QT_TR_NOOP("note has no content");
            }
        }
        else if (userException.parameter.ref().startsWith(QStringLiteral("NoteAttributes."))) {
            if (note.hasNoteAttributes()) {
                errorDescription += QT_TR_NOOP("invalid note attributes");
                QNWARNING(errorDescription << QStringLiteral(": ") << note.noteAttributes());
            }
            else {
                errorDescription += QT_TR_NOOP("note has no attributes");
            }
        }
        else if (userException.parameter.ref().startsWith(QStringLiteral("ResourceAttributes."))) {
            errorDescription += QT_TR_NOOP("invalid resource attributes for some of note's resources");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else if (userException.parameter.ref() == QStringLiteral("Resource.mime")) {
            errorDescription += QT_TR_NOOP("invalid mime type for some of note's resources");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else if (userException.parameter.ref() == QStringLiteral("Tag.name")) {
            errorDescription += QT_TR_NOOP("Note.tagNames was provided and one "
                                           "of the specified tags had an invalid length or pattern");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        errorDescription = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note.deleted")) {
            errorDescription += QT_TR_NOOP("deletion timestamp is set on active note");
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_REQUIRED)
    {
        errorDescription = QT_TR_NOOP("DATA_REQUIRED exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Resource.data")) {
            errorDescription += QT_TR_NOOP("Data body for some of note's resources is missing");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::ENML_VALIDATION)
    {
        errorDescription = QT_TR_NOOP("ENML_VALIDATION exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("note: note's content doesn't validate against DTD");
        QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED)
    {
        errorDescription = QT_TR_NOOP("LIMIT_REACHED exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (thrownOnCreation && (userException.parameter.ref() == QStringLiteral("Note"))) {
            errorDescription += QT_TR_NOOP("already at maximum number of notes per account");
        }
        else if (userException.parameter.ref() == QStringLiteral("Note.size")) {
            errorDescription += QT_TR_NOOP("total note size is too large");
        }
        else if (userException.parameter.ref() == QStringLiteral("Note.resources")) {
            errorDescription += QT_TR_NOOP("too many resources on note");
        }
        else if (userException.parameter.ref() == QStringLiteral("Note.tagGuids")) {
            errorDescription += QT_TR_NOOP("too many tags on note");
        }
        else if (userException.parameter.ref() == QStringLiteral("Resource.data.size")) {
            errorDescription += QT_TR_NOOP("one of note's resource's data is too large");
        }
        else if (userException.parameter.ref().startsWith(QStringLiteral("NoteAttribute."))) {
            errorDescription += QT_TR_NOOP("note attributes string is too large");
            if (note.hasNoteAttributes()) {
                QNWARNING(errorDescription << QStringLiteral(", note attributes: ") << note.noteAttributes());
            }
        }
        else if (userException.parameter.ref().startsWith(QStringLiteral("ResourceAttribute."))) {
            errorDescription += QT_TR_NOOP("one of note's resources has too large resource attributes string");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else if (userException.parameter.ref() == QStringLiteral("Tag")) {
            errorDescription += QT_TR_NOOP("Note.tagNames was provided, and the required new tags "
                                           "would exceed the maximum number per account");
            QNWARNING(errorDescription << QStringLiteral(", note: ") << note);
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED)
    {
        errorDescription = QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += QStringLiteral(": ");
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == QStringLiteral("Note.notebookGuid")) {
            errorDescription += QT_TR_NOOP("note's notebook is not owned by user");
            if (note.hasNotebookGuid()) {
                QNWARNING(errorDescription << QStringLiteral(", notebook guid: ") << note.notebookGuid());
            }
        }
        else if (!thrownOnCreation && (userException.parameter.ref() == QStringLiteral("Note"))) {
            errorDescription += QT_TR_NOOP("note is not owned by user");
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter");
            errorDescription += QStringLiteral(": ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::QUOTA_REACHED)
    {
        errorDescription = QT_TR_NOOP("QUOTA_REACHED exception during the attempt to");
        errorDescription += QStringLiteral(" ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QStringLiteral(" ");
        errorDescription += QT_TR_NOOP("note: note exceeds upload quota limit");
        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QT_TR_NOOP("note"), userException, source, errorDescription);
}

qint32 NoteStore::processUnexpectedEdamUserException(const QString & typeName,
                                                     const qevercloud::EDAMUserException & userException,
                                                     const NoteStore::UserExceptionSource::type & source,
                                                     QNLocalizedString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    errorDescription = QT_TR_NOOP("Unexpected EDAM user exception on attempt to");
    errorDescription += QStringLiteral(" ");
    errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
    errorDescription += QStringLiteral(" ");
    errorDescription += typeName;
    errorDescription += QStringLiteral(": ");
    errorDescription += QT_TR_NOOP("error code");
    errorDescription += QStringLiteral(": ");
    errorDescription += ToString(userException.errorCode);

    if (userException.parameter.isSet()) {
        errorDescription += QStringLiteral(": ");
        errorDescription += QT_TR_NOOP("parameter");
        errorDescription += QStringLiteral(": ");
        errorDescription += userException.parameter.ref();
    }

    return userException.errorCode;
}

qint32 NoteStore::processEdamSystemException(const qevercloud::EDAMSystemException & systemException,
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

void NoteStore::processEdamNotFoundException(const qevercloud::EDAMNotFoundException & notFoundException,
                                             QNLocalizedString & errorDescription) const
{
    errorDescription = QT_TR_NOOP("Note store could not find data element");

    if (notFoundException.identifier.isSet() && !notFoundException.identifier->isEmpty()) {
        errorDescription += QStringLiteral(": ");
        errorDescription += notFoundException.identifier.ref();
    }

    if (notFoundException.key.isSet() && !notFoundException.key->isEmpty()) {
        errorDescription += QStringLiteral(": ");
        errorDescription += notFoundException.key.ref();
    }
}

} // namespace quentier
