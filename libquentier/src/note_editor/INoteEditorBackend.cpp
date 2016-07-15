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

#include <quentier/note_editor/INoteEditorBackend.h>
#include <quentier/note_editor/NoteEditor.h>
#include <quentier/logging/QuentierLogger.h>
#include <QUndoStack>
#include <iostream>

namespace quentier {

INoteEditorBackend::~INoteEditorBackend()
{}

INoteEditorBackend::INoteEditorBackend(NoteEditor * parent) :
    m_pNoteEditor(parent)
{}

std::ostream & operator<<(const INoteEditorBackend::Rotation::type rotationDirection, std::ostream & strm)
{
    switch(rotationDirection)
    {
    case INoteEditorBackend::Rotation::Clockwise:
        strm << "Clockwise";
        break;
    case INoteEditorBackend::Rotation::Counterclockwise:
        strm << "Counterclockwise";
        break;
    default:
        strm << "Unknown";
        break;
    }

    return strm;
}

} // namespace quentier
