#include <qute_note/note_editor/INoteEditorBackend.h>
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QUndoStack>

namespace qute_note {

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

} // namespace qute_note
