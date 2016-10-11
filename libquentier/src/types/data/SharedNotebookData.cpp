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

#include "SharedNotebookData.h"

namespace quentier {

SharedNotebookData::SharedNotebookData() :
    QSharedData(),
    m_qecSharedNotebook(),
    m_indexInNotebook(-1)
{}

SharedNotebookData::SharedNotebookData(const SharedNotebookData & other) :
    QSharedData(other),
    m_qecSharedNotebook(other.m_qecSharedNotebook),
    m_indexInNotebook(other.m_indexInNotebook)
{}

SharedNotebookData::SharedNotebookData(SharedNotebookData && other) :
    QSharedData(std::move(other)),
    m_qecSharedNotebook(std::move(other.m_qecSharedNotebook)),
    m_indexInNotebook(std::move(other.m_indexInNotebook))
{}

SharedNotebookData::SharedNotebookData(const qevercloud::SharedNotebook & other) :
    QSharedData(),
    m_qecSharedNotebook(other),
    m_indexInNotebook(-1)
{}

SharedNotebookData::SharedNotebookData(qevercloud::SharedNotebook && other) :
    QSharedData(),
    m_qecSharedNotebook(std::move(other)),
    m_indexInNotebook(-1)
{}

SharedNotebookData::~SharedNotebookData()
{}

} // namespace quentier
