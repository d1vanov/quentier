/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

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
