#include "NoteModelItem.h"
#include <quentier/utility/Utility.h>

namespace quentier {

NoteModelItem::NoteModelItem(const QString & localUid,
                             const QString & guid,
                             const QString & notebookLocalUid,
                             const QString & notebookGuid,
                             const QString & title,
                             const QString & previewText,
                             const QImage & thumbnail,
                             const QString & notebookName,
                             const QStringList & tagLocalUids,
                             const QStringList & tagGuids,
                             const QStringList & tagNameList,
                             const qint64 creationTimestamp,
                             const qint64 modificationTimestamp,
                             const qint64 deletionTimestamp,
                             const quint64 sizeInBytes,
                             const bool isSynchronizable,
                             const bool isDirty) :
    m_localUid(localUid),
    m_guid(guid),
    m_notebookLocalUid(notebookLocalUid),
    m_notebookGuid(notebookGuid),
    m_title(title),
    m_previewText(previewText),
    m_thumbnail(thumbnail),
    m_notebookName(notebookName),
    m_tagLocalUids(tagLocalUids),
    m_tagGuids(tagGuids),
    m_tagNameList(tagNameList),
    m_creationTimestamp(creationTimestamp),
    m_modificationTimestamp(modificationTimestamp),
    m_deletionTimestamp(deletionTimestamp),
    m_sizeInBytes(sizeInBytes),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty)
{}

NoteModelItem::~NoteModelItem()
{}

void NoteModelItem::addTagLocalUid(const QString & tagLocalUid)
{
    int index = m_tagLocalUids.indexOf(tagLocalUid);
    if (index >= 0) {
        return;
    }

    m_tagLocalUids << tagLocalUid;
}

void NoteModelItem::removeTagLocalUid(const QString & tagLocalUid)
{
    int index = m_tagLocalUids.indexOf(tagLocalUid);
    if (index < 0) {
        return;
    }

    m_tagLocalUids.removeAt(index);
}

bool NoteModelItem::hasTagLocalUid(const QString & tagLocalUid) const
{
    return m_tagLocalUids.contains(tagLocalUid);
}

int NoteModelItem::numTagLocalUids() const
{
    return m_tagLocalUids.size();
}

void NoteModelItem::addTagGuid(const QString & tagGuid)
{
    int index = m_tagGuids.indexOf(tagGuid);
    if (index >= 0) {
        return;
    }

    m_tagGuids << tagGuid;
}

void NoteModelItem::removeTagGuid(const QString & tagGuid)
{
    int index = m_tagGuids.indexOf(tagGuid);
    if (index < 0) {
        return;
    }

    m_tagGuids.removeAt(index);
}

bool NoteModelItem::hasTagGuid(const QString & tagGuid) const
{
    return m_tagGuids.contains(tagGuid);
}

int NoteModelItem::numTagGuids() const
{
    return m_tagGuids.size();
}

void NoteModelItem::addTagName(const QString & tagName)
{
    int index = m_tagNameList.indexOf(tagName);
    if (index >= 0) {
        return;
    }

    m_tagNameList << tagName;
}

void NoteModelItem::removeTagName(const QString & tagName)
{
    int index = m_tagNameList.indexOf(tagName);
    if (index < 0) {
        return;
    }

    m_tagNameList.removeAt(index);
}

bool NoteModelItem::hasTagName(const QString & tagName) const
{
    return m_tagNameList.contains(tagName);
}

int NoteModelItem::numTagNames() const
{
    return m_tagNameList.size();
}

QTextStream & NoteModelItem::print(QTextStream & strm) const
{
    strm << "NoteModelItem: local uid = " << m_localUid << ", guid = " << m_guid
         << ", notebook local uid = " << m_notebookLocalUid << ", notebook guid = " << m_notebookGuid
         << ", title = " << m_title << ", preview text = " << m_previewText << ", thumbnail "
         << (m_thumbnail.isNull() ? "null" : "not null") << ", notebook name = " << m_notebookName
         << ", tag local uids = " << m_tagLocalUids.join(", ") << ", tag guids = " << m_tagGuids.join(", ")
         << ", tag name list = " << m_tagNameList.join(", ") << ", creation timestamp = " << m_creationTimestamp
         << " (" << printableDateTimeFromTimestamp(m_creationTimestamp) << ")"
         << ", modification timestamp = " << m_modificationTimestamp << " (" << printableDateTimeFromTimestamp(m_modificationTimestamp) << ")"
         << ", deletion timestamp = " << m_deletionTimestamp << " (" << printableDateTimeFromTimestamp(m_deletionTimestamp) << ")"
         << ", size in bytes = " << m_sizeInBytes << ", is synchronizable = " << (m_isSynchronizable ? "true" : "false")
         << ", is dirty = " << (m_isDirty ? "true" : "false");

    return strm;
}

} // namespace quentier
