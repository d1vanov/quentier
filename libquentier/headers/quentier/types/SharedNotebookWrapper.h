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

#ifndef LIB_QUENTIER_TYPES_SHARED_NOTEBOOK_WRAPPER_H
#define LIB_QUENTIER_TYPES_SHARED_NOTEBOOK_WRAPPER_H

#include "ISharedNotebook.h"
#include <QEverCloud.h>
#include <QSharedDataPointer>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SharedNotebookWrapperData)

/**
 * @brief The SharedNotebookWrapper class creates and manages its own instance of
 * qevercloud::SharedNotebook object
 */
class QUENTIER_EXPORT SharedNotebookWrapper: public ISharedNotebook
{
public:
    SharedNotebookWrapper();
    SharedNotebookWrapper(const SharedNotebookWrapper & other);
    SharedNotebookWrapper(SharedNotebookWrapper && other);
    SharedNotebookWrapper & operator=(const SharedNotebookWrapper & other);
    SharedNotebookWrapper & operator=(SharedNotebookWrapper && other);

    SharedNotebookWrapper(const qevercloud::SharedNotebook & other);
    SharedNotebookWrapper(qevercloud::SharedNotebook && other);

    virtual ~SharedNotebookWrapper();

    virtual const qevercloud::SharedNotebook & GetEnSharedNotebook() const Q_DECL_OVERRIDE;
    virtual qevercloud::SharedNotebook & GetEnSharedNotebook() Q_DECL_OVERRIDE;

    virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

private:
    QSharedDataPointer<SharedNotebookWrapperData> d;
};

} // namespace quentier

Q_DECLARE_METATYPE(quentier::SharedNotebookWrapper)

#endif // LIB_QUENTIER_TYPES_SHARED_NOTEBOOK_WRAPPER_H
