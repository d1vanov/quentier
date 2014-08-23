#include "Tag.h"
#include "QEverCloudHelpers.h"
#include "../Utility.h"

namespace qute_note {

Tag::Tag() :
    DataElementWithShortcut(),
    m_qecTag(),
    m_isLocal(true),
    m_isDeleted(false)
{}

Tag::Tag(Tag && other) :
    DataElementWithShortcut(std::move(other)),
    m_qecTag(std::move(other.m_qecTag)),
    m_isLocal(std::move(other.m_isLocal)),
    m_isDeleted(std::move(other.m_isDeleted))
{}

Tag & Tag::operator=(Tag && other)
{
    DataElementWithShortcut::operator=(std::move(other));

    if (this != std::addressof(other)) {
        m_qecTag = std::move(other.m_qecTag);
        m_isLocal = std::move(other.m_isLocal);
        m_isDeleted = std::move(other.m_isDeleted);
    }

    return *this;
}

Tag::Tag(const qevercloud::Tag & other) :
    DataElementWithShortcut(),
    m_qecTag(other),
    m_isLocal(true),
    m_isDeleted(false)
{}

Tag::Tag(qevercloud::Tag && other) :
    DataElementWithShortcut(),
    m_qecTag(std::move(other)),
    m_isLocal(true),
    m_isDeleted(false)
{}

Tag::~Tag()
{}

bool Tag::operator==(const Tag & other) const
{
    if (hasShortcut() != other.hasShortcut()) {
        return false;
    }
    else if (isDirty() != other.isDirty()) {
        return false;
    }
    else if (m_isLocal != other.m_isLocal) {
        return false;
    }
    else if (m_isDeleted != other.m_isDeleted) {
        return false;
    }
    else if (m_qecTag != other.m_qecTag) {
        return false;
    }

    return true;
}

bool Tag::operator!=(const Tag & other) const
{
    return !(*this == other);
}

void Tag::clear()
{
    m_qecTag = qevercloud::Tag();
}

bool Tag::hasGuid() const
{
    return m_qecTag.guid.isSet();
}

const QString & Tag::guid() const
{
    return m_qecTag.guid;
}

void Tag::setGuid(const QString & guid)
{
    m_qecTag.guid = guid;
}

bool Tag::hasUpdateSequenceNumber() const
{
    return m_qecTag.updateSequenceNum.isSet();
}

qint32 Tag::updateSequenceNumber() const
{
    return m_qecTag.updateSequenceNum;
}

void Tag::setUpdateSequenceNumber(const qint32 usn)
{
    m_qecTag.updateSequenceNum = usn;
}

bool Tag::checkParameters(QString & errorDescription) const
{
    if (localGuid().isEmpty() && !m_qecTag.guid.isSet()) {
        errorDescription = QT_TR_NOOP("both tag's local and remote guids are empty");
        return false;
    }

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

bool Tag::isLocal() const
{
    return m_isLocal;
}

void Tag::setLocal(const bool local)
{
    m_isLocal = local;
}

bool Tag::isDeleted() const
{
    return m_isDeleted;
}

void Tag::setDeleted(const bool deleted)
{
    m_isDeleted = deleted;
}

bool Tag::hasName() const
{
    return m_qecTag.name.isSet();
}

const QString & Tag::name() const
{
    return m_qecTag.name;
}

void Tag::setName(const QString & name)
{
    m_qecTag.name = name;
}

bool Tag::hasParentGuid() const
{
    return m_qecTag.parentGuid.isSet();
}

const QString & Tag::parentGuid() const
{
    return m_qecTag.parentGuid;
}

void Tag::setParentGuid(const QString & parentGuid)
{    
    m_qecTag.parentGuid = parentGuid;
}

QTextStream & Tag::Print(QTextStream & strm) const
{
    strm << "Tag { \n";

    if (m_qecTag.guid.isSet()) {
        strm << "guid: " << m_qecTag.guid << "; \n";
    }
    else {
        strm << "guid is not set; \n";
    }

    if (m_qecTag.name.isSet()) {
        strm << "name: " << m_qecTag.name << "; \n";
    }
    else {
        strm << "name is not set; \n";
    }

    if (m_qecTag.parentGuid.isSet()) {
        strm << "parentGuid: " << m_qecTag.parentGuid << "; \n";
    }
    else {
        strm << "parentGuid is not set; \n";
    }

    if (m_qecTag.updateSequenceNum.isSet()) {
        strm << "updateSequenceNumber: " << QString::number(m_qecTag.updateSequenceNum) << "; \n";
    }
    else {
        strm << "updateSequenceNumber is not set; \n";
    }

    strm << "isDirty: " << (isDirty() ? "true" : "false") << "; \n";
    strm << "isLocal: " << (m_isLocal ? "true" : "false") << "; \n";
    strm << "isDeleted: " << (m_isDeleted ? "true" : "false") << "; \n";
    strm << "hasShortcut = " << (hasShortcut() ? "true" : "false") << "; \n";
    strm << "}; \n";

    return strm;
}



} // namespace qute_note
