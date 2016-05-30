#include "TagData.h"
#include <qute_note/types/Tag.h>
#include <qute_note/utility/Utility.h>

namespace qute_note {

TagData::TagData() :
    DataElementWithShortcutData(),
    m_qecTag(),
    m_linkedNotebookGuid(),
    m_parentLocalUid()
{}

TagData::TagData(const TagData & other) :
    DataElementWithShortcutData(other),
    m_qecTag(other.m_qecTag),
    m_linkedNotebookGuid(other.m_linkedNotebookGuid),
    m_parentLocalUid(other.m_parentLocalUid)
{}

TagData::TagData(TagData && other) :
    DataElementWithShortcutData(std::move(other)),
    m_qecTag(std::move(other.m_qecTag)),
    m_linkedNotebookGuid(std::move(other.m_linkedNotebookGuid)),
    m_parentLocalUid(std::move(other.m_parentLocalUid))
{}

TagData::TagData(const qevercloud::Tag & other) :
    DataElementWithShortcutData(),
    m_qecTag(other),
    m_linkedNotebookGuid(),
    m_parentLocalUid()
{}

TagData::~TagData()
{}

void TagData::clear()
{
    m_qecTag = qevercloud::Tag();
    m_linkedNotebookGuid.clear();
}

bool TagData::checkParameters(QString & errorDescription) const
{
    if (m_qecTag.guid.isSet() && !checkGuid(m_qecTag.guid.ref())) {
        errorDescription = QT_TR_NOOP("Tag's guid is invalid: ") + m_qecTag.guid;
        return false;
    }

    if (m_linkedNotebookGuid.isSet() && !checkGuid(m_linkedNotebookGuid.ref())) {
        errorDescription = QT_TR_NOOP("Tag's linked notebook guid is invalid: ") + m_linkedNotebookGuid;
        return false;
    }

    if (m_qecTag.name.isSet() && !Tag::validateName(m_qecTag.name.ref(), &errorDescription)) {
        return false;
    }

    if (m_qecTag.updateSequenceNum.isSet() && !checkUpdateSequenceNumber(m_qecTag.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Tag's update sequence number is invalid: ");
        errorDescription += QString::number(m_qecTag.updateSequenceNum);
        return false;
    }

    if (m_qecTag.parentGuid.isSet() && !checkGuid(m_qecTag.parentGuid.ref())) {
        errorDescription = QT_TR_NOOP("Tag's parent guid is invalid: ") + m_qecTag.parentGuid;
        return false;
    }

    return true;
}

bool TagData::operator==(const TagData & other) const
{
    return (m_qecTag == other.m_qecTag) &&
           (m_isDirty == other.m_isDirty) &&
           (m_isLocal == other.m_isLocal) &&
           (m_hasShortcut == other.m_hasShortcut) &&
           m_linkedNotebookGuid.isEqual(other.m_linkedNotebookGuid) &&
           m_parentLocalUid.isEqual(other.m_parentLocalUid);
}

bool TagData::operator!=(const TagData & other) const
{
    return !(*this == other);
}

} // namespace
