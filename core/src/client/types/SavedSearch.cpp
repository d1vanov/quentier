#include "SavedSearch.h"
#include "../Utility.h"
#include <Limits_constants.h>

namespace qute_note {

SavedSearch::SavedSearch() :
    NoteStoreDataElement(),
    m_enSearch()
{}

SavedSearch::SavedSearch(const SavedSearch & other) :
    NoteStoreDataElement(other),
    m_enSearch(other.m_enSearch)
{}

SavedSearch & SavedSearch::operator=(const SavedSearch & other)
{
    if (this != &other) {
        NoteStoreDataElement::operator=(other);
        m_enSearch = other.m_enSearch;
    }

    return *this;
}

SavedSearch::~SavedSearch()
{}

void SavedSearch::clear()
{
    m_enSearch = evernote::edam::SavedSearch();
}

bool SavedSearch::hasGuid() const
{
    return m_enSearch.__isset.guid;
}

const QString SavedSearch::guid() const
{
    return std::move(QString::fromStdString(m_enSearch.guid));
}

void SavedSearch::setGuid(const QString & guid)
{
    m_enSearch.guid = guid.toStdString();
    m_enSearch.__isset.guid = true;
}

bool SavedSearch::hasUpdateSequenceNumber() const
{
    return m_enSearch.__isset.updateSequenceNum;
}

qint32 SavedSearch::updateSequenceNumber() const
{
    return m_enSearch.updateSequenceNum;
}

void SavedSearch::setUpdateSequenceNumber(const qint32 usn)
{
    m_enSearch.updateSequenceNum = usn;
    m_enSearch.__isset.updateSequenceNum = true;
}

bool SavedSearch::checkParameters(QString & errorDescription) const
{
    if (!m_enSearch.__isset.guid) {
        errorDescription = QObject::tr("Saved search's guid is not set");
        return false;
    }
    else if (!CheckGuid(m_enSearch.guid)) {
        errorDescription = QObject::tr("Saved search's guid is invalid: ");
        errorDescription.append(QString::fromStdString(m_enSearch.guid));
        return false;
    }

    if (!m_enSearch.__isset.name) {
        errorDescription = QObject::tr("Saved search's name is not set");
        return false;
    }
    else {
        size_t nameSize = m_enSearch.name.size();

        if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_SAVED_SEARCH_NAME_LEN_MIN) ||
             (nameSize > evernote::limits::g_Limits_constants.EDAM_SAVED_SEARCH_NAME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Saved search's name exceeds allowed size: ");
            errorDescription.append(QString::fromStdString(m_enSearch.name));
            return false;
        }

        if (m_enSearch.name.at(0) == ' ') {
            errorDescription = QObject::tr("Saved search's name can't begin from space: ");
            errorDescription.append(QString::fromStdString(m_enSearch.name));
            return false;
        }
        else if (m_enSearch.name.at(nameSize - 1) == ' ') {
            errorDescription = QObject::tr("Saved search's name can't end with space: ");
            errorDescription.append(QString::fromStdString(m_enSearch.name));
            return false;
        }
    }

    if (!m_enSearch.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Saved search's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(m_enSearch.updateSequenceNum)) {
        errorDescription = QObject::tr("Saved search's update sequence number is invalid: ");
        errorDescription.append(m_enSearch.updateSequenceNum);
        return false;
    }

    if (!m_enSearch.__isset.query) {
        errorDescription = QObject::tr("Saved search's query is not set");
        return false;
    }
    else {
        size_t querySize = m_enSearch.query.size();

        if ( (querySize < evernote::limits::g_Limits_constants.EDAM_SEARCH_QUERY_LEN_MIN) ||
             (querySize > evernote::limits::g_Limits_constants.EDAM_SEARCH_QUERY_LEN_MAX) )
        {
            errorDescription = QObject::tr("Saved search's query exceeds allowed size: ");
            errorDescription.append(QString::fromStdString(m_enSearch.query));
            return false;
        }
    }

    if (!m_enSearch.__isset.format) {
        errorDescription = QObject::tr("Saved search's format is not set");
        return false;
    }
    else if (m_enSearch.format != evernote::edam::QueryFormat::USER) {
        errorDescription = QObject::tr("Saved search has unsupported query format");
        return false;
    }

    if (!m_enSearch.__isset.scope) {
        errorDescription = QObject::tr("Saved search's scope is not set");
        return false;
    }
    else {
        const auto & scopeIsSet = m_enSearch.scope.__isset;

        if (!scopeIsSet.includeAccount) {
            errorDescription = QObject::tr("Include account option in saved search's scope is not set");
            return false;
        }

        if (!scopeIsSet.includePersonalLinkedNotebooks) {
            errorDescription = QObject::tr("Include personal linked notebooks option in saved search's scope is not set");
            return false;
        }

        if (!scopeIsSet.includeBusinessLinkedNotebooks) {
            errorDescription = QObject::tr("Include business linked notebooks option in saved search's scope is not set");
            return false;
        }
    }

    return true;

}

bool SavedSearch::hasName() const
{
    return m_enSearch.__isset.name;
}

const QString SavedSearch::name() const
{
    return std::move(QString::fromStdString(m_enSearch.name));
}

void SavedSearch::setName(const QString & name)
{
    m_enSearch.name = name.toStdString();
    m_enSearch.__isset.name = true;
}

bool SavedSearch::hasQuery() const
{
    return m_enSearch.__isset.query;
}

const QString SavedSearch::query() const
{
    return std::move(QString::fromStdString(m_enSearch.query));
}

void SavedSearch::setQuery(const QString & query)
{
    m_enSearch.query = query.toStdString();
    m_enSearch.__isset.query = true;
}

bool SavedSearch::hasQueryFormat() const
{
    return m_enSearch.__isset.format;
}

qint8 SavedSearch::queryFormat() const
{
    return m_enSearch.format;
}

void SavedSearch::setQueryFormat(const qint8 queryFormat)
{
    m_enSearch.format = static_cast<evernote::edam::QueryFormat::type>(queryFormat);
    m_enSearch.__isset.format = true;
}

#define CHECK_AND_SET_SCOPE \
    const auto & isSet = m_enSearch.scope.__isset; \
    if (isSet.includeAccount && isSet.includePersonalLinkedNotebooks && isSet.includeBusinessLinkedNotebooks) { \
        m_enSearch.__isset.scope = true; \
    }

bool SavedSearch::hasIncludeAccount() const
{
    return m_enSearch.__isset.scope && m_enSearch.scope.__isset.includeAccount;
}

bool SavedSearch::includeAccount() const
{
    return m_enSearch.scope.includeAccount;
}

void SavedSearch::setIncludeAccount(const bool includeAccount)
{
    m_enSearch.scope.includeAccount = includeAccount;
    m_enSearch.scope.__isset.includeAccount = true;
    CHECK_AND_SET_SCOPE
}

bool SavedSearch::hasIncludePersonalLinkedNotebooks() const
{
    return m_enSearch.__isset.scope && m_enSearch.scope.__isset.includePersonalLinkedNotebooks;
}

bool SavedSearch::includePersonalLinkedNotebooks() const
{
    return m_enSearch.scope.includePersonalLinkedNotebooks;
}

void SavedSearch::setIncludePersonalLinkedNotebooks(const bool includePersonalLinkedNotebooks)
{
    m_enSearch.scope.includePersonalLinkedNotebooks = includePersonalLinkedNotebooks;
    m_enSearch.scope.__isset.includePersonalLinkedNotebooks = true;
    CHECK_AND_SET_SCOPE
}

bool SavedSearch::hasIncludeBusinessLinkedNotebooks() const
{
    return m_enSearch.__isset.scope && m_enSearch.scope.__isset.includeBusinessLinkedNotebooks;
}

bool SavedSearch::includeBusinessLinkedNotebooks() const
{
    return m_enSearch.scope.includeBusinessLinkedNotebooks;
}

void SavedSearch::setIncludeBusinessLinkedNotebooks(const bool includeBusinessLinkedNotebooks)
{
    m_enSearch.scope.includeBusinessLinkedNotebooks = includeBusinessLinkedNotebooks;
    m_enSearch.scope.__isset.includeBusinessLinkedNotebooks = true;
    CHECK_AND_SET_SCOPE
}

QTextStream & SavedSearch::Print(QTextStream & strm) const
{
    // TODO: implement
}

#undef CHECK_AND_SET_SCOPE

} // namepace qute_note
