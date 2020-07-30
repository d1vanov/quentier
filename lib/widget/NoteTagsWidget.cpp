/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#include "FlowLayout.h"
#include "ListItemWidget.h"
#include "NewListItemLineEdit.h"

#include <lib/model/tag/TagModel.h>

#include <quentier/logging/QuentierLogger.h>

#include <QApplication>
#include <QKeyEvent>
#include <QLabel>

namespace quentier {

NoteTagsWidget::NoteTagsWidget(QWidget * parent) :
    QWidget(parent),
    m_pLayout(new FlowLayout)
{
    addTagIconToLayout();
    setLayout(m_pLayout);
}

NoteTagsWidget::~NoteTagsWidget() = default;

void NoteTagsWidget::setLocalStorageManagerThreadWorker(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    createConnections(localStorageManagerAsync);
}

void NoteTagsWidget::setTagModel(TagModel * pTagModel)
{
    if (!m_pTagModel.isNull()) {
        QObject::disconnect(
            m_pTagModel.data(),
            &TagModel::notifyAllTagsListed,
            this,
            &NoteTagsWidget::onAllTagsListed);
    }

    m_pTagModel = QPointer<TagModel>(pTagModel);
}

void NoteTagsWidget::setCurrentNoteAndNotebook(
    const Note & note, const Notebook & notebook)
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::setCurrentNoteAndNotebook: "
        << "note local uid = " << note << ", notebook: " << notebook);

    bool changed = (note.localUid() != m_currentNote.localUid());
    if (!changed)
    {
        QNDEBUG("widget:note_tags", "The note is the same as the current one "
            << "already, checking whether the tag information has changed");

        changed |= (note.hasTagLocalUids() != m_currentNote.hasTagLocalUids());
        changed |= (note.hasTagGuids() != m_currentNote.hasTagGuids());

        if (note.hasTagLocalUids() && m_currentNote.hasTagLocalUids()) {
            changed |= (note.tagLocalUids() != m_currentNote.tagLocalUids());
        }

        if (note.hasTagGuids() && m_currentNote.hasTagGuids()) {
            changed |= (note.tagGuids() != m_currentNote.tagGuids());
        }

        if (!changed) {
            QNDEBUG("widget:note_tags", "Tag info hasn't changed");
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
            QNWARNING("widget:note_tags", "Skipping the note with empty local "
                << "uid");
            return;
        }

        m_currentNote = note;
    }

    changed |= (m_currentNotebookLocalUid != notebook.localUid());

    QString linkedNotebookGuid;
    if (notebook.hasLinkedNotebookGuid()) {
        linkedNotebookGuid = notebook.linkedNotebookGuid();
    }

    changed |= (m_currentLinkedNotebookGuid != linkedNotebookGuid);

    m_currentNotebookLocalUid = notebook.localUid();
    m_currentLinkedNotebookGuid = linkedNotebookGuid;

    bool couldUpdateNote = m_tagRestrictions.m_canUpdateNote;
    bool couldUpdateTags = m_tagRestrictions.m_canUpdateTags;

    if (notebook.hasRestrictions())
    {
        const auto & restrictions = notebook.restrictions();

        m_tagRestrictions.m_canUpdateNote =
            !(restrictions.noCreateTags.isSet() &&
              restrictions.noCreateTags.ref());

        m_tagRestrictions.m_canUpdateTags =
            !(restrictions.noUpdateTags.isSet() &&
              restrictions.noUpdateTags.ref());
    }
    else
    {
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
    m_currentLinkedNotebookGuid.clear();
    m_currentNoteTagLocalUidToNameBimap.clear();
    m_tagRestrictions = Restrictions();
}

void NoteTagsWidget::onTagRemoved(QString tagName)
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::onTagRemoved: tag name = "
        << tagName);

    if (Q_UNLIKELY(m_currentNote.localUid().isEmpty())) {
        QNDEBUG("widget:note_tags", "No current note is set, ignoring the tag "
            << "removal event");
        return;
    }

    auto tagNameIt = m_currentNoteTagLocalUidToNameBimap.right.find(tagName);
    if (Q_UNLIKELY(tagNameIt == m_currentNoteTagLocalUidToNameBimap.right.end()))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't determine the tag which has been removed from "
                       "the note"));
        QNWARNING("widget:note_tags", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    QString tagLocalUid = tagNameIt->second;

    QNTRACE("widget:note_tags", "Local uid of the removed tag: "
        << tagLocalUid);

    const auto * pModelItem = m_pTagModel->itemForLocalUid(tagLocalUid);
    if (Q_UNLIKELY(!pModelItem))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't find the tag model item attempted to be removed "
                       "from the note"));

        QNWARNING("widget:note_tags", errorDescription << ", tag local uid = "
            << tagLocalUid);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    const TagItem * pTagItem = pModelItem->cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Internal error: the tag model item found by tag local "
                       "uid is not of a tag type"));

        QNWARNING("widget:note_tags", errorDescription << ", tag local uid = "
            << tagLocalUid << ", tag model item: " << *pModelItem);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    m_currentNote.removeTagLocalUid(tagLocalUid);

    const QString & tagGuid = pTagItem->guid();
    if (!tagGuid.isEmpty()) {
        m_currentNote.removeTagGuid(tagGuid);
    }

    QNTRACE("widget:note_tags", "Emitting note tags list changed signal: tag "
        << "removed: local uid = " << tagLocalUid << ", guid = " << tagGuid
        << "; note local uid = " << m_currentNote.localUid());

    Q_EMIT noteTagsListChanged(m_currentNote);

    for(int i = 0, size = m_pLayout->count(); i < size; ++i)
    {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            QNWARNING("widget:note_tags", "Detected null item within "
                << "the layout");
            continue;
        }

        auto * pTagItemWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (Q_UNLIKELY(!pTagItemWidget)) {
            continue;
        }

        QString tagName = pTagItemWidget->name();

        auto lit = m_currentNoteTagLocalUidToNameBimap.right.find(tagName);
        if (Q_UNLIKELY(lit == m_currentNoteTagLocalUidToNameBimap.right.end()))
        {
            QNWARNING("widget:note_tags", "Found tag item widget which name "
                << "doesn't correspond to any registered local uid, tag item "
                << "name = " << tagName);

            continue;
        }

        const QString & currentTagLocalUid = lit->second;
        if (currentTagLocalUid != tagLocalUid) {
            continue;
        }

        auto * pNewItemLineEdit = findNewItemWidget();
        if (pNewItemLineEdit)
        {
            NewListItemLineEdit::ItemInfo removedItemInfo;
            removedItemInfo.m_name = tagName;

            removedItemInfo.m_linkedNotebookGuid =
                pTagItemWidget->linkedNotebookGuid();

            removedItemInfo.m_linkedNotebookUsername =
                pTagItemWidget->linkedNotebookUsername();

            pNewItemLineEdit->removeReservedItem(removedItemInfo);
        }

        Q_UNUSED(m_pLayout->takeAt(i));
        pTagItemWidget->hide();
        pTagItemWidget->deleteLater();
        break;
    }

    m_lastDisplayedTagLocalUids.removeOne(tagLocalUid);

    auto lit = m_currentNoteTagLocalUidToNameBimap.left.find(tagLocalUid);
    if (lit != m_currentNoteTagLocalUidToNameBimap.left.end()) {
        Q_UNUSED(m_currentNoteTagLocalUidToNameBimap.left.erase(lit))
    }
}

void NoteTagsWidget::onNewTagNameEntered()
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::onNewTagNameEntered");

    auto * pNewItemLineEdit = qobject_cast<NewListItemLineEdit*>(sender());
    if (Q_UNLIKELY(!pNewItemLineEdit))
    {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't process the addition of a new "
                       "tag: can't cast the signal sender to NewListLineEdit"));

        QNWARNING("widget:note_tags", error);
        Q_EMIT notifyError(error);
        return;
    }

    QString newTagName = pNewItemLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(newTagName);

    QNDEBUG("widget:note_tags", "New tag name: " << newTagName);

    if (newTagName.isEmpty()) {
        return;
    }

    pNewItemLineEdit->setText(QString());
    if (!pNewItemLineEdit->hasFocus()) {
        pNewItemLineEdit->setFocus();
    }

    if (Q_UNLIKELY(newTagName.isEmpty())) {
        QNDEBUG("widget:note_tags", "New note's tag name is empty, skipping");
        return;
    }

    if (Q_UNLIKELY(m_pTagModel.isNull()))
    {
        ErrorString error(
            QT_TR_NOOP("Can't process the addition of a new tag: the tag model "
                       "is null"));

        QNWARNING("widget:note_tags", error);
        Q_EMIT notifyError(error);
        return;
    }

    if (Q_UNLIKELY(m_currentNote.localUid().isEmpty())) {
        QNDEBUG("widget:note_tags", "No current note is set, ignoring the tag "
            << "addition event");
        return;
    }

    auto tagIndex = m_pTagModel->indexForTagName(
        newTagName,
        m_currentLinkedNotebookGuid);

    if (!tagIndex.isValid())
    {
        QNDEBUG("widget:note_tags", "The tag with such name doesn't exist, "
            << "adding it");

        ErrorString errorDescription;

        tagIndex = m_pTagModel->createTag(
            newTagName, /* parent tag name = */ QString(),
            m_currentLinkedNotebookGuid,
            errorDescription);

        if (Q_UNLIKELY(!tagIndex.isValid()))
        {
            ErrorString error(QT_TR_NOOP("Can't add a new tag"));
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            QNINFO("widget:note_tags", error);
            Q_EMIT notifyError(error);
            return;
        }
    }

    const auto * pModelItem = m_pTagModel->itemForIndex(tagIndex);
    if (Q_UNLIKELY(!pModelItem))
    {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't process the addition of a new "
                       "tag: can't find the tag model item for index within "
                       "the tag model"));
        QNWARNING("widget:note_tags", error);
        Q_EMIT notifyError(error);
        return;
    }

    const TagItem * pTagItem = pModelItem->cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem))
    {
        ErrorString error(
            QT_TR_NOOP("Internal error: the tag model item found by tag name "
                       "is not of a tag type"));

        QNWARNING("widget:note_tags", error << ", tag model item: "
            << *pModelItem);

        Q_EMIT notifyError(error);
        return;
    }

    if (!m_currentLinkedNotebookGuid.isEmpty() &&
        (pTagItem->linkedNotebookGuid() != m_currentLinkedNotebookGuid))
    {
        ErrorString error(
            QT_TR_NOOP("Can't link note from external (linked) notebook with "
                       "tag from your own account"));
        QNDEBUG("widget:note_tags", error);
        Q_EMIT notifyError(error);
        return;
    }

    const QString & tagLocalUid = pTagItem->localUid();

    if (m_currentNote.hasTagLocalUids())
    {
        const auto & tagLocalUids = m_currentNote.tagLocalUids();
        if (tagLocalUids.contains(tagLocalUid)) {
            QNDEBUG("widget:note_tags", "Current note already contains tag "
                << "local uid " << tagLocalUid);
            return;
        }
    }

    m_currentNote.addTagLocalUid(tagLocalUid);

    const QString & tagGuid = pTagItem->guid();
    if (!tagGuid.isEmpty()) {
        m_currentNote.addTagGuid(tagGuid);
    }

    QNTRACE("widget:note_tags", "Emitting note tags list changed signal: tag "
        << "added: local uid = " << tagLocalUid << ", guid = " << tagGuid
        << "; note local uid = " << m_currentNote.localUid());

    Q_EMIT noteTagsListChanged(m_currentNote);

    const QString & tagName = pTagItem->name();

    m_lastDisplayedTagLocalUids << tagLocalUid;
    m_currentNoteTagLocalUidToNameBimap.insert(
        TagLocalUidToNameBimap::value_type(tagLocalUid, tagName));

    bool newItemLineEditHadFocus = false;

    NewListItemLineEdit::ItemInfo reservedItemInfo;
    reservedItemInfo.m_name = tagName;
    reservedItemInfo.m_linkedNotebookGuid = pTagItem->linkedNotebookGuid();

    if (!reservedItemInfo.m_linkedNotebookGuid.isEmpty())
    {
        auto linkedNotebooksInfo = m_pTagModel->linkedNotebooksInfo();
        for(const auto & linkedNotebookInfo: qAsConst(linkedNotebooksInfo))
        {
            if (linkedNotebookInfo.m_guid ==
                reservedItemInfo.m_linkedNotebookGuid)
            {
                reservedItemInfo.m_linkedNotebookUsername =
                    linkedNotebookInfo.m_username;

                break;
            }
        }
    }

    pNewItemLineEdit->addReservedItem(std::move(reservedItemInfo));

    newItemLineEditHadFocus = pNewItemLineEdit->hasFocus();
    Q_UNUSED(m_pLayout->removeWidget(pNewItemLineEdit))

    auto * pTagWidget = new ListItemWidget(tagName, tagLocalUid, this);
    pTagWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    pTagWidget->setItemRemovable(m_tagRestrictions.m_canUpdateNote);

    QObject::connect(
        pTagWidget,
        &ListItemWidget::itemRemovedFromList,
        this,
        &NoteTagsWidget::onTagRemoved);

    QObject::connect(
        this,
        &NoteTagsWidget::canUpdateNoteRestrictionChanged,
        pTagWidget,
        &ListItemWidget::setItemRemovable);

    m_pLayout->addWidget(pTagWidget);

    m_pLayout->addWidget(pNewItemLineEdit);
    pNewItemLineEdit->setText(QString());

    if (newItemLineEditHadFocus) {
        pNewItemLineEdit->setFocus();
    }
}

void NoteTagsWidget::onAllTagsListed()
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::onAllTagsListed");

    if (m_pTagModel.isNull()) {
        return;
    }

    QObject::disconnect(
        m_pTagModel.data(),
        &TagModel::notifyAllTagsListed,
        this,
        &NoteTagsWidget::onAllTagsListed);

    updateLayout();
}

void NoteTagsWidget::onUpdateNoteComplete(
    Note note, LocalStorageManager::UpdateNoteOptions options, QUuid requestId)
{
    Q_UNUSED(requestId)

    if (!(options & LocalStorageManager::UpdateNoteOption::UpdateTags)) {
        return;
    }

    if (note.localUid() != m_currentNote.localUid()) {
        return;
    }

    QNTRACE("widget:note_tags", "NoteTagsWidget::onUpdateNoteComplete: "
        << "note local uid = " << note.localUid() << ", request id = "
        << requestId);

    if ((options & LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata) &&
        (options & LocalStorageManager::UpdateNoteOption::UpdateResourceBinaryData))
    {
        m_currentNote = note;
    }
    else
    {
        auto resources = m_currentNote.resources();
        m_currentNote = note;
        m_currentNote.setResources(resources);
    }

    updateLayout();
}

void NoteTagsWidget::onExpungeNoteComplete(Note note, QUuid requestId)
{
    if (note.localUid() != m_currentNote.localUid()) {
        return;
    }

    QNDEBUG("widget:note_tags", "NoteTagsWidget::onExpungeNoteComplete: "
        << "note local uid = " << note.localUid() << ", request id = "
        << requestId);

    clear();
    clearLayout();
}

void NoteTagsWidget::onUpdateNotebookComplete(
    Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    if (notebook.localUid() != m_currentNotebookLocalUid) {
        return;
    }

    QNTRACE("widget:note_tags", "NoteTagsWidget::onUpdateNotebookComplete: "
        << "notebook = " << notebook << "\nRequest id = " << requestId);

    bool changed = false;
    bool canUpdateNote = false;
    bool canUpdateTags = false;

    if (notebook.hasRestrictions())
    {
        const auto & restrictions = notebook.restrictions();

        canUpdateNote =
            !(restrictions.noCreateTags.isSet() &&
              restrictions.noCreateTags.ref());

        canUpdateTags =
            !(restrictions.noUpdateTags.isSet() &&
              restrictions.noUpdateTags.ref());
    }
    else
    {
        canUpdateNote = true;
        canUpdateTags = true;
    }

    changed |= (canUpdateNote != m_tagRestrictions.m_canUpdateNote);
    changed |= (canUpdateTags != m_tagRestrictions.m_canUpdateTags);
    m_tagRestrictions.m_canUpdateNote = canUpdateNote;
    m_tagRestrictions.m_canUpdateTags = canUpdateTags;

    if (!changed) {
        QNTRACE("widget:note_tags", "No notebook restrictions were changed, "
            << "nothing to do");
        return;
    }

    Q_EMIT canUpdateNoteRestrictionChanged(m_tagRestrictions.m_canUpdateNote);

    if (!m_tagRestrictions.m_canUpdateNote) {
        removeNewTagWidgetFromLayout();
    }
}

void NoteTagsWidget::onExpungeNotebookComplete(
    Notebook notebook, QUuid requestId)
{
    Q_UNUSED(requestId)

    if (notebook.localUid() != m_currentNotebookLocalUid) {
        return;
    }

    QNTRACE("widget:note_tags", "NoteTagsWidget::onExpungeNotebookComplete: "
        << "notebook = " << notebook << "\nRequest id = " << requestId);

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

    QNDEBUG("widget:note_tags", "NoteTagsWidget::onUpdateTagComplete: tag = "
        << tag << "\nRequest id = " << requestId);

    auto previousNameIt = m_currentNoteTagLocalUidToNameBimap.left.find(
        tag.localUid());

    if (Q_UNLIKELY(previousNameIt ==
                   m_currentNoteTagLocalUidToNameBimap.left.end()))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Detected the update of a tag, however, its previous "
                       "name cannot be found"));
        QNWARNING("widget:note_tags", errorDescription << ", tag = " << tag);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    QString tagName = (tag.hasName() ? tag.name() : QString());
    QString previousName = previousNameIt->second;
    if (tag.hasName() && (previousName == tagName)) {
        QNDEBUG("widget:note_tags", "The tag's name hasn't changed, nothing to "
            << "do");
        return;
    }

    m_currentNoteTagLocalUidToNameBimap.left.replace_data(
        previousNameIt,
        tagName);

    // Need to find the note tag widget responsible for this tag and to change
    // its displayed name
    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNoteTagWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (!pNoteTagWidget) {
            continue;
        }

        if (pNoteTagWidget->name() != previousName) {
            continue;
        }

        if (!tag.hasName())
        {
            QNDEBUG("widget:note_tags", "Detected the update of tag not having "
                << "any name... Strange enough, will just remove that tag's "
                << "widget");

            pItem = m_pLayout->takeAt(i);
            pItem->widget()->hide();
            pItem->widget()->deleteLater();
        }
        else
        {
            pNoteTagWidget->setName(tagName);
        }

        break;
    }
}

void NoteTagsWidget::onExpungeTagComplete(
    Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::onExpungeTagComplete: tag = "
        << tag << "\nRequest id = " << requestId);

    Q_UNUSED(requestId)

    QStringList expungedTagLocalUids;
    expungedTagLocalUids << tag.localUid();
    expungedTagLocalUids << expungedChildTagLocalUids;

    for(const auto & tagLocalUid: qAsConst(expungedTagLocalUids)) {
        removeTagWidgetFromLayout(tagLocalUid);
    }
}

void NoteTagsWidget::clearLayout(const bool skipNewTagWidget)
{
    while(m_pLayout->count() > 0) {
        auto * pItem = m_pLayout->takeAt(0);
        pItem->widget()->hide();
        pItem->widget()->deleteLater();
    }

    m_lastDisplayedTagLocalUids.clear();
    m_currentNoteTagLocalUidToNameBimap.clear();

    addTagIconToLayout();

    if (skipNewTagWidget || m_currentNote.localUid().isEmpty() ||
        m_currentNotebookLocalUid.isEmpty() ||
        !m_tagRestrictions.m_canUpdateNote)
    {
        setTagItemsRemovable(false);
        return;
    }

    if (m_tagRestrictions.m_canUpdateNote) {
        addNewTagWidgetToLayout();
    }

    setTagItemsRemovable(m_tagRestrictions.m_canUpdateNote);
}

void NoteTagsWidget::updateLayout()
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::updateLayout");

    const QString & noteLocalUid = m_currentNote.localUid();
    if (Q_UNLIKELY(noteLocalUid.isEmpty())) {
        QNTRACE("widget:note_tags", "Note local uid is empty, nothing to do");
        return;
    }

    if (!m_currentNote.hasTagLocalUids())
    {
        if (m_lastDisplayedTagLocalUids.isEmpty()) {
            QNTRACE("widget:note_tags", "Note tags are still empty, nothing to "
                << "do");
        }
        else {
            QNTRACE("widget:note_tags", "The last tag has been removed from "
                << "the note");
        }

        clearLayout();
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
    else
    {
        shouldUpdateLayout = true;
    }

    if (!shouldUpdateLayout) {
        QNTRACE("widget:note_tags", "Note's tag local uids haven't changed, no "
            << "need to update the layout");
        return;
    }

    clearLayout(/* skip new tag widget = */ true);

    if (!m_pTagModel->allTagsListed())
    {
        QNDEBUG("widget:note_tags", "Not all tags have been listed within "
            << "the tag model yet");

        QObject::connect(
            m_pTagModel.data(),
            &TagModel::notifyAllTagsListed,
            this,
            &NoteTagsWidget::onAllTagsListed,
            Qt::UniqueConnection);

        return;
    }

    m_lastDisplayedTagLocalUids.reserve(numTags);

    QStringList tagNames;
    tagNames.reserve(numTags);

    for(int i = 0; i < numTags; ++i)
    {
        const QString & tagLocalUid = tagLocalUids[i];

        const auto * pModelItem = m_pTagModel->itemForLocalUid(tagLocalUid);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING("widget:note_tags", "Can't find tag model item for tag "
                << "with local uid " << tagLocalUid);
            continue;
        }

        const auto * pTagItem = pModelItem->cast<TagItem>();
        if (Q_UNLIKELY(!pTagItem)) {
            QNWARNING("widget:note_tags", "Skipping the tag model item found "
                << "by tag local uid yet containing no actual tag item: "
                << *pModelItem);
            continue;
        }

        m_lastDisplayedTagLocalUids << tagLocalUid;

        const QString & tagName = pTagItem->name();
        if (Q_UNLIKELY(tagName.isEmpty())) {
            QNDEBUG("widget:note_tags", "Skipping the tag with empty name, "
                << "local uid = " << tagLocalUid);
            continue;
        }

        m_currentNoteTagLocalUidToNameBimap.insert(
            TagLocalUidToNameBimap::value_type(tagLocalUid, tagName));

        tagNames << tagName;
    }

    for(int i = 0, size = tagNames.size(); i < size; ++i)
    {
        const QString & tagName = tagNames[i];
        const QString & tagLocalUid = tagLocalUids[i];

        auto * pTagWidget = new ListItemWidget(tagName, tagLocalUid, this);
        pTagWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        pTagWidget->setItemRemovable(m_tagRestrictions.m_canUpdateNote);

        QObject::connect(
            pTagWidget,
            &ListItemWidget::itemRemovedFromList,
            this,
            &NoteTagsWidget::onTagRemoved);

        QObject::connect(
            this,
            &NoteTagsWidget::canUpdateNoteRestrictionChanged,
            pTagWidget,
            &ListItemWidget::setItemRemovable);

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
    QNTRACE("widget:note_tags", "NoteTagsWidget::addTagIconToLayout");

    QPixmap tagIconImage(QStringLiteral(":/tag/tag.png"));
    auto * pTagIconLabel = new QLabel(this);
    pTagIconLabel->setPixmap(tagIconImage.scaled(QSize(20, 20)));
    pTagIconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pLayout->addWidget(pTagIconLabel);
}

void NoteTagsWidget::addNewTagWidgetToLayout()
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::addNewTagWidgetToLayout");

    const int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNewItemWidget = qobject_cast<NewListItemLineEdit*>(
            pItem->widget());

        if (!pNewItemWidget) {
            continue;
        }

        pItem = m_pLayout->takeAt(i);
        pItem->widget()->hide();
        pItem->widget()->deleteLater();
        break;
    }

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        QNWARNING("widget:note_tags", "No tag model is set, won't add the new "
            << "tag widget");
        return;
    }

    if (!m_pTagModel->allTagsListed())
    {
        QNDEBUG("widget:note_tags", "Not all tags have been listed within "
            << "the tag model yet");

        QObject::connect(
            m_pTagModel.data(),
            &TagModel::notifyAllTagsListed,
            this,
            &NoteTagsWidget::onAllTagsListed,
            Qt::UniqueConnection);

        return;
    }

    QVector<NewListItemLineEdit::ItemInfo> reservedItems;

    reservedItems.reserve(
        static_cast<int>(m_currentNoteTagLocalUidToNameBimap.size()));

    for(auto it = m_currentNoteTagLocalUidToNameBimap.right.begin(),
        end = m_currentNoteTagLocalUidToNameBimap.right.end(); it != end; ++it)
    {
        NewListItemLineEdit::ItemInfo item;
        item.m_name = it->first;
        item.m_linkedNotebookGuid = m_currentLinkedNotebookGuid;

        if (!item.m_linkedNotebookGuid.isEmpty())
        {
            auto linkedNotebooksInfo = m_pTagModel->linkedNotebooksInfo();
            for(const auto & linkedNotebookInfo: qAsConst(linkedNotebooksInfo))
            {
                if (linkedNotebookInfo.m_guid == item.m_linkedNotebookGuid) {
                    item.m_linkedNotebookUsername =
                        linkedNotebookInfo.m_username;

                    break;
                }
            }
        }
    }

    auto * pNewTagLineEdit = new NewListItemLineEdit(
        m_pTagModel,
        reservedItems,
        this);

    pNewTagLineEdit->setTargetLinkedNotebookGuid(
        m_currentLinkedNotebookGuid.isEmpty()
        ? QLatin1String("")
        : m_currentLinkedNotebookGuid);

    QObject::connect(
        pNewTagLineEdit,
        &NewListItemLineEdit::returnPressed,
        this,
        &NoteTagsWidget::onNewTagNameEntered);

    QObject::connect(
        pNewTagLineEdit,
        &NewListItemLineEdit::receivedFocusFromWindowSystem,
        this,
        &NoteTagsWidget::newTagLineEditReceivedFocusFromWindowSystem);

    m_pLayout->addWidget(pNewTagLineEdit);
}

void NoteTagsWidget::removeNewTagWidgetFromLayout()
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::removeNewTagWidgetFromLayout");

    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNewTagLineEdit = qobject_cast<NewListItemLineEdit*>(
            pItem->widget());

        if (!pNewTagLineEdit) {
            continue;
        }

        pItem = m_pLayout->takeAt(i);
        delete pItem->widget();
        delete pItem;
        break;
    }
}

void NoteTagsWidget::removeTagWidgetFromLayout(const QString & tagLocalUid)
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::removeTagWidgetFromLayout: "
        << "tag local uid = " << tagLocalUid);

    int tagIndex = m_lastDisplayedTagLocalUids.indexOf(tagLocalUid);
    if (tagIndex < 0) {
        QNDEBUG("widget:note_tags", "This tag local uid is not listed within "
            << "the ones displayed");
        return;
    }

    m_lastDisplayedTagLocalUids.removeAt(tagIndex);

    QString tagName;

    auto it = m_currentNoteTagLocalUidToNameBimap.left.find(tagLocalUid);
    if (Q_UNLIKELY(it == m_currentNoteTagLocalUidToNameBimap.left.end()))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Detected the expunge of a tag, however, its name "
                       "cannot be found"));

        QNWARNING("widget:note_tags", errorDescription << ", tag local uid = "
            << tagLocalUid);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    tagName = it->second;

    // Need to find the note tag widget responsible for this tag and remove it
    // from the layout
    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNoteTagWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (!pNoteTagWidget) {
            continue;
        }

        if (pNoteTagWidget->name() != tagName) {
            continue;
        }

        pItem = m_pLayout->takeAt(i);
        pItem->widget()->hide();
        pItem->widget()->deleteLater();
        break;
    }
}

void NoteTagsWidget::setTagItemsRemovable(const bool removable)
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::setTagItemsRemovable: "
        << "removable = " << (removable ? "true" : "false"));

    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNoteTagWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (!pNoteTagWidget) {
            continue;
        }

        pNoteTagWidget->setItemRemovable(removable);
    }
}

void NoteTagsWidget::createConnections(
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::createConnections");

    // Connect local storage signals to local slots
    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNoteComplete,
        this,
        &NoteTagsWidget::onUpdateNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNoteComplete,
        this,
        &NoteTagsWidget::onExpungeNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookComplete,
        this,
        &NoteTagsWidget::onUpdateNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookComplete,
        this,
        &NoteTagsWidget::onExpungeNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateTagComplete,
        this,
        &NoteTagsWidget::onUpdateTagComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeTagComplete,
        this,
        &NoteTagsWidget::onExpungeTagComplete);
}

NewListItemLineEdit * NoteTagsWidget::findNewItemWidget()
{
    const int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNewItemWidget = qobject_cast<NewListItemLineEdit*>(
            pItem->widget());

        if (!pNewItemWidget) {
            continue;
        }

        return pNewItemWidget;
    }

    return nullptr;
}

bool NoteTagsWidget::isActive() const
{
    return !m_currentNote.localUid().isEmpty();
}

QStringList NoteTagsWidget::tagNames() const
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::tagNames");

    if (!isActive()) {
        QNDEBUG("widget:note_tags", "NoteTagsWidget is not active");
        return {};
    }

    QStringList result;

    result.reserve(
        static_cast<int>(m_currentNoteTagLocalUidToNameBimap.size()));

    for(auto it = m_currentNoteTagLocalUidToNameBimap.right.begin(),
        end = m_currentNoteTagLocalUidToNameBimap.right.end(); it != end; ++it)
    {
        result << it->first;
        QNTRACE("widget:note_tags", "Added tag name " << result.back());
    }

    return result;
}

} // namespace quentier
