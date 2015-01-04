#include "Tag.h"
#include "data/TagData.h"

namespace qute_note {

QN_DEFINE_LOCAL_GUID(Tag)
QN_DEFINE_DIRTY(Tag)
QN_DEFINE_LOCAL(Tag)
QN_DEFINE_SHORTCUT(Tag)

Tag::Tag() :
    d(new TagData)
{}

Tag::Tag(const Tag & other) :
    d(other.d)
{}

Tag::Tag(Tag && other) :
    d(std::move(other.d))
{}

Tag & Tag::operator=(Tag && other)
{
    if (this != &other) {
        d = std::move(other.d);
    }

    return *this;
}

Tag::Tag(const qevercloud::Tag & other) :
    d(new TagData(other))
{}

Tag::Tag(qevercloud::Tag && other) :
    d(new TagData(std::move(other)))
{}

Tag & Tag::operator=(const Tag & other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

Tag::~Tag()
{}

bool Tag::operator==(const Tag & other) const
{
    if (hasShortcut() != other.hasShortcut()) {
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

bool Tag::operator!=(const Tag & other) const
{
    return !(*this == other);
}

Tag::operator const qevercloud::Tag &() const
{
    return d->m_qecTag;
}

Tag::operator qevercloud::Tag &()
{
    return d->m_qecTag;
}

void Tag::clear()
{
    d->clear();
}

bool Tag::hasGuid() const
{
    return d->m_qecTag.guid.isSet();
}

const QString & Tag::guid() const
{
    return d->m_qecTag.guid;
}

void Tag::setGuid(const QString & guid)
{
    if (!guid.isEmpty()) {
        d->m_qecTag.guid = guid;
    }
    else {
        d->m_qecTag.guid.clear();
    }
}

bool Tag::hasUpdateSequenceNumber() const
{
    return d->m_qecTag.updateSequenceNum.isSet();
}

qint32 Tag::updateSequenceNumber() const
{
    return d->m_qecTag.updateSequenceNum;
}

void Tag::setUpdateSequenceNumber(const qint32 usn)
{
    d->m_qecTag.updateSequenceNum = usn;
}

bool Tag::checkParameters(QString & errorDescription) const
{
    if (localGuid().isEmpty() && !d->m_qecTag.guid.isSet()) {
        errorDescription = QT_TR_NOOP("both tag's local and remote guids are empty");
        return false;
    }

    return d->checkParameters(errorDescription);
}

bool Tag::isDeleted() const
{
    return d->m_isDeleted;
}

void Tag::setDeleted(const bool deleted)
{
    d->m_isDeleted = deleted;
}

bool Tag::hasName() const
{
    return d->m_qecTag.name.isSet();
}

const QString & Tag::name() const
{
    return d->m_qecTag.name;
}

void Tag::setName(const QString & name)
{
    if (!name.isEmpty()) {
        d->m_qecTag.name = name;
    }
    else {
        d->m_qecTag.name.clear();
    }
}

bool Tag::hasParentGuid() const
{
    return d->m_qecTag.parentGuid.isSet();
}

const QString & Tag::parentGuid() const
{
    return d->m_qecTag.parentGuid;
}

void Tag::setParentGuid(const QString & parentGuid)
{
    if (!parentGuid.isEmpty()) {
        d->m_qecTag.parentGuid = parentGuid;
    }
    else {
        d->m_qecTag.parentGuid.clear();
    }
}

bool Tag::hasLinkedNotebookGuid() const
{
    return d->m_linkedNotebookGuid.isSet();
}

const QString & Tag::linkedNotebookGuid() const
{
    return d->m_linkedNotebookGuid;
}

void Tag::setLinkedNotebookGuid(const QString & linkedNotebookGuid)
{
    if (!linkedNotebookGuid.isEmpty()) {
        d->m_linkedNotebookGuid = linkedNotebookGuid;
    }
    else {
        d->m_linkedNotebookGuid.clear();
    }
}

QTextStream & Tag::Print(QTextStream & strm) const
{
    strm << "Tag { \n";

    if (d->m_qecTag.guid.isSet()) {
        strm << "guid: " << d->m_qecTag.guid << "; \n";
    }
    else {
        strm << "guid is not set; \n";
    }

    if (d->m_linkedNotebookGuid.isSet()) {
        strm << "linked notebook guid: " << d->m_linkedNotebookGuid << "; \n";
    }
    else {
        strm << "linked notebook guid is not set; \n";
    }

    if (d->m_qecTag.name.isSet()) {
        strm << "name: " << d->m_qecTag.name << "; \n";
    }
    else {
        strm << "name is not set; \n";
    }

    if (d->m_qecTag.parentGuid.isSet()) {
        strm << "parentGuid: " << d->m_qecTag.parentGuid << "; \n";
    }
    else {
        strm << "parentGuid is not set; \n";
    }

    if (d->m_qecTag.updateSequenceNum.isSet()) {
        strm << "updateSequenceNumber: " << QString::number(d->m_qecTag.updateSequenceNum) << "; \n";
    }
    else {
        strm << "updateSequenceNumber is not set; \n";
    }

    strm << "isDirty: " << (isDirty() ? "true" : "false") << "; \n";
    strm << "isLocal: " << (d->m_isLocal ? "true" : "false") << "; \n";
    strm << "isDeleted: " << (d->m_isDeleted ? "true" : "false") << "; \n";
    strm << "hasShortcut = " << (hasShortcut() ? "true" : "false") << "; \n";
    strm << "}; \n";

    return strm;
}

} // namespace qute_note
