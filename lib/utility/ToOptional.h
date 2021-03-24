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

#ifndef QUENTIER_LIB_UTILITY_TO_OPTIONAL_H
#define QUENTIER_LIB_UTILITY_TO_OPTIONAL_H

#include <qevercloud/types/TypeAliases.h>

#include <QString>
#include <QStringList>

#include <optional>

namespace quentier {

/**
 * @brief Converts non-empty input string into non-nullopt std::optional and
 *        converst empty input string into nullopt std::optional
 */
[[nodiscard]] std::optional<QString> toOptional(QString str);

/**
 * @brief Converts non-empty input string list into non-nullopt std::optional
 *        and converts empty input string list into nullopt std::optional
 */
[[nodiscard]] std::optional<QStringList> toOptional(QStringList lst);

/**
 * @brief Converts non-zero input timestamp into non-nullopt std::optional and
 *        converts zero input timestamp into nullopt std::optional
 */
[[nodiscard]] std::optional<qevercloud::Timestamp> toOptional(
    qevercloud::Timestamp timestamp);

} // namespace quentier

#endif // QUENTIER_LIB_UTILITY_TO_OPTIONAL_H
