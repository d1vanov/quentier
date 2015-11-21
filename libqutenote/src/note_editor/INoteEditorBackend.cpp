#include <qute_note/note_editor/INoteEditorBackend.h>
#include <qute_note/note_editor/NoteEditor.h>
#include <QUndoStack>

namespace qute_note {

INoteEditorBackend::~INoteEditorBackend()
{}

INoteEditorBackend::INoteEditorBackend(NoteEditor * parent) :
    m_pNoteEditor(parent)
{}

} // namespace qute_note
