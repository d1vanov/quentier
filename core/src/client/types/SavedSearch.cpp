#include "SavedSearch.h"
#include "QEverCloudHelpers.h"
#include "../Utility.h"

namespace qute_note {

SavedSearch::SavedSearch(SavedSearch && other) :
    DataElementWithShortcut(std::move(other)),
    m_qecSearch(std::move(other))
{}

SavedSearch & SavedSearch::operator=(SavedSearch && other)
{
    DataElementWithShortcut::operator=(std::move(other));

    if (this != std::addressof(other)) {
        m_qecSearch = std::move(other.m_qecSearch);
    }

    return *this;
}

SavedSearch::SavedSearch(const qevercloud::SavedSearch & search) :
    DataElementWithShortcut(),
    m_qecSearch(search)
{}

SavedSearch::SavedSearch(qevercloud::SavedSearch && search) :
    DataElementWithShortcut(),
    m_qecSearch(std::move(search))
{}

SavedSearch::~SavedSearch()
{}

SavedSearch::operator const qevercloud::SavedSearch & () const
{
    return m_qecSearch;
}

SavedSearch::operator qevercloud::SavedSearch & ()
{
    return m_qecSearch;
}

bool SavedSearch::operator==(const SavedSearch & other) const
{
    if (hasShortcut() != other.hasShortcut()) {
        return false;
    }
    else if (isDirty() != other.isDirty()) {
        return false;
    }
    else if (m_qecSearch != other.m_qecSearch) {
        return false;
    }

    return true;
}

bool SavedSearch::operator!=(const SavedSearch & other) const
{
    return !(*this == other);
}

void SavedSearch::clear()
{
    m_qecSearch = qevercloud::SavedSearch();
}

bool SavedSearch::hasGuid() const
{
    return m_qecSearch.guid.isSet();
}

const QString & SavedSearch::guid() const
{
    return m_qecSearch.guid;
}

void SavedSearch::setGuid(const QString & guid)
{
    m_qecSearch.guid = guid;
}

bool SavedSearch::hasUpdateSequenceNumber() const
{
    return m_qecSearch.updateSequenceNum.isSet();
}

qint32 SavedSearch::updateSequenceNumber() const
{
    return m_qecSearch.updateSequenceNum;
}

void SavedSearch::setUpdateSequenceNumber(const qint32 usn)
{
    m_qecSearch.updateSequenceNum = usn;
}

bool SavedSearch::checkParameters(QString & errorDescription) const
{
    if (localGuid().isEmpty() && !m_qecSearch.guid.isSet()) {
        errorDescription = QT_TR_NOOP("Both saved search's local and remote guids are empty");
        return false;
    }

    if (m_qecSearch.guid.isSet() && !CheckGuid(m_qecSearch.guid.ref())) {
        errorDescription = QT_TR_NOOP("Saved search's guid is invalid: ") + m_qecSearch.guid;
        return false;
    }

    if (m_qecSearch.name.isSet())
    {
        const QString & name = m_qecSearch.name;
        int nameSize = name.size();

        if ( (nameSize < qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MIN) ||
             (nameSize > qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Saved search's name exceeds allowed size: ") + name;
            return false;
        }

        if (name.at(0) == ' ') {
            errorDescription = QT_TR_NOOP("Saved search's name can't begin from space: ") + name;
            return false;
        }
        else if (name.at(nameSize - 1) == ' ') {
            errorDescription = QT_TR_NOOP("Saved search's name can't end with space: ") + name;
            return false;
        }
    }

    if (m_qecSearch.updateSequenceNum.isSet() && !CheckUpdateSequenceNumber(m_qecSearch.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Saved search's update sequence number is invalid: ");
        errorDescription.append(QString::number(m_qecSearch.updateSequenceNum));
        return false;
    }

    if (m_qecSearch.query.isSet())
    {
        const QString & query = m_qecSearch.query;
        int querySize = query.size();

        if ( (querySize < qevercloud::EDAM_SEARCH_QUERY_LEN_MIN) ||
             (querySize > qevercloud::EDAM_SEARCH_QUERY_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Saved search's query exceeds allowed size: ") + query;
            return false;
        }
    }

    if (m_qecSearch.format.isSet() && (static_cast<QueryFormat>(m_qecSearch.format) != qevercloud::QueryFormat::USER)) {
        errorDescription = QT_TR_NOOP("Saved search has unsupported query format");
        return false;
    }

    return true;

}

bool SavedSearch::hasName() const
{
    return m_qecSearch.name.isSet();
}

const QString & SavedSearch::name() const
{
    return m_qecSearch.name;
}

void SavedSearch::setName(const QString & name)
{
    m_qecSearch.name = name;
}

bool SavedSearch::hasQuery() const
{
    return m_qecSearch.query.isSet();
}

const QString & SavedSearch::query() const
{
    return m_qecSearch.query;
}

void SavedSearch::setQuery(const QString & query)
{
    m_qecSearch.query = query;
}

bool SavedSearch::hasQueryFormat() const
{
    return m_qecSearch.format.isSet();
}

SavedSearch::QueryFormat SavedSearch::queryFormat() const
{
    return m_qecSearch.format;
}

void SavedSearch::setQueryFormat(const qint8 queryFormat)
{
    if (queryFormat <= QueryFormat::SEXP) {
        m_qecSearch.format = static_cast<QueryFormat>(queryFormat);
    }
    else {
        m_qecSearch.format.clear();
    }
}

#define CHECK_AND_SET_SCOPE() \
    if (!m_qecSearch.scope.isSet()) { \
        m_qecSearch.scope = SavedSearchScope(); \
    }

bool SavedSearch::hasIncludeAccount() const
{
    return m_qecSearch.scope.isSet() && m_qecSearch.scope->includeAccount.isSet();
}

bool SavedSearch::includeAccount() const
{
    return m_qecSearch.scope->includeAccount;
}

void SavedSearch::setIncludeAccount(const bool includeAccount)
{
    CHECK_AND_SET_SCOPE();
    m_qecSearch.scope->includeAccount = includeAccount;
}

bool SavedSearch::hasIncludePersonalLinkedNotebooks() const
{
    return m_qecSearch.scope.isSet() && m_qecSearch.scope->includePersonalLinkedNotebooks.isSet();
}

bool SavedSearch::includePersonalLinkedNotebooks() const
{
    return m_qecSearch.scope->includePersonalLinkedNotebooks;
}

void SavedSearch::setIncludePersonalLinkedNotebooks(const bool includePersonalLinkedNotebooks)
{
    CHECK_AND_SET_SCOPE();
    m_qecSearch.scope->includePersonalLinkedNotebooks = includePersonalLinkedNotebooks;
}

bool SavedSearch::hasIncludeBusinessLinkedNotebooks() const
{
    return m_qecSearch.scope.isSet() && m_qecSearch.scope->includeBusinessLinkedNotebooks.isSet();
}

bool SavedSearch::includeBusinessLinkedNotebooks() const
{    
    return m_qecSearch.scope->includeBusinessLinkedNotebooks;
}

void SavedSearch::setIncludeBusinessLinkedNotebooks(const bool includeBusinessLinkedNotebooks)
{
    CHECK_AND_SET_SCOPE();
    m_qecSearch.scope->includeBusinessLinkedNotebooks = includeBusinessLinkedNotebooks;
}

QTextStream & SavedSearch::Print(QTextStream & strm) const
{
    strm << "Saved search: { \n" ;

    const QString _localGuid = localGuid();
    if (!_localGuid.isEmpty()) {
        strm << "localGuid: " << _localGuid << "; \n";
    }
    else {
        strm << "localGuid is not set; \n";
    }

    if (m_qecSearch.guid.isSet()) {
        strm << "guid: " << m_qecSearch.guid << "; \n";
    }
    else {
        strm << "guid is not set; \n";
    }

    if (m_qecSearch.updateSequenceNum.isSet()) {
        strm << "updateSequenceNumber: " << QString::number(m_qecSearch.updateSequenceNum) << "; \n";
    }
    else {
        strm << "updateSequenceNumber is not set; \n";
    }

    if (m_qecSearch.name.isSet()) {
        strm << "name: " << m_qecSearch.name << "; \n";
    }
    else {
        strm << "name is not set; \n";
    }

    if (m_qecSearch.query.isSet()) {
        strm << "query: " << m_qecSearch.query << "; \n";
    }
    else {
        strm << "query is not set; \n";
    }

    if (m_qecSearch.format.isSet()) {
        strm << "queryFormat: " << m_qecSearch.format << "; \n";
    }
    else {
        strm << "queryFormat is not set; \n";
    }

    if (m_qecSearch.scope.isSet()) {
        strm << "scope is set; \n";
    }
    else {
        strm << "scope is not set; \n";
        return strm;
    }

    const SavedSearchScope & scope = m_qecSearch.scope;

    if (scope.includeAccount.isSet()) {
        strm << "includeAccount: " << (scope.includeAccount ? "true" : "false") << "; \n";
    }
    else {
        strm << "includeAccount is not set; \n";
    }

    if (scope.includePersonalLinkedNotebooks.isSet()) {
        strm << "includePersonalLinkedNotebooks: " << (scope.includePersonalLinkedNotebooks ? "true" : "false") << "; \n";
    }
    else {
        strm << "includePersonalLinkedNotebooks is not set; \n";
    }

    if (scope.includeBusinessLinkedNotebooks.isSet()) {
        strm << "includeBusinessLinkedNotebooks: " << (scope.includeBusinessLinkedNotebooks ? "true" : "false") << "; \n";
    }
    else {
        strm << "includeBusinessLinkedNotebooks is not set; \n";
    }

    strm << "hasShortcut = " << (hasShortcut() ? "true" : "false") << "; \n";

    return strm;
}

#undef CHECK_AND_SET_SCOPE

} // namepace qute_note
