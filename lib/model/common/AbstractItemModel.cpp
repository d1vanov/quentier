/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include "AbstractItemModel.h"

#include <QDebug>

namespace quentier {

namespace {

template <class T>
void printItemInfo(const AbstractItemModel::ItemInfo & info, T & t)
{
    t << "Item info: local id = " << info.m_localId
      << ", name = " << info.m_name
      << ", linked notebook guid = " << info.m_linkedNotebookGuid
      << ", linked notebook username = " << info.m_linkedNotebookUsername;
}

template <class T>
void printLinkedNotebookInfo(
    const AbstractItemModel::LinkedNotebookInfo & info, T & t)
{
    t << "Linked notebook guid = " << info.m_guid
      << ", username = " << info.m_username;
}

} // namespace

AbstractItemModel::AbstractItemModel(Account account, QObject * parent) :
    QAbstractItemModel{parent}, m_account{std::move(account)}
{}

AbstractItemModel::~AbstractItemModel() = default;

QDebug & operator<<(QDebug & dbg, const AbstractItemModel::ItemInfo & info)
{
    printItemInfo(info, dbg);
    return dbg;
}

QTextStream & operator<<(
    QTextStream & strm, const AbstractItemModel::ItemInfo & info)
{
    printItemInfo(info, strm);
    return strm;
}

QDebug & operator<<(
    QDebug & dbg, const AbstractItemModel::LinkedNotebookInfo & info)
{
    printLinkedNotebookInfo(info, dbg);
    return dbg;
}

QTextStream & operator<<(
    QTextStream & strm, const AbstractItemModel::LinkedNotebookInfo & info)
{
    printLinkedNotebookInfo(info, strm);
    return strm;
}

} // namespace quentier
