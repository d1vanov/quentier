/*
 * Copyright 2019 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteModel2.h"

namespace quentier {

NoteModel2::NoteModel2(const Account & account,
                       LocalStorageManagerAsync & localStorageManagerAsync,
                       NoteCache & noteCache, NotebookCache & notebookCache,
                       QObject * parent,
                       const IncludedNotes::type includedNotes,
                       const NoteSortingMode::type noteSortingMode) :
    QAbstractItemModel(parent),
    m_account(account),
    m_includedNotes(includedNotes),
    m_noteSortingMode(noteSortingMode),
    m_cache(noteCache),
    m_notebookCache(notebookCache),
    m_filters()
{
    // TODO: implement
}

NoteModel2::~NoteModel2()
{}

int NoteModel2::sortingColumn() const
{
    switch(m_noteSortingMode)
    {
    case NoteSortingMode::CreatedAscending:
    case NoteSortingMode::CreatedDescending:
        return Columns::CreationTimestamp;
    case NoteSortingMode::ModifiedAscending:
    case NoteSortingMode::ModifiedDescending:
        return Columns::ModificationTimestamp;
    case NoteSortingMode::TitleAscending:
    case NoteSortingMode::TitleDescending:
        return Columns::Title;
    case NoteSortingMode::SizeAscending:
    case NoteSortingMode::SizeDescending:
        return Columns::Size;
    default:
        return Columns::ModificationTimestamp;
    }
}

Qt::SortOrder NoteModel2::sortOrder() const
{
    switch(m_noteSortingMode)
    {
    case NoteSortingMode::CreatedDescending:
    case NoteSortingMode::ModifiedDescending:
    case NoteSortingMode::TitleDescending:
    case NoteSortingMode::SizeDescending:
        return Qt::DescendingOrder;
    default:
        return Qt::AscendingOrder;
    }
}

} // namespace quentier
