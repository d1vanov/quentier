/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_MODEL_NOTE_CACHE_H
#define QUENTIER_LIB_MODEL_NOTE_CACHE_H

#include <quentier/types/Note.h>
#include <quentier/utility/LRUCache.hpp>

namespace quentier {

using NoteCache = LRUCache<QString, Note>;

} // namespace quentier

#endif // QUENTIER_LIB_MODEL_NOTE_CACHE_H
