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

#ifndef LIB_QUENTIER_TYPES_DATA_SHARED_NOTEBOOK_WRAPPER_DATA_H
#define LIB_QUENTIER_TYPES_DATA_SHARED_NOTEBOOK_WRAPPER_DATA_H

#include <QSharedData>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <qt5qevercloud/QEverCloud.h>
#else
#include <qt4qevercloud/QEverCloud.h>
#endif

namespace quentier {

class SharedNotebookWrapperData : public QSharedData
{
public:
    SharedNotebookWrapperData();
    SharedNotebookWrapperData(const SharedNotebookWrapperData & other);
    SharedNotebookWrapperData(SharedNotebookWrapperData && other);
    SharedNotebookWrapperData(const qevercloud::SharedNotebook & other);
    SharedNotebookWrapperData(qevercloud::SharedNotebook && other);
    virtual ~SharedNotebookWrapperData();

    qevercloud::SharedNotebook    m_qecSharedNotebook;

private:
    SharedNotebookWrapperData & operator=(const SharedNotebookWrapperData & other) Q_DECL_EQ_DELETE;
    SharedNotebookWrapperData & operator=(SharedNotebookWrapperData && other) Q_DECL_EQ_DELETE;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_DATA_SHARED_NOTEBOOK_WRAPPER_DATA_H
