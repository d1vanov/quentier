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

#ifndef LIB_QUENTIER_TYPES_SHARED_NOTEBOOK_ADAPTER_H
#define LIB_QUENTIER_TYPES_SHARED_NOTEBOOK_ADAPTER_H

#include "ISharedNotebook.h"
#include <QEverCloud.h>

namespace quentier {

/**
 * @brief The SharedNotebookAdapter class uses reference to external qevercloud::SharedNotebook
 * and adapts its interface to that of ISharedNotebook. The instances of this class
 * should be used only within the same scope as the referenced external
 * qevercloud::SharedNotebook object, otherwise it is possible to run into undefined behaviour.
 * SharedNotebookAdapter class is aware of constness of external object it references,
 * it would throw SharedNotebookAdapterAccessException exception in attempts to use
 * referenced const object in non-const context
 *
 * @see SharedNotebookAdapterAccessException
 */
class QUENTIER_EXPORT SharedNotebookAdapter: public ISharedNotebook
{
public:
    SharedNotebookAdapter(qevercloud::SharedNotebook & sharedNotebook);
    SharedNotebookAdapter(const qevercloud::SharedNotebook & sharedNotebook);
    SharedNotebookAdapter(const SharedNotebookAdapter & other);
    SharedNotebookAdapter(SharedNotebookAdapter && other);
    SharedNotebookAdapter & operator=(const SharedNotebookAdapter & other);
    SharedNotebookAdapter & operator=(SharedNotebookAdapter && other);

    virtual ~SharedNotebookAdapter();

    virtual const qevercloud::SharedNotebook & GetEnSharedNotebook() const Q_DECL_OVERRIDE;
    virtual qevercloud::SharedNotebook & GetEnSharedNotebook() Q_DECL_OVERRIDE;

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    SharedNotebookAdapter() Q_DECL_DELETE;

    qevercloud::SharedNotebook * m_pEnSharedNotebook;
    bool m_isConst;
};

} // namespace quentier

#endif // LIB_QUENTIER_TYPES_SHARED_NOTEBOOK_ADAPTER_H
