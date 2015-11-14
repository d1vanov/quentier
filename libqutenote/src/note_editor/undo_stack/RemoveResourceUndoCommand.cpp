#include "RemoveResourceUndoCommand.h"
#include "RemoveResourceUndoCommandNotReadyException.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

RemoveResourceUndoCommand::RemoveResourceUndoCommand(const ResourceWrapper & resource, NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, parent),
    m_resource(resource),
    m_htmlBefore(),
    m_htmlAfter(),
    m_htmlBeforeSet(false),
    m_htmlAfterSet(false)
{
    m_ready = false;
    init();
}

RemoveResourceUndoCommand::RemoveResourceUndoCommand(const ResourceWrapper & resource, NoteEditorPrivate & noteEditorPrivate,
                                               const QString & text, QUndoCommand * parent) :
    INoteEditorUndoCommand(noteEditorPrivate, text, parent),
    m_resource(resource),
    m_htmlBefore(),
    m_htmlAfter(),
    m_htmlBeforeSet(false),
    m_htmlAfterSet(false)
{
    m_ready = false;
    init();
}

RemoveResourceUndoCommand::~RemoveResourceUndoCommand()
{}

void RemoveResourceUndoCommand::setHtmlBefore(const QString & htmlBefore)
{
    m_htmlBefore = htmlBefore;
    m_htmlBeforeSet = true;

    if (m_htmlBeforeSet && m_htmlAfterSet) {
        m_ready = true;
    }
}

void RemoveResourceUndoCommand::setHtmlAfter(const QString & htmlAfter)
{
    m_htmlAfter = htmlAfter;
    m_htmlAfterSet = true;

    if (m_htmlBeforeSet && m_htmlAfterSet) {
        m_ready = true;
    }
}

void RemoveResourceUndoCommand::undo()
{
    QNDEBUG("RemoveResourceUndoCommand::undo");

    if (Q_UNLIKELY(!m_ready)) {
        throw RemoveResourceUndoCommandNotReadyException();
    }

    m_noteEditorPrivate.setNoteHtml(m_htmlBefore);
    m_noteEditorPrivate.addResourceToNote(m_resource);
}

void RemoveResourceUndoCommand::redo()
{
    QNDEBUG("RemoveResourceUndoCommand::redo");

    if (Q_UNLIKELY(!m_ready)) {
        throw RemoveResourceUndoCommandNotReadyException();
    }

    m_noteEditorPrivate.setNoteHtml(m_htmlAfter);
    m_noteEditorPrivate.removeResourceFromNote(m_resource);
}

void RemoveResourceUndoCommand::init()
{
    QUndoCommand::setText(QObject::tr("Remove attachment"));
}

} // namespace qute_note
