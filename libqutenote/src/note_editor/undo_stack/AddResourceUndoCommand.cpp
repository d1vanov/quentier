#include "AddResourceUndoCommand.h"
#include "AddResourceUndoCommandNotReadyException.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

AddResourceUndoCommand::AddResourceUndoCommand(const ResourceWrapper & resource, NoteEditorPrivate & noteEditorPrivate, QUndoCommand * parent) :
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

AddResourceUndoCommand::AddResourceUndoCommand(const ResourceWrapper & resource, NoteEditorPrivate & noteEditorPrivate,
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

AddResourceUndoCommand::~AddResourceUndoCommand()
{}

void AddResourceUndoCommand::setHtmlBefore(const QString & htmlBefore)
{
    m_htmlBefore = htmlBefore;
    m_htmlBeforeSet = true;

    if (m_htmlBeforeSet && m_htmlAfterSet) {
        m_ready = true;
    }
}

void AddResourceUndoCommand::setHtmlAfter(const QString & htmlAfter)
{
    m_htmlAfter = htmlAfter;
    m_htmlAfterSet = true;

    if (m_htmlBeforeSet && m_htmlAfterSet) {
        m_ready = true;
    }
}

void AddResourceUndoCommand::undoImpl()
{
    QNDEBUG("AddResourceUndoCommand::undoImpl");

    if (Q_UNLIKELY(!m_ready)) {
        throw AddResourceUndoCommandNotReadyException();
    }

    m_noteEditorPrivate.setNoteHtml(m_htmlBefore);
    m_noteEditorPrivate.removeResourceFromNote(m_resource);
}

void AddResourceUndoCommand::redoImpl()
{
    QNDEBUG("AddResourceUndoCommand::redoImpl");

    if (Q_UNLIKELY(!m_ready)) {
        throw AddResourceUndoCommandNotReadyException();
    }

    m_noteEditorPrivate.setNoteHtml(m_htmlAfter);
    m_noteEditorPrivate.addResourceToNote(m_resource);
}

void AddResourceUndoCommand::init()
{
    QUndoCommand::setText(QObject::tr("Add attachment"));
}

} // namespace qute_note
