#ifndef LIB_QUENTIER_TYPES_DATA_TAG_DATA_H
#define LIB_QUENTIER_TYPES_DATA_TAG_DATA_H

#include "FavoritableDataElementData.h"
#include <quentier/utility/QNLocalizedString.h>
#include <QEverCloud.h>

namespace quentier {

class TagData : public FavoritableDataElementData
{
public:
    TagData();
    TagData(const TagData & other);
    TagData(TagData && other);
    TagData(const qevercloud::Tag & other);
    virtual ~TagData();

    void clear();
    bool checkParameters(QNLocalizedString & errorDescription) const;

    bool operator==(const TagData & other) const;
    bool operator!=(const TagData & other) const;

    qevercloud::Tag                 m_qecTag;
    qevercloud::Optional<QString>   m_linkedNotebookGuid;
    qevercloud::Optional<QString>   m_parentLocalUid;

private:
    TagData & operator=(const TagData & other) Q_DECL_DELETE;
    TagData & operator=(TagData && other) Q_DECL_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_TAG_DATA_H
