#include "Tag.h"
#include "../Utility.h"
#include <Limits_constants.h>

namespace qute_note {

Tag::Tag() :
    NoteStoreDataElement(),
    m_enTag(),
    m_isLocal(true),
    m_isDeleted(false)
{}

Tag::Tag(const Tag & other) :
    NoteStoreDataElement(other),
    m_enTag(other.m_enTag),
    m_isLocal(other.m_isLocal),
    m_isDeleted(other.m_isDeleted)
{}

Tag & Tag::operator=(const Tag & other)
{
    if (this != &other) {
        NoteStoreDataElement::operator=(other);
        m_enTag = other.m_enTag;
        m_isLocal = other.m_isLocal;
        m_isDeleted = other.m_isDeleted;
    }

    return *this;
}

Tag::~Tag()
{}

bool Tag::operator==(const Tag & other) const
{
    return ((m_enTag == other.m_enTag) && (isDirty() == other.isDirty()));
}

bool Tag::operator!=(const Tag & other) const
{
    return !(*this == other);
}

void Tag::clear()
{
    m_enTag = evernote::edam::Tag();
}

bool Tag::hasGuid() const
{
    return m_enTag.__isset.guid;
}

const QString Tag::guid() const
{
    return std::move(QString::fromStdString(m_enTag.guid));
}

void Tag::setGuid(const QString & guid)
{
    m_enTag.guid = guid.toStdString();

    if (guid.isEmpty()) {
        m_enTag.__isset.guid = false;
    }
    else {
        m_enTag.__isset.guid = true;
    }
}

bool Tag::hasUpdateSequenceNumber() const
{
    return m_enTag.__isset.updateSequenceNum;
}

qint32 Tag::updateSequenceNumber() const
{
    return m_enTag.updateSequenceNum;
}

void Tag::setUpdateSequenceNumber(const qint32 usn)
{
    m_enTag.updateSequenceNum = usn;
    m_enTag.__isset.updateSequenceNum = true;
}

bool Tag::checkParameters(QString & errorDescription) const
{
    if (!m_enTag.__isset.guid) {
        errorDescription = QObject::tr("Tag's guid is not set");
        return false;
    }
    else if (!CheckGuid(m_enTag.guid)) {
        errorDescription = QObject::tr("Tag's guid is invalid: ");
        errorDescription.append(QString::fromStdString(m_enTag.guid));
        return false;
    }

    if (!m_enTag.__isset.name) {
        errorDescription = QObject::tr("Tag's name is not set");
        return false;
    }
    else {
        size_t nameSize = m_enTag.name.size();

        if ( (nameSize < evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MIN) ||
             (nameSize > evernote::limits::g_Limits_constants.EDAM_TAG_NAME_LEN_MAX) )
        {
            errorDescription = QObject::tr("Tag's name exceeds allowed size: ");
            errorDescription.append(QString::fromStdString(m_enTag.name));
            return false;
        }

        if (m_enTag.name.at(0) == ' ') {
            errorDescription = QObject::tr("Tag's name can't begin from space: ");
            errorDescription.append(QString::fromStdString(m_enTag.name));
            return false;
        }
        else if (m_enTag.name.at(nameSize - 1) == ' ') {
            errorDescription = QObject::tr("Tag's name can't end with space: ");
            errorDescription.append(QString::fromStdString(m_enTag.name));
            return false;
        }

        if (m_enTag.name.find(',') != m_enTag.name.npos) {
            errorDescription = QObject::tr("Tag's name can't contain comma: ");
            errorDescription.append(QString::fromStdString(m_enTag.name));
            return false;
        }
    }

    if (!m_enTag.__isset.updateSequenceNum) {
        errorDescription = QObject::tr("Tag's update sequence number is not set");
        return false;
    }
    else if (!CheckUpdateSequenceNumber(m_enTag.updateSequenceNum)) {
        errorDescription = QObject::tr("Tag's update sequence number is invalid: ");
        errorDescription.append(m_enTag.updateSequenceNum);
        return false;
    }

    if (m_enTag.__isset.parentGuid && !CheckGuid(m_enTag.parentGuid)) {
        errorDescription = QObject::tr("Tag's parent guid is invalid: ");
        errorDescription.append(QString::fromStdString(m_enTag.parentGuid));
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
    return m_enTag.__isset.name;
}

const QString Tag::name() const
{
    return std::move(QString::fromStdString(m_enTag.name));
}

void Tag::setName(const QString & name)
{
    m_enTag.name = name.toStdString();
    m_enTag.__isset.name = true;
}

bool Tag::hasParentGuid() const
{
    return m_enTag.__isset.parentGuid;
}

const QString Tag::parentGuid() const
{
    return std::move(QString::fromStdString(m_enTag.parentGuid));
}

void Tag::setParentGuid(const QString & parentGuid)
{
    m_enTag.parentGuid = parentGuid.toStdString();

    if (parentGuid.isEmpty()) {
        m_enTag.__isset.parentGuid = false;
    }
    else {
        m_enTag.__isset.parentGuid = true;
    }
}

QTextStream & Tag::Print(QTextStream & strm) const
{
    strm << "Tag { \n";

    if (m_enTag.__isset.guid) {
        strm << "guid: " << QString::fromStdString(m_enTag.guid) << "; \n";
    }
    else {
        strm << "guid is not set; \n";
    }

    if (m_enTag.__isset.name) {
        strm << "name: " << QString::fromStdString(m_enTag.name) << "; \n";
    }
    else {
        strm << "name is not set; \n";
    }

    if (m_enTag.__isset.parentGuid) {
        strm << "parentGuid: " << QString::fromStdString(m_enTag.parentGuid) << "; \n";
    }
    else {
        strm << "parentGuid is not set; \n";
    }

    if (m_enTag.__isset.updateSequenceNum) {
        strm << "updateSequenceNumber: " << QString::number(m_enTag.updateSequenceNum) << "; \n";
    }
    else {
        strm << "updateSequenceNumber is not set; \n";
    }

    strm << "isDirty: " << (isDirty() ? "true" : "false") << "; \n";
    strm << "isLocal: " << (m_isLocal ? "true" : "false") << "; \n";
    strm << "isDeleted: " << (m_isDeleted ? "true" : "false") << "; \n";
    strm << "}; \n";

    return strm;
}



} // namespace qute_note
