#ifndef __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__TAG_DATA_H
#define __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__TAG_DATA_H

#include "LocalStorageDataElementData.h"
#include <QEverCloud.h>
#include <QSharedData>

namespace qute_note {

class TagData : public LocalStorageDataElementData,
                public QSharedData
{
public:
    TagData();
    TagData(const TagData & other);
    TagData(TagData && other);
    TagData(const qevercloud::Tag & other);
    TagData & operator=(const TagData & other);
    TagData & operator=(TagData && other);
    TagData & operator=(const qevercloud::Tag & other);
    virtual ~TagData();

    void clear();
    bool checkParameters(QString & errorDescription) const;

    bool operator==(const TagData & other) const;
    bool operator!=(const TagData & other) const;

    qevercloud::Tag     m_qecTag;
    bool                m_isLocal;
    bool                m_isDeleted;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__TYPES__DATA__TAG_DATA_H
