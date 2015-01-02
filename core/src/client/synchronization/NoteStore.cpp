#include "NoteStore.h"
#include <client/types/Notebook.h>
#include <client/types/Note.h>
#include <client/types/Tag.h>
#include <client/types/SavedSearch.h>
#include <logging/QuteNoteLogger.h>
#include <tools/QuteNoteCheckPtr.h>

namespace qute_note {

NoteStore::NoteStore(QSharedPointer<qevercloud::NoteStore> pQecNoteStore) :
    m_pQecNoteStore(pQecNoteStore)
{
    QUTE_NOTE_CHECK_PTR(m_pQecNoteStore)
}

void NoteStore::setNoteStoreUrl(const QString & noteStoreUrl)
{
    m_pQecNoteStore->setNoteStoreUrl(noteStoreUrl);
}

void NoteStore::setAuthenticationToken(const QString & authToken)
{
    m_pQecNoteStore->setAuthenticationToken(authToken);
}

qint32 NoteStore::createNotebook(Notebook & notebook, QString & errorDescription,
                                 qint32 & rateLimitSeconds)
{
    try
    {
        notebook = m_pQecNoteStore->createNotebook(notebook);
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

qint32 NoteStore::updateNotebook(Notebook & notebook, QString & errorDescription,
                                 qint32 & rateLimitSeconds)
{
    try
    {
        qint32 usn = m_pQecNoteStore->updateNotebook(notebook);
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

qint32 NoteStore::createNote(Note & note, QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        note = m_pQecNoteStore->createNote(note);
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

qint32 NoteStore::updateNote(Note & note, QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        note = m_pQecNoteStore->updateNote(note);
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

qint32 NoteStore::createTag(Tag & tag, QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        tag = m_pQecNoteStore->createTag(tag);
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
        qint32 usn = m_pQecNoteStore->updateTag(tag);
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

qint32 NoteStore::createSavedSearch(SavedSearch & savedSearch, QString & errorDescription, qint32 & rateLimitSeconds)
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

qint32 NoteStore::updateSavedSearch(SavedSearch & savedSearch, QString & errorDescription, qint32 & rateLimitSeconds)
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

qint32 NoteStore::getSyncChunk(const qint32 afterUSN, const qint32 maxEntries,
                               const qevercloud::SyncChunkFilter & filter,
                               qevercloud::SyncChunk & syncChunk,
                               QString & errorDescription, qint32 & rateLimitSeconds)
{
    try
    {
        syncChunk = m_pQecNoteStore->getFilteredSyncChunk(afterUSN, maxEntries, filter);
        return 0;
    }
    catch(const qevercloud::EDAMUserException & userException)
    {
        return processEdamUserExceptionForSyncChunk(userException, afterUSN, maxEntries,
                                                    errorDescription);
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
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" tag");

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
        errorDescription = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" tag");

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

    return processUnexpectedEdamUserException(QT_TR_NOOP("tag"), userException,
                                              source, errorDescription);
}

qint32 NoteStore::processEdamUserExceptionForSavedSearch(const SavedSearch & search,
                                                         const qevercloud::EDAMUserException & userException,
                                                         const NoteStore::UserExceptionSource::type & source,
                                                         QString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" saved search");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "SavedSearch.name") {
            if (search.hasName()) {
                errorDescription += QT_TR_NOOP("invalid length or pattern of saved search's name: ");
                errorDescription += search.name();
            }
            else {
                errorDescription += QT_TR_NOOP("saved search has no name");
            }
        }
        else if (userException.parameter.ref() == "SavedSearch.query") {
            if (search.hasQuery()) {
                errorDescription += QT_TR_NOOP("invalid length of saved search's query: ");
                errorDescription += QString::number(search.query().length());
                QNWARNING(errorDescription << ", query: " << search.query());
            }
            else {
                errorDescription += QT_TR_NOOP("saved search has no query");
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
        errorDescription = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" saved search");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "SavedSearch.name") {
            if (search.hasName()) {
                errorDescription += QT_TR_NOOP("saved search's name is already in use: ");
                errorDescription += search.name();
            }
            else {
                errorDescription += QT_TR_NOOP("saved search has no name");
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

qint32 NoteStore::processEdamUserExceptionForSyncChunk(const qevercloud::EDAMUserException & userException,
                                                       const qint32 afterUSN, const qint32 maxEntries,
                                                       QString & errorDescription) const
{
    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to get sync chunk");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "afterUSN") {
            errorDescription += QT_TR_NOOP("afterUSN is negative: ");
            errorDescription += QString::number(afterUSN);
        }
        else if (userException.parameter.ref() == "maxEntries") {
            errorDescription += QT_TR_NOOP("maxEntries is less than 1: ");
            errorDescription += QString::number(maxEntries);
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter: ");
            errorDescription += userException.parameter.ref();
        }
    }
    else
    {
        errorDescription += QT_TR_NOOP("Unknown EDAM user exception on attempt to get filtered sync chunk");
    }

    return userException.errorCode;
}

qint32 NoteStore::processEdamUserExceptionForNotebook(const Notebook & notebook,
                                                      const qevercloud::EDAMUserException & userException,
                                                      const NoteStore::UserExceptionSource::type & source,
                                                      QString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" notebook");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "Notebook.name") {
            if (notebook.hasName()) {
                errorDescription += QT_TR_NOOP("invalid length or pattern of notebook's name: ");
                errorDescription += notebook.name();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no name");
            }
        }
        else if (userException.parameter.ref() == "Notebook.stack") {
            if (notebook.hasStack()) {
                errorDescription += QT_TR_NOOP("invalid length or pattern of notebook's stack: ");
                errorDescription += notebook.stack();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no stack");
            }
        }
        else if (userException.parameter.ref() == "Publishing.uri") {
            if (notebook.hasPublishingUri()) {
                errorDescription += QT_TR_NOOP("invalid publishing uri for notebook: ");
                errorDescription += notebook.publishingUri();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no publishing uri");
            }
        }
        else if (userException.parameter.ref() == "Publishing.publicDescription")
        {
            if (notebook.hasPublishingPublicDescription()) {
                errorDescription += QT_TR_NOOP("public description for notebook is too long: ");
                errorDescription += notebook.publishingPublicDescription();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no public description");
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
        errorDescription = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" notebook");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "Notebook.name") {
            if (notebook.hasName()) {
                errorDescription += QT_TR_NOOP("notebook's name is already in use: ");
                errorDescription += notebook.name();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no name");
            }
        }
        else if (userException.parameter.ref() == "Publishing.uri") {
            if (notebook.hasPublishingUri()) {
                errorDescription += QT_TR_NOOP("notebook's publishing uri is already in use: ");
                errorDescription += notebook.publishingUri();
            }
            else {
                errorDescription += QT_TR_NOOP("notebook has no publishing uri");
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
        errorDescription = QT_TR_NOOP("LIMIT_REACHED exception durig the attempt to create notebook");

        if (userException.parameter.isSet() && (userException.parameter.ref() == "Notebook")) {
            errorDescription += QT_TR_NOOP(": already at max number of notebooks, please remove some of them");
        }

        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QT_TR_NOOP("notebook"), userException,
                                              source, errorDescription);
}

qint32 NoteStore::processEdamUserExceptionForNote(const Note & note, const qevercloud::EDAMUserException & userException,
                                                  const NoteStore::UserExceptionSource::type & source,
                                                  QString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    const auto exceptionData = userException.exceptionData();

    if (userException.errorCode == qevercloud::EDAMErrorCode::BAD_DATA_FORMAT)
    {
        errorDescription = QT_TR_NOOP("BAD_DATA_FORMAT exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "Note.title") {
            if (note.hasTitle()) {
                errorDescription += QT_TR_NOOP("invalid length or pattetn of note's title: ");
                errorDescription += note.title();
            }
            else {
                errorDescription += QT_TR_NOOP("note has no title");
            }
        }
        else if (userException.parameter.ref() == "Note.content") {
            if (note.hasContent()) {
                errorDescription += QT_TR_NOOP("invalid lenfth for note's ENML content: ");
                errorDescription += QString::number(note.content().length());
                QNWARNING(errorDescription << ", note's content: " << note.content());
            }
            else {
                errorDescription += QT_TR_NOOP("note has no content");
            }
        }
        else if (userException.parameter.ref().startsWith("NoteAttributes.")) {
            if (note.hasNoteAttributes()) {
                errorDescription += QT_TR_NOOP("invalid note attributes");
                QNWARNING(errorDescription << ": " << note.noteAttributes());
            }
            else {
                errorDescription += QT_TR_NOOP("note has no attributes");
            }
        }
        else if (userException.parameter.ref().startsWith("ResourceAttributes.")) {
            errorDescription += QT_TR_NOOP("invalid resource attributes for some of note's resources");
            QNWARNING(errorDescription << ", note: " << note);
        }
        else if (userException.parameter.ref() == "Resource.mime") {
            errorDescription += QT_TR_NOOP("invalid mime type for some of note's resources");
            QNWARNING(errorDescription << ", note: " << note);
        }
        else if (userException.parameter.ref() == "Tag.name") {
            errorDescription += QT_TR_NOOP("Note.tagNames was provided and one "
                                           "of the specified tags had an invalid length or pattern");
            QNWARNING(errorDescription << ", note: " << note);
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter: ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_CONFLICT)
    {
        errorDescription = QT_TR_NOOP("DATA_CONFLICT exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "Note.deleted") {
            errorDescription += QT_TR_NOOP("deletion timestamp is set on active note");
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::DATA_REQUIRED)
    {
        errorDescription = QT_TR_NOOP("DATA_REQUIRED exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "Resource.data") {
            errorDescription += QT_TR_NOOP("Data body for some of note's resources is missing");
            QNWARNING(errorDescription << ", note: " << note);
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::ENML_VALIDATION)
    {
        errorDescription = QT_TR_NOOP("ENML_VALIDATION exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" note: note's content doesn't validate against DTD");
        QNWARNING(errorDescription << ", note: " << note);
        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::LIMIT_REACHED)
    {
        errorDescription = QT_TR_NOOP("LIMIT_REACHED exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (thrownOnCreation && (userException.parameter.ref() == "Note")) {
            errorDescription += QT_TR_NOOP("already at maximum number of notes per account");
        }
        else if (userException.parameter.ref() == "Note.size") {
            errorDescription += QT_TR_NOOP("total note size is too large");
        }
        else if (userException.parameter.ref() == "Note.resources") {
            errorDescription += QT_TR_NOOP("too many resources on note");
        }
        else if (userException.parameter.ref() == "Note.tagGuids") {
            errorDescription += QT_TR_NOOP("too many tags on note");
        }
        else if (userException.parameter.ref() == "Resource.data.size") {
            errorDescription += QT_TR_NOOP("one of note's resource's data is too large");
        }
        else if (userException.parameter.ref().startsWith("NoteAttribute.")) {
            errorDescription += QT_TR_NOOP("note attributes string is too large");
            if (note.hasNoteAttributes()) {
                QNWARNING(errorDescription << ", note attributes: " << note.noteAttributes());
            }
        }
        else if (userException.parameter.ref().startsWith("ResourceAttribute.")) {
            errorDescription += QT_TR_NOOP("one of note's resources has too large resource attributes string");
            QNWARNING(errorDescription << ", note: " << note);
        }
        else if (userException.parameter.ref() == "Tag") {
            errorDescription += QT_TR_NOOP("Note.tagNames was provided, and the required new tags "
                                           "would exceed the maximum number per account");
            QNWARNING(errorDescription << ", note: " << note);
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter: ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::PERMISSION_DENIED)
    {
        errorDescription = QT_TR_NOOP("PERMISSION_DENIED exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" note");

        if (!userException.parameter.isSet())
        {
            if (!exceptionData.isNull() && !exceptionData->errorMessage.isEmpty()) {
                errorDescription += ": ";
                errorDescription += exceptionData->errorMessage;
            }

            return userException.errorCode;
        }

        if (userException.parameter.ref() == "Note.notebookGuid") {
            errorDescription += QT_TR_NOOP("note's notebook is not owned by user");
            if (note.hasNotebookGuid()) {
                QNWARNING(errorDescription << ", notebook guid: " << note.notebookGuid());
            }
        }
        else if (!thrownOnCreation && (userException.parameter.ref() == "Note")) {
            errorDescription += QT_TR_NOOP("note is not owned by user");
        }
        else {
            errorDescription += QT_TR_NOOP("unexpected parameter: ");
            errorDescription += userException.parameter.ref();
        }

        return userException.errorCode;
    }
    else if (userException.errorCode == qevercloud::EDAMErrorCode::QUOTA_REACHED)
    {
        errorDescription = QT_TR_NOOP("QUOTA_REACHED exception during the attempt to ");
        errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
        errorDescription += QT_TR_NOOP(" note: note exceeds upload quota limit");
        return userException.errorCode;
    }

    return processUnexpectedEdamUserException(QT_TR_NOOP("note"), userException,
                                              source, errorDescription);
}

qint32 NoteStore::processUnexpectedEdamUserException(const QString & typeName,
                                                     const qevercloud::EDAMUserException & userException,
                                                     const NoteStore::UserExceptionSource::type & source,
                                                     QString & errorDescription) const
{
    bool thrownOnCreation = (source == UserExceptionSource::Creation);

    errorDescription = QT_TR_NOOP("Unexpected EDAM user exception on attempt to ");
    errorDescription += (thrownOnCreation ? QT_TR_NOOP("create") : QT_TR_NOOP("update"));
    errorDescription += typeName;
    errorDescription += QT_TR_NOOP(": error code = ");
    errorDescription += ToQString(userException.errorCode);

    if (userException.parameter.isSet()) {
        errorDescription += QT_TR_NOOP(": parameter: ");
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
        errorDescription += ToQString(systemException.errorCode);
        if (systemException.message.isSet() && !systemException.message->isEmpty()) {
            errorDescription += QT_TR_NOOP(", message: ");
            errorDescription += systemException.message.ref();
        }
    }

    return systemException.errorCode;
}

void NoteStore::processEdamNotFoundException(const qevercloud::EDAMNotFoundException & notFoundException,
                                             QString & errorDescription) const
{
    errorDescription = QT_TR_NOOP("Note store could not find data element");

    if (notFoundException.identifier.isSet() && !notFoundException.identifier->isEmpty()) {
        errorDescription += ": ";
        errorDescription += notFoundException.identifier.ref();
    }

    if (notFoundException.key.isSet() && !notFoundException.key->isEmpty()) {
        errorDescription += ": ";
        errorDescription += notFoundException.key.ref();
    }
}

} // namespace qute_note
