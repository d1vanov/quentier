#ifndef LIB_QUTE_NOTE_TYPES_DATA_TAG_DATA_H
#define LIB_QUTE_NOTE_TYPES_DATA_TAG_DATA_H

#include "FavoritableDataElementData.h"
#include <QEverCloud.h>

namespace qute_note {

class TagData : public FavoritableDataElementData
{
public:
    TagData();
    TagData(const TagData & other);
    TagData(TagData && other);
    TagData(const qevercloud::Tag & other);
    virtual ~TagData();

    void clear();
    bool checkParameters(QString & errorDescription) const;

    bool operator==(const TagData & other) const;
    bool operator!=(const TagData & other) const;

    qevercloud::Tag                 m_qecTag;
    qevercloud::Optional<QString>   m_linkedNotebookGuid;
    qevercloud::Optional<QString>   m_parentLocalUid;

private:
    TagData & operator=(const TagData & other) Q_DECL_DELETE;
    TagData & operator=(TagData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_TYPES_DATA_TAG_DATA_H
