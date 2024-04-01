/*
 * Copyright 2019-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/local_storage/Fwd.h>

#include <qevercloud/types/Notebook.h>
#include <qevercloud/types/Tag.h>

#include <QList>

namespace quentier {

[[nodiscard]] bool fetchNotes(
    const QList<qevercloud::Notebook> & notebooks,
    const QList<qevercloud::Tag> & tags, quint32 minTagsPerNote,
    quint32 numNotes, local_storage::ILocalStoragePtr localStorage);

} // namespace quentier
