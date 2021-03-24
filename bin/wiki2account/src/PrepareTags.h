/*
 * Copyright 2019-2021 Dmitry Ivanov
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

#ifndef QUENTIER_WIKI2ACCOUNT_PREPARE_TAGS_H
#define QUENTIER_WIKI2ACCOUNT_PREPARE_TAGS_H

#include <qevercloud/types/Tag.h>

namespace quentier {

class ErrorString;
class LocalStorageManagerAsync;

QList<qevercloud::Tag> prepareTags(
    quint32 minTagsPerNote, quint32 maxTagsPerNote,
    LocalStorageManagerAsync & localStorageManagerAsync,
    ErrorString & errorDescription);

} // namespace quentier

#endif // QUENTIER_WIKI2ACCOUNT_PREPARE_TAGS_H
