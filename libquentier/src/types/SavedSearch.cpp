#include "data/SavedSearchData.h"
#include <quentier/types/SavedSearch.h>
#include <quentier/utility/Utility.h>

namespace quentier {

QN_DEFINE_LOCAL_UID(SavedSearch)
QN_DEFINE_DIRTY(SavedSearch)
QN_DEFINE_LOCAL(SavedSearch)
QN_DEFINE_FAVORITED(SavedSearch)

SavedSearch::SavedSearch() :
    d(new SavedSearchData)
{}

SavedSearch::SavedSearch(const SavedSearch & other) :
    d(other.d)
{}

SavedSearch::SavedSearch(SavedSearch && other) :
    d(std::move(other.d))
{}

SavedSearch & SavedSearch::operator=(SavedSearch && other)
{
    if (this != &other) {
        d = std::move(other.d);
    }

    return *this;
}

SavedSearch::SavedSearch(const qevercloud::SavedSearch & search) :
    d(new SavedSearchData(search))
{}

SavedSearch::SavedSearch(qevercloud::SavedSearch && search) :
    d(new SavedSearchData(std::move(search)))
{}

SavedSearch & SavedSearch::operator=(const SavedSearch & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

SavedSearch::~SavedSearch()
{}

SavedSearch::operator const qevercloud::SavedSearch & () const
{
    return d->m_qecSearch;
}

SavedSearch::operator qevercloud::SavedSearch & ()
{
    return d->m_qecSearch;
}

bool SavedSearch::operator==(const SavedSearch & other) const
{
    if (isFavorited() != other.isFavorited()) {
        return false;
    }
    else if (isLocal() != other.isLocal()) {
        return false;
    }
    else if (isDirty() != other.isDirty()) {
        return false;
    }
    else if (d == other.d) {
        return true;
    }
    else {
        return (*d == *(other.d));
    }
}

bool SavedSearch::operator!=(const SavedSearch & other) const
{
    return !(*this == other);
}

void SavedSearch::clear()
{
    d->clear();
}

bool SavedSearch::validateName(const QString & name, QNLocalizedString * pErrorDescription)
{
    if (name != name.trimmed())
    {
        if (pErrorDescription) {
            *pErrorDescription = QT_TR_NOOP("saved search name cannot start or end with whitespace");
        }

        return false;
    }

    int len = name.length();
    if (len < qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MIN)
    {
        if (pErrorDescription) {
            *pErrorDescription = QT_TR_NOOP("saved search name's length is too small");
        }

        return false;
    }

    if (len > qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MAX)
    {
        if (pErrorDescription) {
            *pErrorDescription = QT_TR_NOOP("saved search's name length is too large");
        }

        return false;
    }

    return true;
}

bool SavedSearch::hasGuid() const
{
    return d->m_qecSearch.guid.isSet();
}

const QString & SavedSearch::guid() const
{
    return d->m_qecSearch.guid;
}

void SavedSearch::setGuid(const QString & guid)
{
    if (!guid.isEmpty()) {
        d->m_qecSearch.guid = guid;
    }
    else {
        d->m_qecSearch.guid.clear();
    }
}

bool SavedSearch::hasUpdateSequenceNumber() const
{
    return d->m_qecSearch.updateSequenceNum.isSet();
}

qint32 SavedSearch::updateSequenceNumber() const
{
    return d->m_qecSearch.updateSequenceNum;
}

void SavedSearch::setUpdateSequenceNumber(const qint32 usn)
{
    d->m_qecSearch.updateSequenceNum = usn;
}

bool SavedSearch::checkParameters(QNLocalizedString & errorDescription) const
{
    if (localUid().isEmpty() && !d->m_qecSearch.guid.isSet()) {
        errorDescription = QT_TR_NOOP("both saved search's local and remote guids are empty");
        return false;
    }

    return d->checkParameters(errorDescription);
}

bool SavedSearch::hasName() const
{
    return d->m_qecSearch.name.isSet();
}

const QString & SavedSearch::name() const
{
    return d->m_qecSearch.name;
}

void SavedSearch::setName(const QString & name)
{
    if (!name.isEmpty()) {
        d->m_qecSearch.name = name;
    }
    else {
        d->m_qecSearch.name.clear();
    }
}

bool SavedSearch::hasQuery() const
{
    return d->m_qecSearch.query.isSet();
}

const QString & SavedSearch::query() const
{
    return d->m_qecSearch.query;
}

void SavedSearch::setQuery(const QString & query)
{
    if (!query.isEmpty()) {
        d->m_qecSearch.query = query;
    }
    else {
        d->m_qecSearch.query.clear();
    }
}

bool SavedSearch::hasQueryFormat() const
{
    return d->m_qecSearch.format.isSet();
}

SavedSearch::QueryFormat SavedSearch::queryFormat() const
{
    return d->m_qecSearch.format;
}

void SavedSearch::setQueryFormat(const qint8 queryFormat)
{
    if (queryFormat <= qevercloud::QueryFormat::SEXP) {
        d->m_qecSearch.format = static_cast<QueryFormat>(queryFormat);
    }
    else {
        d->m_qecSearch.format.clear();
    }
}

#define CHECK_AND_SET_SCOPE() \
    if (!d->m_qecSearch.scope.isSet()) { \
        d->m_qecSearch.scope = SavedSearchScope(); \
    }

bool SavedSearch::hasIncludeAccount() const
{
    return d->m_qecSearch.scope.isSet() && d->m_qecSearch.scope->includeAccount.isSet();
}

bool SavedSearch::includeAccount() const
{
    return d->m_qecSearch.scope->includeAccount;
}

void SavedSearch::setIncludeAccount(const bool includeAccount)
{
    CHECK_AND_SET_SCOPE();
    d->m_qecSearch.scope->includeAccount = includeAccount;
}

bool SavedSearch::hasIncludePersonalLinkedNotebooks() const
{
    return d->m_qecSearch.scope.isSet() && d->m_qecSearch.scope->includePersonalLinkedNotebooks.isSet();
}

bool SavedSearch::includePersonalLinkedNotebooks() const
{
    return d->m_qecSearch.scope->includePersonalLinkedNotebooks;
}

void SavedSearch::setIncludePersonalLinkedNotebooks(const bool includePersonalLinkedNotebooks)
{
    CHECK_AND_SET_SCOPE();
    d->m_qecSearch.scope->includePersonalLinkedNotebooks = includePersonalLinkedNotebooks;
}

bool SavedSearch::hasIncludeBusinessLinkedNotebooks() const
{
    return d->m_qecSearch.scope.isSet() && d->m_qecSearch.scope->includeBusinessLinkedNotebooks.isSet();
}

bool SavedSearch::includeBusinessLinkedNotebooks() const
{
    return d->m_qecSearch.scope->includeBusinessLinkedNotebooks;
}

void SavedSearch::setIncludeBusinessLinkedNotebooks(const bool includeBusinessLinkedNotebooks)
{
    CHECK_AND_SET_SCOPE();
    d->m_qecSearch.scope->includeBusinessLinkedNotebooks = includeBusinessLinkedNotebooks;
}

QTextStream & SavedSearch::print(QTextStream & strm) const
{
    strm << "Saved search: { \n" ;

    const QString localUid_ = localUid();
    if (!localUid_.isEmpty()) {
        strm << "localUid: " << localUid_ << "; \n";
    }
    else {
        strm << "localUid is not set; \n";
    }

    if (d->m_qecSearch.guid.isSet()) {
        strm << "guid: " << d->m_qecSearch.guid << "; \n";
    }
    else {
        strm << "guid is not set; \n";
    }

    if (d->m_qecSearch.updateSequenceNum.isSet()) {
        strm << "updateSequenceNumber: " << QString::number(d->m_qecSearch.updateSequenceNum) << "; \n";
    }
    else {
        strm << "updateSequenceNumber is not set; \n";
    }

    if (d->m_qecSearch.name.isSet()) {
        strm << "name: " << d->m_qecSearch.name << "; \n";
    }
    else {
        strm << "name is not set; \n";
    }

    if (d->m_qecSearch.query.isSet()) {
        strm << "query: " << d->m_qecSearch.query << "; \n";
    }
    else {
        strm << "query is not set; \n";
    }

    if (d->m_qecSearch.format.isSet()) {
        strm << "queryFormat: " << d->m_qecSearch.format << "; \n";
    }
    else {
        strm << "queryFormat is not set; \n";
    }

    if (d->m_qecSearch.scope.isSet()) {
        strm << "scope is set; \n";
    }
    else {
        strm << "scope is not set; \n";
        return strm;
    }

    const SavedSearchScope & scope = d->m_qecSearch.scope;

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

    strm << "isFavorited = " << (isFavorited() ? "true" : "false") << "; \n";

    strm << "}; \n";
    return strm;
}

#undef CHECK_AND_SET_SCOPE

} // namepace quentier
