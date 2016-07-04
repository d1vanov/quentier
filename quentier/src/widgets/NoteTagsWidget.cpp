#include "NoteTagsWidget.h"
#include "FlowLayout.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

NoteTagsWidget::NoteTagsWidget(QWidget * parent) :
    QWidget(parent),
    m_currentNote(),
    m_currentNotebookLocalUid(),
    m_currentNoteTagLocalUidToNameBimap(),
    m_pTagModel(),
    m_findNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_findNotebookRequestIds(),
    m_tagRestrictions(),
    m_pLayout(new FlowLayout(this))
{}

void NoteTagsWidget::setCurrentNote(const Note & note)
{
    clear();

    if (Q_UNLIKELY(note.localUid().isEmpty())) {
        QNWARNING("Skipping the note with empty local uid");
        return;
    }

    if (Q_UNLIKELY(!note.hasNotebookGuid() && !note.hasNotebookLocalUid())) {
        QNWARNING("Skipping the note which has neither notebook local uid not notebook guid");
        return;
    }

    m_currentNote = note;

    if (!m_currentNote.hasTagLocalUids() && !m_currentNote.hasTagGuids()) {
        clearLayout();
        return;
    }

    // TODO: find the corresponding notebook
}

void NoteTagsWidget::clear()
{
    m_currentNote.clear();
    m_currentNote.setLocalUid(QString());

    m_currentNotebookLocalUid.clear();
    m_currentNoteTagLocalUidToNameBimap.clear();
    m_findNoteRequestIds.clear();
    m_updateNoteRequestIds.clear();
    m_findNotebookRequestIds.clear();
    m_tagRestrictions = Restrictions();
}

void NoteTagsWidget::onUpdateNoteComplete(Note note, bool updateResources,
                                          bool updateTags, QUuid requestId)
{
    Q_UNUSED(requestId)
    Q_UNUSED(updateResources)

    if (!updateTags) {
        return;
    }

    if (note.localUid() != m_currentNote.localUid()) {
        return;
    }

    QStringList tagLocalUids;
    if (note.hasTagLocalUids()) {
        note.tagLocalUids(tagLocalUids);
    }

    QNDEBUG("NoteTagsWidget::onUpdateNoteComplete: note local uid = " << note.localUid()
            << ", note tag local uids: " << tagLocalUids);

    // TODO: continue implementing the method
}

void NoteTagsWidget::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                        QString errorDescription, QUuid requestId)
{
    auto it = m_updateNoteRequestIds.find(requestId);
    if (it == m_updateNoteRequestIds.end()) {
        return;
    }

    QNDEBUG("NoteTagsWidget::onUpdateNoteFailed: note local uid = " << note.localUid()
            << ", error description: " << errorDescription << ", request id = " << requestId);

    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    // TODO: continue implementing the method
}

void NoteTagsWidget::onFindNoteComplete(Note note, bool withResourceBinaryData, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(requestId)
}

void NoteTagsWidget::onFindNoteFailed(Note note, bool withResourceBinaryData,
                                      QString errorDescription, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(withResourceBinaryData)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteTagsWidget::onExpungeNoteComplete(Note note, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(requestId)
}

void NoteTagsWidget::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NoteTagsWidget::onFindNotebookFailed(Notebook notebook, QString errorDescription,
                                          QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(errorDescription)
    Q_UNUSED(requestId)
}

void NoteTagsWidget::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NoteTagsWidget::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(notebook)
    Q_UNUSED(requestId)
}

void NoteTagsWidget::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
}

void NoteTagsWidget::onExpungeTagComplete(Tag tag, QUuid requestId)
{
    // TODO: implement
    Q_UNUSED(tag)
    Q_UNUSED(requestId)
}

void NoteTagsWidget::clearLayout()
{
    // TODO: implement
}

bool NoteTagsWidget::isActive() const
{
    return !m_currentNote.localUid().isEmpty();
}

} // namespace quentier
