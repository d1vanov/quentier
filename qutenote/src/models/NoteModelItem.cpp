#include "NoteModelItem.h"
#include <qute_note/utility/Utility.h>

namespace qute_note {

NoteModelItem::NoteModelItem(const QString & localUid,
                             const QString & guid,
                             const QString & notebookLocalUid,
                             const QString & notebookGuid,
                             const QString & title,
                             const QString & previewText,
                             const QImage & thumbnail,
                             const QString & notebookName,
                             const QStringList & tagNameList,
                             const qint64 creationTimestamp,
                             const qint64 modificationTimestamp,
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
    m_tagNameList(tagNameList),
    m_creationTimestamp(creationTimestamp),
    m_modificationTimestamp(modificationTimestamp),
    m_sizeInBytes(sizeInBytes),
    m_isSynchronizable(isSynchronizable),
    m_isDirty(isDirty)
{}

NoteModelItem::~NoteModelItem()
{}

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
    if (index >= 0) {
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
         << (m_thumbnail.isNull() ? "null" : "not null") << ", notebook name = " << m_notebookName << ", tag name list = " << m_tagNameList.join(", ")
         << ", creation timestamp = " << m_creationTimestamp << " (" << printableDateTimeFromTimestamp(m_creationTimestamp) << ")"
         << ", modification timestamp = " << m_modificationTimestamp << " (" << printableDateTimeFromTimestamp(m_modificationTimestamp) << ")"
         << ", size in bytes = " << m_sizeInBytes << ", is synchronizable = " << (m_isSynchronizable ? "true" : "false")
         << ", is dirty = " << (m_isDirty ? "true" : "false");

    return strm;
}

} // namespace qute_note
