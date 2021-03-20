/*
 * Copyright 2021 Dmitry Ivanov
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

#include "ToOptional.h"

namespace quentier {

std::optional<QString> toOptional(QString str)
{
    if (!str.isEmpty()) {
        return std::make_optional(std::move(str));
    }

    return std::nullopt;
}

std::optional<QStringList> toOptional(QStringList lst)
{
    if (!lst.isEmpty()) {
        return std::make_optional(std::move(lst));
    }

    return std::nullopt;
}

std::optional<qevercloud::Timestamp> toOptional(qevercloud::Timestamp timestamp)
{
    if (timestamp != 0) {
        return std::make_optional(timestamp);
    }

    return std::nullopt;
}

} // namespace quentier
