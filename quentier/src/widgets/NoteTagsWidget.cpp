#include "NoteTagsWidget.h"
#include "NoteTagWidget.h"
#include "FlowLayout.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

NoteTagsWidget::NoteTagsWidget(QWidget * parent) :
    QWidget(parent),
    m_currentNote(),
    m_currentNotebookLocalUid(),
    m_lastDisplayedTagLocalUids(),
    m_currentNoteTagLocalUidToNameBimap(),
    m_pTagModel(),
    m_findNoteRequestIds(),
    m_updateNoteRequestIds(),
    m_findNotebookRequestIds(),
    m_tagRestrictions(),
    m_pLayout(new FlowLayout(this))
{
    addTagIconToLayout();
    setLayout(m_pLayout);
}

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

    if (!m_currentNote.hasTagLocalUids()) {
        clearLayout();
        return;
    }

    // TODO: find the corresponding notebook
}

void NoteTagsWidget::clear()
{
    m_currentNote.clear();
    m_currentNote.setLocalUid(QString());
    m_lastDisplayedTagLocalUids.clear();

    m_currentNotebookLocalUid.clear();
    m_currentNoteTagLocalUidToNameBimap.clear();
    m_findNoteRequestIds.clear();
    m_updateNoteRequestIds.clear();
    m_findNotebookRequestIds.clear();
    m_tagRestrictions = Restrictions();
}

void NoteTagsWidget::onTagRemoved(const QString & tagName)
{
    // TODO: implement
    Q_UNUSED(tagName)
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
        tagLocalUids = note.tagLocalUids();
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

void NoteTagsWidget::clearLayout(const bool skipNewTagWidget)
{
    while(m_pLayout->count() > 0) {
        QLayoutItem * pItem = m_pLayout->takeAt(0);
        delete pItem->widget();
        delete pItem;
    }

    m_lastDisplayedTagLocalUids.clear();
    m_currentNoteTagLocalUidToNameBimap.clear();

    addTagIconToLayout();

    if (skipNewTagWidget || m_currentNote.localUid().isEmpty() ||
        m_currentNotebookLocalUid.isEmpty() || !m_tagRestrictions.m_canCreateTags)
    {
        return;
    }

    addNewTagWidgetToLayout();
}

void NoteTagsWidget::updateLayout()
{
    QNDEBUG("NoteTagsWidget::updateLayout");

    const QString & noteLocalUid = m_currentNote.localUid();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        QNTRACE("Note local uid is empty, nothing to do");
        return;
    }

    if (!m_currentNote.hasTagLocalUids())
    {
        if (m_lastDisplayedTagLocalUids.isEmpty()) {
            QNTRACE("Note tags are still empty, nothing to do");
        }
        else {
            QNTRACE("The last tag has been removed from the note");
            clearLayout();
        }

        return;
    }

    bool shouldUpdateLayout = false;

    const QStringList & tagLocalUids = m_currentNote.tagLocalUids();
    int numTags = tagLocalUids.size();
    if (numTags == m_lastDisplayedTagLocalUids.size())
    {
        for(int i = 0, size = tagLocalUids.size(); i < size; ++i)
        {
            int index = m_lastDisplayedTagLocalUids.indexOf(tagLocalUids[i]);
            if (index < 0) {
                shouldUpdateLayout = true;
                break;
            }
        }
    }

    if (!shouldUpdateLayout) {
        QNTRACE("Note's tag local uids haven't changed, no need to update the layout");
        return;
    }

    clearLayout(/* skip new tag widget = */ true);

    m_lastDisplayedTagLocalUids.reserve(numTags);

    QStringList tagNames;
    tagNames.reserve(numTags);

    for(int i = 0; i < numTags; ++i)
    {
        const QString & tagLocalUid = tagLocalUids[i];

        const TagModelItem * pTagItem = m_pTagModel->itemForLocalUid(tagLocalUid);
        if (Q_UNLIKELY(!pTagItem)) {
            QNWARNING("Can't find tag model item for tag with local uid " << tagLocalUid);
            continue;
        }

        m_lastDisplayedTagLocalUids << tagLocalUid;

        const QString & tagName = pTagItem->name();
        if (Q_UNLIKELY(tagName.isEmpty())) {
            QNDEBUG("Skipping the tag with empty name, local uid = " << tagLocalUid);
            continue;
        }

        m_currentNoteTagLocalUidToNameBimap.insert(TagLocalUidToNameBimap::value_type(tagLocalUid, tagName));
        tagNames << tagName;
    }

    tagNames.sort(Qt::CaseSensitive);
    for(int i = 0, size = tagNames.size(); i < size; ++i)
    {
        const QString & tagName = tagNames[i];

        NoteTagWidget * pTagWidget = new NoteTagWidget(tagName, this);
        QObject::connect(pTagWidget, QNSIGNAL(NoteTagWidget,removeTagFromNote,const QString&),
                         this, QNSLOT(NoteTagsWidget,onTagRemoved,const QString&));

        m_pLayout->addWidget(pTagWidget);
    }

    addNewTagWidgetToLayout();
}

void NoteTagsWidget::addTagIconToLayout()
{
    // TODO: implement
}

void NoteTagsWidget::addNewTagWidgetToLayout()
{
    // TODO: implement
}

bool NoteTagsWidget::isActive() const
{
    return !m_currentNote.localUid().isEmpty();
}

} // namespace quentier
