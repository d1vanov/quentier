/*
 * Copyright 2016 Dmitry Ivanov
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

#include "NoteTagsWidget.h"
#include "NoteTagWidget.h"
#include "NewTagLineEditor.h"
#include "FlowLayout.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QLabel>

namespace quentier {

NoteTagsWidget::NoteTagsWidget(QWidget * parent) :
    QWidget(parent),
    m_currentNote(),
    m_currentNotebookLocalUid(),
    m_lastDisplayedTagLocalUids(),
    m_currentNoteTagLocalUidToNameBimap(),
    m_pTagModel(),
    m_updateNoteRequestIdToRemovedTagLocalUidAndGuid(),
    m_tagRestrictions(),
    m_pLayout(new FlowLayout(this))
{
    addTagIconToLayout();
    setLayout(m_pLayout);
}

void NoteTagsWidget::setLocalStorageManagerThreadWorker(LocalStorageManagerThreadWorker & localStorageWorker)
{
    createConnections(localStorageWorker);
}

void NoteTagsWidget::setTagModel(TagModel * pTagModel)
{
    m_pTagModel = QPointer<TagModel>(pTagModel);
}

void NoteTagsWidget::setCurrentNoteAndNotebook(const Note & note, const Notebook & notebook)
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::setCurrentNoteAndNotebook: note local uid = ") << note
            << QStringLiteral(", notebook: ") << notebook);

    bool changed = (note.localUid() != m_currentNote.localUid());
    if (!changed)
    {
        QNDEBUG(QStringLiteral("The note is the same as the current one already, checking whether the tag information has changed"));

        changed |= (note.hasTagLocalUids() != m_currentNote.hasTagLocalUids());
        changed |= (note.hasTagGuids() != m_currentNote.hasTagGuids());

        if (note.hasTagLocalUids() && m_currentNote.hasTagLocalUids()) {
            changed |= (note.tagLocalUids() != m_currentNote.tagLocalUids());
        }

        if (note.hasTagGuids() && m_currentNote.hasTagGuids()) {
            changed |= (note.tagGuids() != m_currentNote.tagGuids());
        }

        if (!changed) {
            QNDEBUG(QStringLiteral("Tag info hasn't changed"));
        }
        else {
            clear();
        }

        m_currentNote = note;   // Accepting the update just in case
    }
    else
    {
        clear();

        if (Q_UNLIKELY(note.localUid().isEmpty())) {
            QNWARNING(QStringLiteral("Skipping the note with empty local uid"));
            return;
        }

        m_currentNote = note;
    }

    changed |= (m_currentNotebookLocalUid != notebook.localUid());

    m_currentNotebookLocalUid = notebook.localUid();

    bool couldUpdateNote = m_tagRestrictions.m_canUpdateNote;
    bool couldUpdateTags = m_tagRestrictions.m_canUpdateTags;

    if (notebook.hasRestrictions()) {
        const qevercloud::NotebookRestrictions & restrictions = notebook.restrictions();
        m_tagRestrictions.m_canUpdateNote = !(restrictions.noCreateTags.isSet() && restrictions.noCreateTags.ref());
        m_tagRestrictions.m_canUpdateTags = !(restrictions.noUpdateTags.isSet() && restrictions.noUpdateTags.ref());
    }
    else {
        m_tagRestrictions.m_canUpdateNote = true;
        m_tagRestrictions.m_canUpdateTags = true;
    }

    changed |= (couldUpdateNote != m_tagRestrictions.m_canUpdateNote);
    changed |= (couldUpdateTags != m_tagRestrictions.m_canUpdateTags);

    if (changed) {
        updateLayout();
    }
}

void NoteTagsWidget::clear()
{
    m_currentNote.clear();
    m_currentNote.setLocalUid(QString());
    m_lastDisplayedTagLocalUids.clear();

    m_currentNotebookLocalUid.clear();
    m_currentNoteTagLocalUidToNameBimap.clear();
    m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.clear();
    m_tagRestrictions = Restrictions();
}

void NoteTagsWidget::onTagRemoved(QString tagName)
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::onTagRemoved: tag name = ") << tagName);

    if (Q_UNLIKELY(m_currentNote.localUid().isEmpty())) {
        QNTRACE(QStringLiteral("No current note is set, ignoring the tag removal event"));
        return;
    }

    auto localUidIt = m_currentNoteTagLocalUidToNameBimap.right.find(tagName);
    if (Q_UNLIKELY(localUidIt == m_currentNoteTagLocalUidToNameBimap.right.end())) {
        QNLocalizedString errorDescription = QT_TR_NOOP("can't determine the tag which has been removed from the note");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QString tagLocalUid = localUidIt->second;

    const TagModelItem * pItem = m_pTagModel->itemForLocalUid(tagLocalUid);
    if (Q_UNLIKELY(!pItem)) {
        QNLocalizedString errorDescription = QT_TR_NOOP("can't find the tag item attempted to be removed from the note");
        QNWARNING(errorDescription << QStringLiteral(", tag local uid = ") << tagLocalUid);
        emit notifyError(errorDescription);
        return;
    }

    m_currentNote.removeTagLocalUid(tagLocalUid);

    const QString & tagGuid = pItem->guid();
    if (!tagGuid.isEmpty()) {
        m_currentNote.removeTagGuid(tagGuid);
    }

    NoteTagWidget * pNoteTagWidget = qobject_cast<NoteTagWidget*>(sender());
    if (Q_LIKELY(pNoteTagWidget))
    {
        m_pLayout->removeWidget(pNoteTagWidget);

        if (m_tagRestrictions.m_canUpdateNote &&
            ((m_lastDisplayedTagLocalUids.size() - 1) < m_pTagModel->account().noteTagCountMax()))
        {
            NewTagLineEditor * pNewTagLineEditorWidget = m_pLayout->findChild<NewTagLineEditor*>();
            if (!pNewTagLineEditorWidget) {
                addNewTagWidgetToLayout();
            }
        }
    }

    m_lastDisplayedTagLocalUids.removeOne(tagLocalUid);
    Q_UNUSED(m_currentNoteTagLocalUidToNameBimap.right.erase(localUidIt))

    QUuid updateNoteRequestId = QUuid::createUuid();
    m_updateNoteRequestIdToRemovedTagLocalUidAndGuid[updateNoteRequestId] = std::pair<QString, QString>(tagLocalUid, tagGuid);

    QNTRACE(QStringLiteral("Emitting the request to update note in local storage: request id = ") << updateNoteRequestId
            << QStringLiteral(", note = ") << m_currentNote);
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

    QNDEBUG(QStringLiteral("NoteTagsWidget::onUpdateNoteComplete: note local uid = ") << note.localUid()
            << QStringLiteral(", request id = ") << requestId);

    if (updateResources) {
        m_currentNote = note;
    }
    else {
        QList<Resource> resources = m_currentNote.resources();
        m_currentNote = note;
        m_currentNote.setResources(resources);
    }

    updateLayout();
}

void NoteTagsWidget::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                        QNLocalizedString errorDescription, QUuid requestId)
{
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    auto it = m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.find(requestId);
    if (it == m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteTagsWidget::onUpdateNoteFailed: note local uid = ") << note.localUid()
            << QStringLiteral(", error description: ") << errorDescription << QStringLiteral(", request id = ")
            << requestId);

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

    QNDEBUG(QStringLiteral("NoteTagsWidget::onExpungeNoteComplete: note local uid = ") << note.localUid()
            << QStringLiteral(", request id = ") << requestId);

    clear();
    clearLayout();
}

void NoteTagsWidget::onUpdateNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    if (notebook.localUid() != m_currentNotebookLocalUid) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteTagsWidget::onUpdateNotebookComplete: notebook = ") << notebook
            << QStringLiteral("\nRequest id = ") << requestId);

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
        QNTRACE(QStringLiteral("No notebook restrictions were changed, nothing to do"));
        return;
    }

    emit canUpdateNoteRestrictionChanged(m_tagRestrictions.m_canUpdateNote);

    if (!m_tagRestrictions.m_canUpdateNote) {
        removeNewTagWidgetFromLayout();
    }
}

void NoteTagsWidget::onExpungeNotebookComplete(Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    if (notebook.localUid() != m_currentNotebookLocalUid) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteTagsWidget::onExpungeNotebookComplete: notebook = ") << notebook
            << QStringLiteral("\nRequest id = ") << requestId);

    clear();
    clearLayout();
}

void NoteTagsWidget::onUpdateTagComplete(Tag tag, QUuid requestId)
{
    Q_UNUSED(requestId)

    int tagIndex = m_lastDisplayedTagLocalUids.indexOf(tag.localUid());
    if (tagIndex < 0) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteTagsWidget::onUpdateTagComplete: tag = ") << tag << QStringLiteral("\nRequest id = ") << requestId);

    QString tagName = (tag.hasName() ? tag.name() : QString());
    m_lastDisplayedTagLocalUids[tagIndex] = tagName;

    auto previousNameIt = m_currentNoteTagLocalUidToNameBimap.left.find(tag.localUid());
    if (Q_UNLIKELY(previousNameIt == m_currentNoteTagLocalUidToNameBimap.left.end())) {
        QNLocalizedString errorDescription = QT_TR_NOOP("detected the update of tag, however, its previous name cannot be found");
        QNWARNING(errorDescription << QStringLiteral(", tag = ") << tag);
        emit notifyError(errorDescription);
        return;
    }

    const QString & previousName = previousNameIt->second;
    if (tag.hasName() && (previousName == tag.name())) {
        QNDEBUG(QStringLiteral("The tag's name hasn't changed, nothing to do"));
        return;
    }

    m_currentNoteTagLocalUidToNameBimap.left.replace_data(previousNameIt, tagName);

    // Need to find the note tag widget responsible for this tag and to change its displayed name
    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        NoteTagWidget * pNoteTagWidget = qobject_cast<NoteTagWidget*>(pItem->widget());
        if (!pNoteTagWidget) {
            continue;
        }

        if (pNoteTagWidget->tagName() != previousName) {
            continue;
        }

        if (!tag.hasName()) {
            QNDEBUG(QStringLiteral("Detected the update of tag not having any name... Strange enough, will just remove that tag's widget"));
            pItem = m_pLayout->takeAt(i);
            delete pItem->widget();
            delete pItem;
        }
        else {
            pNoteTagWidget->setTagName(tag.name());
        }

        break;
    }
}

void NoteTagsWidget::onExpungeTagComplete(Tag tag, QUuid requestId)
{
    Q_UNUSED(requestId)

    int tagIndex = m_lastDisplayedTagLocalUids.indexOf(tag.localUid());
    if (tagIndex < 0) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteTagsWidget::onExpungeTagComplete: tag = ") << tag << QStringLiteral("\nRequest id = ") << requestId);

    m_lastDisplayedTagLocalUids.removeAt(tagIndex);

    QString tagName;

    auto it = m_currentNoteTagLocalUidToNameBimap.left.find(tag.localUid());
    if (Q_UNLIKELY(it == m_currentNoteTagLocalUidToNameBimap.left.end()))
    {
        QNLocalizedString errorDescription = QT_TR_NOOP("detected the expunge of tag, however, its name cannot be found");
        QNWARNING(errorDescription << QStringLiteral(", tag = ") << tag);

        if (!tag.hasName()) {
            emit notifyError(errorDescription);
            return;
        }

        tagName = tag.name();
    }
    else
    {
        tagName = it->second;
    }

    // Need to find the note tag widget responsible for this tag and remove it from the layout
    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        NoteTagWidget * pNoteTagWidget = qobject_cast<NoteTagWidget*>(pItem->widget());
        if (!pNoteTagWidget) {
            continue;
        }

        if (pNoteTagWidget->tagName() != tagName) {
            continue;
        }

        pItem = m_pLayout->takeAt(i);
        delete pItem->widget();
        delete pItem;
        break;
    }
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

    if (m_tagRestrictions.m_canUpdateNote) {
        addNewTagWidgetToLayout();
    }
}

void NoteTagsWidget::updateLayout()
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::updateLayout"));

    const QString & noteLocalUid = m_currentNote.localUid();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        QNTRACE(QStringLiteral("Note local uid is empty, nothing to do"));
        return;
    }

    if (!m_currentNote.hasTagLocalUids())
    {
        if (m_lastDisplayedTagLocalUids.isEmpty()) {
            QNTRACE(QStringLiteral("Note tags are still empty, nothing to do"));
        }
        else {
            QNTRACE(QStringLiteral("The last tag has been removed from the note"));
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
        QNTRACE(QStringLiteral("Note's tag local uids haven't changed, no need to update the layout"));
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
            QNWARNING(QStringLiteral("Can't find tag model item for tag with local uid ") << tagLocalUid);
            continue;
        }

        m_lastDisplayedTagLocalUids << tagLocalUid;

        const QString & tagName = pTagItem->name();
        if (Q_UNLIKELY(tagName.isEmpty())) {
            QNDEBUG(QStringLiteral("Skipping the tag with empty name, local uid = ") << tagLocalUid);
            continue;
        }

        m_currentNoteTagLocalUidToNameBimap.insert(TagLocalUidToNameBimap::value_type(tagLocalUid, tagName));
        tagNames << tagName;
    }

    tagNames.sort();
    for(int i = 0, size = tagNames.size(); i < size; ++i)
    {
        const QString & tagName = tagNames[i];

        NoteTagWidget * pTagWidget = new NoteTagWidget(tagName, this);
        QObject::connect(pTagWidget, QNSIGNAL(NoteTagWidget,removeTagFromNote,QString),
                         this, QNSLOT(NoteTagsWidget,onTagRemoved,QString));
        QObject::connect(this, QNSIGNAL(NoteTagsWidget,canUpdateNoteRestrictionChanged,bool),
                         pTagWidget, QNSLOT(NoteTagWidget,onCanCreateTagRestrictionChanged,bool));

        m_pLayout->addWidget(pTagWidget);
    }

    if (Q_LIKELY((tagNames.size() < m_pTagModel->account().noteTagCountMax()) &&
                 (m_tagRestrictions.m_canUpdateNote)))
    {
        addNewTagWidgetToLayout();
    }
}

void NoteTagsWidget::addTagIconToLayout()
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::addTagIconToLayout"));

    QPixmap tagIconImage(QStringLiteral("/label/tag.png"));
    QLabel * pTagIconLabel = new QLabel(this);
    pTagIconLabel->setPixmap(tagIconImage);
    m_pLayout->addWidget(pTagIconLabel);
}

void NoteTagsWidget::addNewTagWidgetToLayout()
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::addNewTagWidgetToLayout"));

    NewTagLineEditor * pNewTagLineEditor = new NewTagLineEditor(m_pTagModel, this);
    m_pLayout->addWidget(pNewTagLineEditor);
}

void NoteTagsWidget::removeNewTagWidgetFromLayout()
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::removeNewTagWidgetFromLayout"));

    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        NewTagLineEditor * pNewTagLineEditor = qobject_cast<NewTagLineEditor*>(pItem->widget());
        if (!pNewTagLineEditor) {
            continue;
        }

        pItem = m_pLayout->takeAt(i);
        delete pItem->widget();
        delete pItem;
        break;
    }
}

void NoteTagsWidget::createConnections(LocalStorageManagerThreadWorker & localStorageWorker)
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::createConnections"));

    // Connect local storage signals to local slots
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NoteTagsWidget,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid),
                     this, QNSLOT(NoteTagsWidget,onUpdateNoteFailed,Note,bool,bool,QNLocalizedString,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteTagsWidget,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteTagsWidget,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteTagsWidget,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteTagsWidget,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,expungeTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteTagsWidget,onExpungeTagComplete,Tag,QUuid));

    // Connect local signals to local storage slots
    QObject::connect(this, QNSIGNAL(NoteTagsWidget,updateNote,Note,bool,bool,QUuid),
                     &localStorageWorker, QNSLOT(LocalStorageManagerThreadWorker,onUpdateNoteRequest,Note,bool,bool,QUuid));
}

bool NoteTagsWidget::isActive() const
{
    return !m_currentNote.localUid().isEmpty();
}

} // namespace quentier
