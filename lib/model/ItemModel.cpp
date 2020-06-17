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

#include "ItemModel.h"

#include <QDebug>

namespace quentier {

ItemModel::ItemModel(QObject * parent) :
    QAbstractItemModel(parent)
{}

ItemModel::~ItemModel()
{}

QDebug & operator<<(QDebug & dbg, const ItemModel::LinkedNotebookInfo & info)
{
    dbg << "Linked notebook guid = " << info.m_guid
        << ", username = " << info.m_username;

    return dbg;
}

} // namespace quentier
