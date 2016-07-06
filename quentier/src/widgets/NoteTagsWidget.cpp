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
    m_updateNoteRequestIdToRemovedTagLocalUidAndGuid(),
    m_findNotebookRequestId(),
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

    if (note.hasNotebookLocalUid()) {
        m_currentNotebookLocalUid = note.notebookLocalUid();
    }
    else {
        m_currentNotebookLocalUid = QString();
    }

    Notebook dummy;
    dummy.setLocalUid(m_currentNotebookLocalUid);
    if (note.hasNotebookGuid()) {
        dummy.setGuid(note.notebookGuid());
    }

    m_findNotebookRequestId = QUuid::createUuid();
    QNTRACE("Emitting the request to find notebook: local uid = " << dummy.localUid()
            << ", guid = " << (dummy.hasGuid() ? dummy.guid() : QStringLiteral("<empty>"))
            << ", request id = " << m_findNotebookRequestId);
    emit findNotebook(dummy, m_findNotebookRequestId);
}

void NoteTagsWidget::clear()
{
    m_currentNote.clear();
    m_currentNote.setLocalUid(QString());
    m_lastDisplayedTagLocalUids.clear();

    m_currentNotebookLocalUid.clear();
    m_currentNoteTagLocalUidToNameBimap.clear();
    m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.clear();
    m_findNotebookRequestId = QUuid();
    m_tagRestrictions = Restrictions();
}

void NoteTagsWidget::onTagRemoved(QString tagName)
{
    QNDEBUG("NoteTagsWidget::onTagRemoved: tag name = " << tagName);

    if (Q_UNLIKELY(m_currentNote.localUid().isEmpty())) {
        QNTRACE("No current note is set, ignoring the tag removal event");
        return;
    }

    auto localUidIt = m_currentNoteTagLocalUidToNameBimap.right.find(tagName);
    if (Q_UNLIKELY(localUidIt == m_currentNoteTagLocalUidToNameBimap.right.end())) {
        const char * errorDescription = QT_TR_NOOP("Can't determine the tag which has been removed from the note");
        QNWARNING(errorDescription);
        emit notifyError(tr(errorDescription));
        return;
    }

    QString tagLocalUid = localUidIt->second;

    const TagModelItem * pItem = m_pTagModel->itemForLocalUid(tagLocalUid);
    if (Q_UNLIKELY(!pItem)) {
        const char * errorDescription = QT_TR_NOOP("Can't find the tag item attempted to be removed from the note");
        QNWARNING(errorDescription << ", tag local uid = " << tagLocalUid);
        emit notifyError(tr(errorDescription));
        return;
    }

    m_currentNote.removeTagLocalUid(tagLocalUid);

    const QString & tagGuid = pItem->guid();
    if (!tagGuid.isEmpty()) {
        m_currentNote.removeTagGuid(tagGuid);
    }

    NoteTagWidget * pNoteTagWidget = qobject_cast<NoteTagWidget*>(sender());
    if (Q_LIKELY(pNoteTagWidget)) {
        m_pLayout->removeWidget(pNoteTagWidget);
    }

    m_lastDisplayedTagLocalUids.removeOne(tagLocalUid);
    Q_UNUSED(m_currentNoteTagLocalUidToNameBimap.right.erase(localUidIt))

    QUuid updateNoteRequestId = QUuid::createUuid();
    m_updateNoteRequestIdToRemovedTagLocalUidAndGuid[updateNoteRequestId] = std::pair<QString, QString>(tagLocalUid, tagGuid);

    QNTRACE("Emitting the request to update note in local storage: request id = " << updateNoteRequestId
            << ", note = " << m_currentNote);
    emit updateNote(m_currentNote, /* update resources = */ false, /* update tags = */ true, updateNoteRequestId);
}

void NoteTagsWidget::onUpdateNoteComplete(Note note, bool updateResources,
                                          bool updateTags, QUuid requestId)
{
    Q_UNUSED(requestId)

    if (!updateTags) {
        return;
    }

    if (note.localUid() != m_currentNote.localUid()) {
        return;
    }

    QNDEBUG("NoteTagsWidget::onUpdateNoteComplete: note local uid = " << note.localUid()
            << ", request id = " << requestId);

    if (updateResources) {
        m_currentNote = note;
    }
    else {
        QList<ResourceWrapper> resources = m_currentNote.resources();
        m_currentNote = note;
        m_currentNote.setResources(resources);
    }

    updateLayout();
}

void NoteTagsWidget::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                        QString errorDescription, QUuid requestId)
{
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    auto it = m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.find(requestId);
    if (it == m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.end()) {
        return;
    }

    QNDEBUG("NoteTagsWidget::onUpdateNoteFailed: note local uid = " << note.localUid()
            << ", error description: " << errorDescription << ", request id = " << requestId);

    const auto & idsPair = it.value();

    m_currentNote.addTagLocalUid(idsPair.first);
    if (!idsPair.second.isEmpty()) {
        m_currentNote.addTagGuid(idsPair.second);
    }

    Q_UNUSED(m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.erase(it))
    updateLayout();
}

void NoteTagsWidget::onExpungeNoteComplete(Note note, QUuid requestId)
{
    if (note.localUid() != m_currentNote.localUid()) {
        return;
    }

    QNDEBUG("NoteTagsWidget::onExpungeNoteComplete: note local uid = " << note.localUid()
            << ", request id = " << requestId);

    clear();
    clearLayout();
}

void NoteTagsWidget::onFindNotebookComplete(Notebook notebook, QUuid requestId)
{
    if (requestId != m_findNotebookRequestId) {
        return;
    }

    QNDEBUG("NoteTagsWidget::onFindNotebookComplete: notebook = " << notebook << "\nRequest id = " << requestId);

    m_currentNotebookLocalUid = notebook.localUid();

    if (notebook.hasRestrictions()) {
        const qevercloud::NotebookRestrictions & restrictions = notebook.restrictions();
        m_tagRestrictions.m_canUpdateNote = !(restrictions.noCreateTags.isSet() && restrictions.noCreateTags.ref());
        m_tagRestrictions.m_canUpdateTags = !(restrictions.noUpdateTags.isSet() && restrictions.noUpdateTags.ref());
    }
    else {
        m_tagRestrictions.m_canUpdateNote = true;
        m_tagRestrictions.m_canUpdateTags = true;
    }

    updateLayout();
}

void NoteTagsWidget::onFindNotebookFailed(Notebook notebook, QString errorDescription,
                                          QUuid requestId)
{
    if (requestId != m_findNotebookRequestId) {
        return;
    }

    QNDEBUG("NoteTagsWidget::onFindNotebookFailed: notebook = " << notebook
            << "\nError description = " << errorDescription
            << ", request id = " << requestId);

    clearLayout();
    emit notifyError(errorDescription);
}

void NoteTagsWidget::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    if (notebook.localUid() != m_currentNotebookLocalUid) {
        return;
    }

    QNDEBUG("NoteTagsWidget::onUpdateNotebookComplete: notebook = " << notebook
            << "\nRequest id = " << requestId);

    bool changed = false;
    bool canUpdateNote = false;
    bool canUpdateTags = false;

    if (notebook.hasRestrictions()) {
        const qevercloud::NotebookRestrictions & restrictions = notebook.restrictions();
        canUpdateNote = !(restrictions.noCreateTags.isSet() && restrictions.noCreateTags.ref());
        canUpdateTags = !(restrictions.noUpdateTags.isSet() && restrictions.noUpdateTags.ref());
    }
    else {
        canUpdateNote = true;
        canUpdateTags = true;
    }

    changed |= (canUpdateNote != m_tagRestrictions.m_canUpdateNote);
    changed |= (canUpdateTags != m_tagRestrictions.m_canUpdateTags);
    m_tagRestrictions.m_canUpdateNote = canUpdateNote;
    m_tagRestrictions.m_canUpdateTags = canUpdateTags;

    if (!changed) {
        QNTRACE("No notebook restrictions were changed, nothing to do");
        return;
    }

    emit canUpdateNoteRestrictionChanged(m_tagRestrictions.m_canUpdateNote);

    if (!m_tagRestrictions.m_canUpdateNote) {
        // TODO: remove the add new tag widget from the layout
    }
}

void NoteTagsWidget::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    if (notebook.localUid() != m_currentNotebookLocalUid) {
        return;
    }

    QNDEBUG("NoteTagsWidget::onExpungeNotebookComplete: notebook = " << notebook
            << "\nRequest id = " << requestId);

    clear();
    clearLayout();
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
        m_currentNotebookLocalUid.isEmpty() || !m_tagRestrictions.m_canUpdateNote)
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
        QObject::connect(pTagWidget, QNSIGNAL(NoteTagWidget,removeTagFromNote,QString),
                         this, QNSLOT(NoteTagsWidget,onTagRemoved,QString));

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
