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

#include <quentier/types/SharedNotebookAdapter.h>
#include <quentier/exception/SharedNotebookAdapterAccessException.h>
#include <quentier/utility/QuentierCheckPtr.h>

namespace quentier {

SharedNotebookAdapter::SharedNotebookAdapter(qevercloud::SharedNotebook & sharedNotebook) :
    ISharedNotebook(),
    m_pEnSharedNotebook(&sharedNotebook),
    m_isConst(false)
{}

SharedNotebookAdapter & SharedNotebookAdapter::operator=(const SharedNotebookAdapter & other)
{
    if (this != &other) {
        m_pEnSharedNotebook = other.m_pEnSharedNotebook;
        m_isConst = other.m_isConst;
    }

    return *this;
}

SharedNotebookAdapter & SharedNotebookAdapter::operator=(SharedNotebookAdapter && other)
{
    m_pEnSharedNotebook = std::move(other.m_pEnSharedNotebook);
    m_isConst = std::move(other.m_isConst);
    return *this;
}

SharedNotebookAdapter::SharedNotebookAdapter(const qevercloud::SharedNotebook & sharedNotebook) :
    ISharedNotebook(),
    m_pEnSharedNotebook(const_cast<qevercloud::SharedNotebook*>(&sharedNotebook)),
    m_isConst(true)
{}

SharedNotebookAdapter::SharedNotebookAdapter(const SharedNotebookAdapter & other) :
    ISharedNotebook(other),
    m_pEnSharedNotebook(other.m_pEnSharedNotebook),
    m_isConst(other.m_isConst)
{}

SharedNotebookAdapter::SharedNotebookAdapter(SharedNotebookAdapter && other) :
    ISharedNotebook(std::move(other)),
    m_pEnSharedNotebook(other.m_pEnSharedNotebook),
    m_isConst(other.m_isConst)
{}

SharedNotebookAdapter::~SharedNotebookAdapter()
{}

const qevercloud::SharedNotebook & SharedNotebookAdapter::GetEnSharedNotebook() const
{
    QUENTIER_CHECK_PTR(m_pEnSharedNotebook, "Null pointer to external SharedNotebook in SharedNotebookAdapter");
    return *m_pEnSharedNotebook;
}

qevercloud::SharedNotebook & SharedNotebookAdapter::GetEnSharedNotebook()
{
    if (m_isConst) {
        throw SharedNotebookAdapterAccessException("Attempt to access non-const reference "
                                                   "to SharedNotebook from constant SharedNotebookAdapter");
    }

    QUENTIER_CHECK_PTR(m_pEnSharedNotebook, "Null pointer to external SharedNotebook in SharedNotebookAdapter");
    return *m_pEnSharedNotebook;
}

QTextStream & SharedNotebookAdapter::print(QTextStream & strm) const
{
    strm << QStringLiteral("SharedNotebookAdapter: { \n");
    ISharedNotebook::print(strm);
    strm << QStringLiteral("}; \n");

    return strm;
}

} // namespace quentier
