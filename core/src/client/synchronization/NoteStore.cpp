#include "NoteStore.h"
#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/SavedSearch.h>

namespace qute_note {

NoteStore::NoteStore() :
    m_qecNoteStore()
{}

void NoteStore::setNoteStoreUrl(const QString & noteStoreUrl)
{
    m_qecNoteStore.setNoteStoreUrl(noteStoreUrl);
}

void NoteStore::setAuthenticationToken(const QString & authToken)
{
    m_qecNoteStore.setAuthenticationToken(authToken);
}

qint32 NoteStore::createNotebook(Notebook & notebook, QString & errorDescription,
                                 qint32 & rateLimitSeconds)
{
    try
    {
        notebook = m_qecNoteStore.createNotebook(notebook);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateNotebook(Notebook & notebook, QString & errorDescription,
                                 qint32 & rateLimitSeconds)
{
    try
    {
        qint32 usn = m_qecNoteStore.updateNotebook(notebook);
        notebook.setUpdateSequenceNumber(usn);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::createNote(Note & note, QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        note = m_qecNoteStore.createNote(note);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateNote(Note & note, QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        note = m_qecNoteStore.updateNote(note);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::createTag(Tag & tag, QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        tag = m_qecNoteStore.createTag(tag);
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

qint32 NoteStore::updateTag(Tag & tag, QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        qint32 usn = m_qecNoteStore.updateTag(tag);
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
        // TODO: convert from exception to errorDescription string
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::createSavedSearch(SavedSearch & savedSearch, QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        savedSearch = m_qecNoteStore.createSearch(savedSearch);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::updateSavedSearch(SavedSearch & savedSearch, QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        qint32 usn = m_qecNoteStore.updateSearch(savedSearch);
        savedSearch.setUpdateSequenceNumber(usn);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }
    catch(const qevercloud::EDAMNotFoundException & notFoundException)
    {
        // TODO: convert from exception to errorDescription string
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::getSyncChunk(const qint32 afterUSN, const qint32 maxEntries,
                               const qevercloud::SyncChunkFilter & filter,
                               qevercloud::SyncChunk & syncChunk,
                               QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        syncChunk = m_qecNoteStore.getFilteredSyncChunk(afterUSN, maxEntries, filter);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        // TODO: convert from exception to errorDescription string
    }
    catch(const qevercloud::EDAMSystemException & systemException)
    {
        return processEdamSystemException(systemException, errorDescription,
                                          rateLimitSeconds);
    }

    return qevercloud::EDAMErrorCode::UNKNOWN;
}

qint32 NoteStore::processEdamUserExceptionForTag(const Tag & tag, const qevercloud::EDAMUserException & userException,
                                                 const NoteStore::UserExceptionSource::type & source,
                                                 QString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to " +
                                      QString(thrownOnCreation ? "create" : "update") + " tag");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "Tag.name") {
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
                errorDescription += QT_TR_NOOP("malformed parent guid of tag: ");
                errorDescription += tag.parentGuid();
            }
            else {
                errorDescription += QT_TR_NOOP("error code indicates malformed parent guid but it is empty");
            }
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter: ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        errorDescription = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to " +
                                      QString(thrownOnCreation ? "create" : "update") + " tag");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "Tag.name") {
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
                errorDescription += tag.parentGuid();
            }
            else {
                errorDescription += QT_TR_NOOP("error code indicates the problem with circular parent-child correlation "
                                               "but tag's parent guid is empty");
            }
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter: ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED))
    {
        errorDescription = QT_TR_NOOP("LIMIT_REACHED exception during the attempt to create tag");

        if (userException.parameter.isSet() && (userException.parameter.ref() == "Tag")) {
            errorDescription += QT_TR_NOOP(": already at max number of tags, please remove some of them");
        }

        return userException.errorCode;
    }
    else if (!thrownOnCreation && (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED))
    {
        errorDescription = QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to update tag");

        if (userException.parameter.isSet() && (userException.parameter.ref() == "Tag")) {
            errorDescription += QT_TR_NOOP(": user doesn't own the tag, it can't be updated");
        }

        return userException.errorCode;
    }

    // FIXME: print fine error code instead of number
    QString error = QT_TR_NOOP("Unexpected EDAM user exception on attempt to " +
                               QString(thrownOnCreation ? "create" : "update") + " tag: errorCode = " +
                               QString::number(userException.errorCode));

    if (userException.parameter.isSet()) {
        errorDescription += ": ";
        errorDescription += QT_TR_NOOP("parameter: ");
        errorDescription += userException.parameter.ref();
    }

    return userException.errorCode;
}

qint32 NoteStore::processEdamSystemException(const qevercloud::EDAMSystemException & systemException,
                                             QString & errorDescription, qint32 & rateLimitSeconds) const
{
    rateLimitSeconds = -1;

    if (systemException.errorCode == qevercloud::EDAMErrorCode::RATE_LIMIT_REACHED)
    {
        if (!systemException.rateLimitDuration.isSet()) {
            errorDescription = QT_TR_NOOP("Evernote API rate limit exceeded but no rate limit duration is available");
        }
        else {
            errorDescription = QT_TR_NOOP("Evernote API rate limit exceeded, retry in ");
            errorDescription += QString::number(systemException.rateLimitDuration.ref());
            errorDescription += " ";
            errorDescription += QT_TR_NOOP("seconds");
            rateLimitSeconds = systemException.rateLimitDuration.ref();
        }
    }
    else
    {
        errorDescription = QT_TR_NOOP("Caught EDAM system exception, error code ");
        // FIXME: print fine error code instead of number
        errorDescription += QString::number(systemException.errorCode);
        if (systemException.message.isSet() && !systemException.message->isEmpty()) {
            errorDescription += QT_TR_NOOP(", message: ");
            errorDescription += systemException.message.ref();
        }
    }

    return systemException.errorCode;
}

} // namespace qute_note
