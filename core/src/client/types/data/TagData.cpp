#include "TagData.h"
#include "../../Utility.h"
#include "../QEverCloudHelpers.h"

namespace qute_note {

TagData::TagData() :
    DataElementWithShortcutData(),
    m_qecTag(),
    m_isLocal(true),
    m_isDeleted(false)
{}

TagData::TagData(const TagData & other) :
    DataElementWithShortcutData(other),
    m_qecTag(other.m_qecTag),
    m_isLocal(other.m_isLocal),
    m_isDeleted(other.m_isDeleted)
{}

TagData::TagData(TagData && other) :
    DataElementWithShortcutData(std::move(other)),
    m_qecTag(std::move(other.m_qecTag)),
    m_isLocal(std::move(other.m_isLocal)),
    m_isDeleted(std::move(other.m_isDeleted))
{}

TagData::TagData(const qevercloud::Tag & other) :
    DataElementWithShortcutData(),
    m_qecTag(other),
    m_isLocal(true),
    m_isDeleted(false)
{}

TagData::~TagData()
{}

void TagData::clear()
{
    m_qecTag = qevercloud::Tag();
}

bool TagData::checkParameters(QString & errorDescription) const
{
    if (m_qecTag.guid.isSet() && !CheckGuid(m_qecTag.guid.ref())) {
        errorDescription = QT_TR_NOOP("Tag's guid is invalid: ") + m_qecTag.guid;
        return false;
    }

    if (m_qecTag.name.isSet())
    {
        int nameSize = m_qecTag.name->size();
        if ( (nameSize < qevercloud::EDAM_TAG_NAME_LEN_MIN) ||
             (nameSize > qevercloud::EDAM_TAG_NAME_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Tag's name exceeds allowed size: ") + m_qecTag.name;
            return false;
        }

        if (m_qecTag.name->startsWith(' ')) {
            errorDescription = QT_TR_NOOP("Tag's name can't begin from space: ") + m_qecTag.name;
            return false;
        }
        else if (m_qecTag.name->endsWith(' ')) {
            errorDescription = QT_TR_NOOP("Tag's name can't end with space: ") + m_qecTag.name;
            return false;
        }

        if (m_qecTag.name->contains(',')) {
            errorDescription = QT_TR_NOOP("Tag's name can't contain comma: ") + m_qecTag.name;
            return false;
        }
    }

    if (m_qecTag.updateSequenceNum.isSet() && !CheckUpdateSequenceNumber(m_qecTag.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Tag's update sequence number is invalid: ");
        errorDescription += QString::number(m_qecTag.updateSequenceNum);
        return false;
    }

    if (m_qecTag.parentGuid.isSet() && !CheckGuid(m_qecTag.parentGuid.ref())) {
        errorDescription = QT_TR_NOOP("Tag's parent guid is invalid: ") + m_qecTag.parentGuid;
        return false;
    }

    return true;
}

bool TagData::operator==(const TagData & other) const
{
    return (m_qecTag == other.m_qecTag) && (m_isLocal == other.m_isLocal) &&
            (m_isDeleted == other.m_isDeleted);
}

bool TagData::operator!=(const TagData & other) const
{
    return !(*this == other);
}

} // namespace
