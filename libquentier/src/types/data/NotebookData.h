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

#ifndef LIB_QUENTIER_TYPES_DATA_NOTEBOOK_DATA_H
#define LIB_QUENTIER_TYPES_DATA_NOTEBOOK_DATA_H

#include "FavoritableDataElementData.h"
#include <quentier/utility/QNLocalizedString.h>
#include <QEverCloud.h>

namespace quentier {

class NotebookData: public FavoritableDataElementData
{
public:
    NotebookData();
    NotebookData(const NotebookData & other);
    NotebookData(NotebookData && other);
    NotebookData(const qevercloud::Notebook & other);
    NotebookData(qevercloud::Notebook && other);

    virtual ~NotebookData();

    bool checkParameters(QNLocalizedString & errorDescription) const;

    bool operator==(const NotebookData & other) const;
    bool operator!=(const NotebookData & other) const;

    qevercloud::Notebook            m_qecNotebook;
    bool                            m_isLastUsed;
    qevercloud::Optional<QString>   m_linkedNotebookGuid;

private:
    NotebookData & operator=(const NotebookData & other) Q_DECL_DELETE;
    NotebookData & operator=(NotebookData && other) Q_DECL_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_NOTEBOOK_DATA_H
