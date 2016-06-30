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
