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

bool SavedSearch::validateName(const QString & name, ErrorString * pErrorDescription)
{
    if (name != name.trimmed())
    {
        if (pErrorDescription) {
            pErrorDescription->base() = QT_TRANSLATE_NOOP("", "Saved search name cannot start or end with whitespace");
            pErrorDescription->details() = name;
        }

        return false;
    }

    int len = name.length();
    if (len < qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MIN)
    {
        if (pErrorDescription) {
            pErrorDescription->base() = QT_TRANSLATE_NOOP("", "Saved search name's length is too small");
            pErrorDescription->details() = name;
        }

        return false;
    }

    if (len > qevercloud::EDAM_SAVED_SEARCH_NAME_LEN_MAX)
    {
        if (pErrorDescription) {
            pErrorDescription->base() = QT_TRANSLATE_NOOP("", "Saved search's name length is too large");
            pErrorDescription->details() = name;
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

bool SavedSearch::checkParameters(ErrorString & errorDescription) const
{
    if (localUid().isEmpty() && !d->m_qecSearch.guid.isSet()) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Both saved search's local and remote guids are empty");
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
    strm << QStringLiteral("Saved search: { \n");

    const QString localUid_ = localUid();
    if (!localUid_.isEmpty()) {
        strm << QStringLiteral("localUid: ") << localUid_ << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("localUid is not set; \n");
    }

    if (d->m_qecSearch.guid.isSet()) {
        strm << QStringLiteral("guid: ") << d->m_qecSearch.guid << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("guid is not set; \n");
    }

    if (d->m_qecSearch.updateSequenceNum.isSet()) {
        strm << QStringLiteral("updateSequenceNumber: ") << QString::number(d->m_qecSearch.updateSequenceNum) << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("updateSequenceNumber is not set; \n");
    }

    if (d->m_qecSearch.name.isSet()) {
        strm << QStringLiteral("name: ") << d->m_qecSearch.name << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("name is not set; \n");
    }

    if (d->m_qecSearch.query.isSet()) {
        strm << QStringLiteral("query: ") << d->m_qecSearch.query << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("query is not set; \n");
    }

    if (d->m_qecSearch.format.isSet()) {
        strm << QStringLiteral("queryFormat: ") << d->m_qecSearch.format << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("queryFormat is not set; \n");
    }

    if (d->m_qecSearch.scope.isSet()) {
        strm << QStringLiteral("scope is set; \n");
    }
    else {
        strm << QStringLiteral("scope is not set; \n");
        return strm;
    }

    const SavedSearchScope & scope = d->m_qecSearch.scope;

    if (scope.includeAccount.isSet()) {
        strm << QStringLiteral("includeAccount: ") << (scope.includeAccount ? QStringLiteral("true") : QStringLiteral("false"))
             << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("includeAccount is not set; \n");
    }

    if (scope.includePersonalLinkedNotebooks.isSet()) {
        strm << QStringLiteral("includePersonalLinkedNotebooks: ") << (scope.includePersonalLinkedNotebooks ? QStringLiteral("true") : QStringLiteral("false"))
             << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("includePersonalLinkedNotebooks is not set; \n");
    }

    if (scope.includeBusinessLinkedNotebooks.isSet()) {
        strm << QStringLiteral("includeBusinessLinkedNotebooks: ") << (scope.includeBusinessLinkedNotebooks ? QStringLiteral("true") : QStringLiteral("false"))
             << QStringLiteral("; \n");
    }
    else {
        strm << QStringLiteral("includeBusinessLinkedNotebooks is not set; \n");
    }

    strm << QStringLiteral("isFavorited = ") << (isFavorited() ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral("; \n");

    strm << QStringLiteral("}; \n");
    return strm;
}

#undef CHECK_AND_SET_SCOPE

} // namepace quentier
