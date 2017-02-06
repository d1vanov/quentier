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

#include "SavedSearchData.h"
#include <quentier/types/SavedSearch.h>
#include <quentier/utility/Utility.h>

namespace quentier {

SavedSearchData::SavedSearchData() :
    FavoritableDataElementData(),
    m_qecSearch()
{
}

SavedSearchData::SavedSearchData(const SavedSearchData & other) :
    FavoritableDataElementData(other),
    m_qecSearch(other.m_qecSearch)
{}

SavedSearchData::SavedSearchData(SavedSearchData && other) :
    FavoritableDataElementData(std::move(other)),
    m_qecSearch(std::move(other.m_qecSearch))
{}

SavedSearchData::SavedSearchData(const qevercloud::SavedSearch & other) :
    FavoritableDataElementData(),
    m_qecSearch(other)
{}

SavedSearchData::SavedSearchData(qevercloud::SavedSearch && other) :
    FavoritableDataElementData(),
    m_qecSearch(std::move(other))
{}

SavedSearchData::~SavedSearchData()
{}

void SavedSearchData::clear()
{
    m_qecSearch = qevercloud::SavedSearch();
}

bool SavedSearchData::checkParameters(ErrorString & errorDescription) const
{
    if (m_qecSearch.guid.isSet() && !checkGuid(m_qecSearch.guid.ref())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Saved search's guid is invalid");
        errorDescription.details() = m_qecSearch.guid;
        return false;
    }

    if (m_qecSearch.name.isSet() && !SavedSearch::validateName(m_qecSearch.name, &errorDescription)) {
        return false;
    }

    if (m_qecSearch.updateSequenceNum.isSet() && !checkUpdateSequenceNumber(m_qecSearch.updateSequenceNum)) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Saved search's update sequence number is invalid");
        errorDescription.details() = QString::number(m_qecSearch.updateSequenceNum);
        return false;
    }

    if (m_qecSearch.query.isSet())
    {
        const QString & query = m_qecSearch.query;
        int querySize = query.size();

        if ( (querySize < qevercloud::EDAM_SEARCH_QUERY_LEN_MIN) ||
             (querySize > qevercloud::EDAM_SEARCH_QUERY_LEN_MAX) )
        {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "Saved search's query exceeds allowed size");
            errorDescription.details() = query;
            return false;
        }
    }

    if (m_qecSearch.format.isSet() && (static_cast<qevercloud::QueryFormat::type>(m_qecSearch.format) != qevercloud::QueryFormat::USER)) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Saved search has unsupported query format");
        errorDescription.details() = QString::number(m_qecSearch.format.ref());
        return false;
    }

    return true;
}

bool SavedSearchData::operator==(const SavedSearchData & other) const
{
    return (m_qecSearch == other.m_qecSearch) &&
           (m_isDirty == other.m_isDirty) &&
           (m_isLocal == other.m_isLocal) &&
           (m_isFavorited == other.m_isFavorited);
}

bool SavedSearchData::operator!=(const SavedSearchData & other) const
{
    return !(*this == other);
}

} // namespace quentier
