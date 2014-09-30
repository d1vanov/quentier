#include "SavedSearchData.h"
#include "../../Utility.h"
#include "../QEverCloudHelpers.h"

namespace qute_note {

SavedSearchData::SavedSearchData() :
    DataElementWithShortcutData(),
    m_qecSearch()
{
}

SavedSearchData::SavedSearchData(const SavedSearchData & other) :
    DataElementWithShortcutData(other),
    m_qecSearch(other.m_qecSearch)
{}

SavedSearchData::SavedSearchData(SavedSearchData && other) :
    DataElementWithShortcutData(std::move(other)),
    m_qecSearch(std::move(other.m_qecSearch))
{}

SavedSearchData::SavedSearchData(const qevercloud::SavedSearch & other) :
    DataElementWithShortcutData(),
    m_qecSearch(other)
{}

SavedSearchData::SavedSearchData(qevercloud::SavedSearch && other) :
    DataElementWithShortcutData(),
    m_qecSearch(std::move(other))
{}

SavedSearchData & SavedSearchData::operator=(const SavedSearchData & other)
{
    DataElementWithShortcutData::operator=(other);

    if (this != std::addressof(other)) {
        m_qecSearch = other.m_qecSearch;
    }

    return *this;
}

SavedSearchData & SavedSearchData::operator=(SavedSearchData && other)
{
    DataElementWithShortcutData::operator=(std::move(other));

    if (this != std::addressof(other)) {
        m_qecSearch = std::move(other.m_qecSearch);
    }

    return *this;
}

SavedSearchData & SavedSearchData::operator=(const qevercloud::SavedSearch & other)
{
    m_qecSearch = other;
    return *this;
}

SavedSearchData::~SavedSearchData()
{}

void SavedSearchData::clear()
{
    m_qecSearch = qevercloud::SavedSearch();
}

bool SavedSearchData::checkParameters(QString &errorDescription) const
{
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

    if (m_qecSearch.format.isSet() && (static_cast<qevercloud::QueryFormat::type>(m_qecSearch.format) != qevercloud::QueryFormat::USER)) {
        errorDescription = QT_TR_NOOP("Saved search has unsupported query format");
        return false;
    }

    return true;
}

bool SavedSearchData::operator==(const SavedSearchData & other) const
{
    return (m_qecSearch == other.m_qecSearch);
}

bool SavedSearchData::operator!=(const SavedSearchData & other) const
{
    return !(*this != other);
}

} // namespace qute_note
