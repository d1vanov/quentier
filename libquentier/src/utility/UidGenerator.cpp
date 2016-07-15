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

#include <quentier/utility/UidGenerator.h>

namespace quentier {

QString UidGenerator::Generate()
{
    QUuid uid = QUuid::createUuid();
    return UidToString(uid);
}

QString UidGenerator::UidToString(const QUuid & uid)
{
    if (uid.isNull()) {
        return QString();
    }

    QString result = uid.toString();
    result.remove(result.size()-1, 1);
    result.remove(0, 1);
    return result;
}

} // namespace quentier
