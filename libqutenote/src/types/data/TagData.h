#ifndef __LIB_QUTE_NOTE__TYPES__DATA__TAG_DATA_H
#define __LIB_QUTE_NOTE__TYPES__DATA__TAG_DATA_H

#include "DataElementWithShortcutData.h"
#include <QEverCloud.h>

namespace qute_note {

class TagData : public DataElementWithShortcutData
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
    bool                            m_isDeleted;
    qevercloud::Optional<QString>   m_linkedNotebookGuid;

private:
    TagData & operator=(const TagData & other) Q_DECL_DELETE;
    TagData & operator=(TagData && other) Q_DECL_DELETE;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__TYPES__DATA__TAG_DATA_H
