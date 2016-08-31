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
    m_accountType(Account::Type::Local),
    m_evernoteAccountType(Account::EvernoteAccountType::Free),
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
    m_accountType(other.m_accountType),
    m_evernoteAccountType(other.m_evernoteAccountType),
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
    m_accountType(std::move(other.m_accountType)),
    m_evernoteAccountType(std::move(other.m_evernoteAccountType)),
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
    m_mailLimitDaily = mailLimitDaily(evernoteAccountType);
    m_noteSizeMax = noteSizeMax(evernoteAccountType);
    m_resourceSizeMax = resourceSizeMax(evernoteAccountType);
    m_linkedNotebookMax = linkedNotebookMax(evernoteAccountType);
    m_noteCountMax = noteCountMax(evernoteAccountType);
    m_notebookCountMax = notebookCountMax(evernoteAccountType);
    m_tagCountMax = tagCountMax(evernoteAccountType);
    m_noteTagCountMax = noteTagCountMax(evernoteAccountType);
    m_savedSearchCountMax = savedSearchCountMax(evernoteAccountType);
    m_noteResourceCountMax = noteResourceCountMax(evernoteAccountType);
}

// TODO: these methods would need to be refined after the Evernote API is migrated to 1.28 (or later) version

qint32 AccountData::mailLimitDaily(const Account::EvernoteAccountType::type evernoteAccountType)
{
    switch(evernoteAccountType)
    {
    case Account::EvernoteAccountType::Premium:
        return qevercloud::EDAM_USER_MAIL_LIMIT_DAILY_PREMIUM;
    default:
        return qevercloud::EDAM_USER_MAIL_LIMIT_DAILY_FREE;
    }
}

qint64 AccountData::noteSizeMax(const Account::EvernoteAccountType::type evernoteAccountType)
{
    switch(evernoteAccountType)
    {
    case Account::EvernoteAccountType::Premium:
        return qevercloud::EDAM_NOTE_SIZE_MAX_PREMIUM;
    default:
        return qevercloud::EDAM_NOTE_SIZE_MAX_FREE;
    }
}

qint64 AccountData::resourceSizeMax(const Account::EvernoteAccountType::type evernoteAccountType)
{
    switch(evernoteAccountType)
    {
    case Account::EvernoteAccountType::Premium:
        return qevercloud::EDAM_RESOURCE_SIZE_MAX_PREMIUM;
    default:
        return qevercloud::EDAM_RESOURCE_SIZE_MAX_FREE;
    }
}

qint32 AccountData::linkedNotebookMax(const Account::EvernoteAccountType::type evernoteAccountType)
{
    switch(evernoteAccountType)
    {
    case Account::EvernoteAccountType::Premium:
        return qevercloud::EDAM_USER_LINKED_NOTEBOOK_MAX_PREMIUM;
    default:
        return qevercloud::EDAM_USER_LINKED_NOTEBOOK_MAX;
    }
}

qint32 AccountData::noteCountMax(const Account::EvernoteAccountType::type evernoteAccountType)
{
    switch(evernoteAccountType)
    {
    case Account::EvernoteAccountType::Business:
        return qevercloud::EDAM_BUSINESS_NOTES_MAX;
    default:
        return qevercloud::EDAM_USER_NOTES_MAX;
    }
}

qint32 AccountData::notebookCountMax(const Account::EvernoteAccountType::type evernoteAccountType)
{
    switch(evernoteAccountType)
    {
    case Account::EvernoteAccountType::Business:
        return qevercloud::EDAM_BUSINESS_NOTEBOOKS_MAX;
    default:
        return qevercloud::EDAM_USER_NOTEBOOKS_MAX;
    }
}

qint32 AccountData::tagCountMax(const Account::EvernoteAccountType::type evernoteAccountType)
{
    switch(evernoteAccountType)
    {
    case Account::EvernoteAccountType::Business:
        return qevercloud::EDAM_BUSINESS_TAGS_MAX;
    default:
        return qevercloud::EDAM_USER_TAGS_MAX;
    }
}

qint32 AccountData::noteTagCountMax(const Account::EvernoteAccountType::type evernoteAccountType)
{
    Q_UNUSED(evernoteAccountType)
    return qevercloud::EDAM_NOTE_TAGS_MAX;
}

qint32 AccountData::savedSearchCountMax(const Account::EvernoteAccountType::type evernoteAccountType)
{
    Q_UNUSED(evernoteAccountType)
    return qevercloud::EDAM_USER_SAVED_SEARCHES_MAX;
}

qint32 AccountData::noteResourceCountMax(const Account::EvernoteAccountType::type evernoteAccountType)
{
    Q_UNUSED(evernoteAccountType)
    return qevercloud::EDAM_NOTE_RESOURCES_MAX;
}

} // namespace quentier
