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

#include "data/SharedNotebookWrapperData.h"
#include <quentier/types/SharedNotebookWrapper.h>

namespace quentier {

SharedNotebookWrapper::SharedNotebookWrapper() :
    ISharedNotebook(),
    d(new SharedNotebookWrapperData)
{}

SharedNotebookWrapper::SharedNotebookWrapper(const SharedNotebookWrapper & other) :
    ISharedNotebook(other),
    d(other.d)
{}

SharedNotebookWrapper::SharedNotebookWrapper(SharedNotebookWrapper && other) :
    ISharedNotebook(),
    d(other.d)
{}

SharedNotebookWrapper::SharedNotebookWrapper(const qevercloud::SharedNotebook & other) :
    ISharedNotebook(),
    d(new SharedNotebookWrapperData(other))
{}

SharedNotebookWrapper::SharedNotebookWrapper(qevercloud::SharedNotebook && other) :
    ISharedNotebook(),
    d(new SharedNotebookWrapperData(std::move(other)))
{}

SharedNotebookWrapper & SharedNotebookWrapper::operator=(const SharedNotebookWrapper & other)
{
    ISharedNotebook::operator=(other);

    if (this != &other) {
        d = other.d;
    }

    return *this;
}

SharedNotebookWrapper & SharedNotebookWrapper::operator=(SharedNotebookWrapper && other)
{
    ISharedNotebook::operator=(std::move(other));

    if (this != &other) {
        d = other.d;
    }

    return *this;
}

SharedNotebookWrapper::~SharedNotebookWrapper()
{}

const qevercloud::SharedNotebook & SharedNotebookWrapper::GetEnSharedNotebook() const
{
    return d->m_qecSharedNotebook;
}

qevercloud::SharedNotebook & SharedNotebookWrapper::GetEnSharedNotebook()
{
    return d->m_qecSharedNotebook;
}

QTextStream & SharedNotebookWrapper::print(QTextStream & strm) const
{
    strm << QStringLiteral("SharedNotebookWrapper: { \n");
    ISharedNotebook::print(strm);
    strm << QStringLiteral("}; \n");

    return strm;
}

} // namespace quentier
