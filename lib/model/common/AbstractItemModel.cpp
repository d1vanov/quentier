/*
 * Copyright 2016-2019 Dmitry Ivanov
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

AbstractItemModel::AbstractItemModel(
    const Account & account, QObject * parent) :
    QAbstractItemModel(parent),
    m_account(account)
{}

AbstractItemModel::~AbstractItemModel() {}

QDebug & operator<<(
    QDebug & dbg, const AbstractItemModel::LinkedNotebookInfo & info)
{
    dbg << "Linked notebook guid = " << info.m_guid
        << ", username = " << info.m_username;

    return dbg;
}

QDebug & operator<<(QDebug & dbg, const AbstractItemModel::ItemInfo & itemInfo)
{
    dbg << "Item info: local uid = " << itemInfo.m_localUid
        << ", name = " << itemInfo.m_name
        << ", linked notebook guid = " << itemInfo.m_linkedNotebookGuid
        << ", linked notebook username = " << itemInfo.m_linkedNotebookUsername;

    return dbg;
}

} // namespace quentier
