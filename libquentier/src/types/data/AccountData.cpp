#include "AccountData.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/generated/constants.h>
#else
#include <qt4qevercloud/generated/constants.h>
#endif

#include <limits>

namespace quentier {

AccountData::AccountData() :
    QSharedData(),
    m_name(),
    m_displayName(),
    m_accountType(Account::Type::Local),
    m_evernoteAccountType(Account::EvernoteAccountType::Free),
    m_userId(-1),
    m_mailLimitDaily(std::numeric_limits<qint32>::max()),
    m_noteSizeMax(std::numeric_limits<qint64>::max()),
    m_resourceSizeMax(std::numeric_limits<qint64>::max()),
    m_linkedNotebookMax(std::numeric_limits<qint32>::max()),
    m_noteCountMax(std::numeric_limits<qint32>::max()),
    m_notebookCountMax(std::numeric_limits<qint32>::max()),
    m_tagCountMax(std::numeric_limits<qint32>::max()),
    m_noteTagCountMax(std::numeric_limits<qint32>::max()),
    m_savedSearchCountMax(std::numeric_limits<qint32>::max()),
    m_noteResourceCountMax(std::numeric_limits<qint32>::max())
{
    if (m_accountType != Account::Type::Local) {
        switchEvernoteAccountType(m_evernoteAccountType);
    }
}

AccountData::~AccountData()
{}

AccountData::AccountData(const AccountData & other) :
    QSharedData(other),
    m_name(other.m_name),
    m_displayName(other.m_displayName),
    m_accountType(other.m_accountType),
    m_evernoteAccountType(other.m_evernoteAccountType),
    m_userId(other.m_userId),
    m_mailLimitDaily(other.m_mailLimitDaily),
    m_noteSizeMax(other.m_noteSizeMax),
    m_resourceSizeMax(other.m_resourceSizeMax),
    m_linkedNotebookMax(other.m_linkedNotebookMax),
    m_noteCountMax(other.m_noteCountMax),
    m_notebookCountMax(other.m_notebookCountMax),
    m_tagCountMax(other.m_tagCountMax),
    m_noteTagCountMax(other.m_noteTagCountMax),
    m_savedSearchCountMax(other.m_savedSearchCountMax),
    m_noteResourceCountMax(other.m_noteResourceCountMax)
{}

AccountData::AccountData(AccountData && other) :
    QSharedData(std::move(other)),
    m_name(std::move(other.m_name)),
    m_displayName(std::move(other.m_displayName)),
    m_accountType(std::move(other.m_accountType)),
    m_evernoteAccountType(std::move(other.m_evernoteAccountType)),
    m_userId(std::move(other.m_userId)),
    m_mailLimitDaily(std::move(other.m_mailLimitDaily)),
    m_noteSizeMax(std::move(other.m_noteSizeMax)),
    m_resourceSizeMax(std::move(other.m_resourceSizeMax)),
    m_linkedNotebookMax(std::move(other.m_linkedNotebookMax)),
    m_noteCountMax(std::move(other.m_noteCountMax)),
    m_notebookCountMax(std::move(other.m_notebookCountMax)),
    m_tagCountMax(std::move(other.m_tagCountMax)),
    m_noteTagCountMax(std::move(other.m_noteTagCountMax)),
    m_savedSearchCountMax(std::move(other.m_savedSearchCountMax)),
    m_noteResourceCountMax(std::move(other.m_noteResourceCountMax))
{}

void AccountData::switchEvernoteAccountType(const Account::EvernoteAccountType::type evernoteAccountType)
{
    m_evernoteAccountType = evernoteAccountType;
    m_mailLimitDaily = mailLimitDaily();
    m_noteSizeMax = noteSizeMax();
    m_resourceSizeMax = resourceSizeMax();
    m_linkedNotebookMax = linkedNotebookMax();
    m_noteCountMax = noteCountMax();
    m_notebookCountMax = notebookCountMax();
    m_tagCountMax = tagCountMax();
    m_noteTagCountMax = noteTagCountMax();
    m_savedSearchCountMax = savedSearchCountMax();
    m_noteResourceCountMax = noteResourceCountMax();
}

void AccountData::setEvernoteAccountLimits(const qevercloud::AccountLimits & limits)
{
    m_mailLimitDaily = (limits.userMailLimitDaily.isSet() ? limits.userMailLimitDaily.ref() : mailLimitDaily());
    m_noteSizeMax = (limits.noteSizeMax.isSet() ? limits.noteSizeMax.ref() : noteSizeMax());
    m_resourceSizeMax = (limits.resourceSizeMax.isSet() ? limits.resourceSizeMax.ref() : resourceSizeMax());
    m_linkedNotebookMax = (limits.userLinkedNotebookMax.isSet() ? limits.userLinkedNotebookMax.ref() : linkedNotebookMax());
    m_noteCountMax = (limits.userNoteCountMax.isSet() ? limits.userNoteCountMax.ref() : noteCountMax());
    m_notebookCountMax = (limits.userNotebookCountMax.isSet() ? limits.userNotebookCountMax.ref() : notebookCountMax());
    m_tagCountMax = (limits.userTagCountMax.isSet() ? limits.userTagCountMax.ref() : tagCountMax());
    m_noteTagCountMax = (limits.noteTagCountMax.isSet() ? limits.noteTagCountMax.ref() : noteTagCountMax());
    m_savedSearchCountMax = (limits.userSavedSearchesMax.isSet() ? limits.userSavedSearchesMax.ref() : savedSearchCountMax());
    m_noteResourceCountMax = (limits.noteResourceCountMax.isSet() ? limits.noteResourceCountMax.ref() : noteResourceCountMax());
}

qint32 AccountData::mailLimitDaily() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint32>::max();
    }

    switch(m_evernoteAccountType)
    {
    case Account::EvernoteAccountType::Premium:
        return qevercloud::EDAM_USER_MAIL_LIMIT_DAILY_PREMIUM;
    default:
        return qevercloud::EDAM_USER_MAIL_LIMIT_DAILY_FREE;
    }
}

qint64 AccountData::noteSizeMax() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint64>::max();
    }

    switch(m_evernoteAccountType)
    {
    case Account::EvernoteAccountType::Premium:
        return qevercloud::EDAM_NOTE_SIZE_MAX_PREMIUM;
    default:
        return qevercloud::EDAM_NOTE_SIZE_MAX_FREE;
    }
}

qint64 AccountData::resourceSizeMax() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint64>::max();
    }

    switch(m_evernoteAccountType)
    {
    case Account::EvernoteAccountType::Premium:
        return qevercloud::EDAM_RESOURCE_SIZE_MAX_PREMIUM;
    default:
        return qevercloud::EDAM_RESOURCE_SIZE_MAX_FREE;
    }
}

qint32 AccountData::linkedNotebookMax() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint32>::max();
    }

    switch(m_evernoteAccountType)
    {
    case Account::EvernoteAccountType::Premium:
        return qevercloud::EDAM_USER_LINKED_NOTEBOOK_MAX_PREMIUM;
    default:
        return qevercloud::EDAM_USER_LINKED_NOTEBOOK_MAX;
    }
}

qint32 AccountData::noteCountMax() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint32>::max();
    }

    switch(m_evernoteAccountType)
    {
    case Account::EvernoteAccountType::Business:
        return qevercloud::EDAM_BUSINESS_NOTES_MAX;
    default:
        return qevercloud::EDAM_USER_NOTES_MAX;
    }
}

qint32 AccountData::notebookCountMax() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint32>::max();
    }

    switch(m_evernoteAccountType)
    {
    case Account::EvernoteAccountType::Business:
        return qevercloud::EDAM_BUSINESS_NOTEBOOKS_MAX;
    default:
        return qevercloud::EDAM_USER_NOTEBOOKS_MAX;
    }
}

qint32 AccountData::tagCountMax() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint32>::max();
    }

    switch(m_evernoteAccountType)
    {
    case Account::EvernoteAccountType::Business:
        return qevercloud::EDAM_BUSINESS_TAGS_MAX;
    default:
        return qevercloud::EDAM_USER_TAGS_MAX;
    }
}

qint32 AccountData::noteTagCountMax() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint32>::max();
    }

    return qevercloud::EDAM_NOTE_TAGS_MAX;
}

qint32 AccountData::savedSearchCountMax() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint32>::max();
    }

    return qevercloud::EDAM_USER_SAVED_SEARCHES_MAX;
}

qint32 AccountData::noteResourceCountMax() const
{
    if (m_accountType == Account::Type::Local) {
        return std::numeric_limits<qint32>::max();
    }

    return qevercloud::EDAM_NOTE_RESOURCES_MAX;
}

} // namespace quentier
