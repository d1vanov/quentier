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

#ifndef LIB_QUENTIER_TYPES_DATA_LINKED_NOTEBOOK_DATA_H
#define LIB_QUENTIER_TYPES_DATA_LINKED_NOTEBOOK_DATA_H

#include <QSharedData>
#include <quentier/utility/QNLocalizedString.h>
#include <QEverCloud.h>

namespace quentier {

class LinkedNotebookData : public QSharedData
{
public:
    LinkedNotebookData();
    LinkedNotebookData(const LinkedNotebookData & other);
    LinkedNotebookData(LinkedNotebookData && other);
    LinkedNotebookData(const qevercloud::LinkedNotebook & other);
    LinkedNotebookData(qevercloud::LinkedNotebook && other);
    virtual ~LinkedNotebookData();

    void clear();
    bool checkParameters(QNLocalizedString & errorDescription) const;

    bool operator==(const LinkedNotebookData & other) const;
    bool operator!=(const LinkedNotebookData & other) const;

    qevercloud::LinkedNotebook    m_qecLinkedNotebook;
    bool                          m_isDirty;

private:
    LinkedNotebookData & operator=(const LinkedNotebookData & other) Q_DECL_DELETE;
    LinkedNotebookData & operator=(LinkedNotebookData && other) Q_DECL_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_LINKED_NOTEBOOK_DATA_H
