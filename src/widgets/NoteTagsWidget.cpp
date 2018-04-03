/*
 * Copyright 2016-2017 Dmitry Ivanov
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
#include "ListItemWidget.h"
#include "NewListItemLineEdit.h"
#include "FlowLayout.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QLabel>
#include <QKeyEvent>
#include <QApplication>

namespace quentier {

NoteTagsWidget::NoteTagsWidget(QWidget * parent) :
    QWidget(parent),
    m_currentNote(),
    m_currentNotebookLocalUid(),
    m_currentLinkedNotebookGuid(),
    m_lastDisplayedTagLocalUids(),
    m_currentNoteTagLocalUidToNameBimap(),
    m_pTagModel(),
    m_updateNoteRequestIdToRemovedTagLocalUidAndGuid(),
    m_updateNoteRequestIdToAddedTagLocalUidAndGuid(),
    m_tagRestrictions(),
    m_stringUtils(),
    m_pLayout(new FlowLayout)
{
    addTagIconToLayout();
    setLayout(m_pLayout);
}

void NoteTagsWidget::setLocalStorageManagerThreadWorker(LocalStorageManagerAsync & localStorageManagerAsync)
{
    createConnections(localStorageManagerAsync);
}

void NoteTagsWidget::setTagModel(TagModel * pTagModel)
{
    if (!m_pTagModel.isNull()) {
        QObject::disconnect(m_pTagModel.data(), QNSIGNAL(TagModel,notifyAllTagsListed),
                            this, QNSLOT(NoteTagsWidget,onAllTagsListed));
    }

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

    QString linkedNotebookGuid;
    if (notebook.hasLinkedNotebookGuid()) {
        linkedNotebookGuid = notebook.linkedNotebookGuid();
    }

    changed |= (m_currentLinkedNotebookGuid != linkedNotebookGuid);

    m_currentNotebookLocalUid = notebook.localUid();
    m_currentLinkedNotebookGuid = linkedNotebookGuid;

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
    m_currentLinkedNotebookGuid.clear();
    m_currentNoteTagLocalUidToNameBimap.clear();
    m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.clear();
    m_updateNoteRequestIdToAddedTagLocalUidAndGuid.clear();
    m_tagRestrictions = Restrictions();
}

void NoteTagsWidget::onTagRemoved(QString tagName)
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::onTagRemoved: tag name = ") << tagName);

    if (Q_UNLIKELY(m_currentNote.localUid().isEmpty())) {
        QNDEBUG(QStringLiteral("No current note is set, ignoring the tag removal event"));
        return;
    }

    auto tagNameIt = m_currentNoteTagLocalUidToNameBimap.right.find(tagName);
    if (Q_UNLIKELY(tagNameIt == m_currentNoteTagLocalUidToNameBimap.right.end())) {
        ErrorString errorDescription(QT_TR_NOOP("Can't determine the tag which has been removed from the note"));
        QNWARNING(errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    QString tagLocalUid = tagNameIt->second;
    QNTRACE(QStringLiteral("Local uid of the removed tag: ") << tagLocalUid);

    const TagModelItem * pModelItem = m_pTagModel->itemForLocalUid(tagLocalUid);
    if (Q_UNLIKELY(!pModelItem)) {
        ErrorString errorDescription(QT_TR_NOOP("Can't find the tag model item attempted to be removed from the note"));
        QNWARNING(errorDescription << QStringLiteral(", tag local uid = ") << tagLocalUid);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    if (Q_UNLIKELY(pModelItem->type() != TagModelItem::Type::Tag)) {
        ErrorString errorDescription(QT_TR_NOOP("Internal error: the tag model item found by tag local uid "
                                                "is not of a tag type"));
        QNWARNING(errorDescription << QStringLiteral(", tag local uid = ") << tagLocalUid
                  << QStringLiteral(", tag model item: ") << *pModelItem);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        ErrorString errorDescription(QT_TR_NOOP("Internal error: the tag model item found by tag local uid "
                                                "contains no tag item even though it is of a tag type"));
        QNWARNING(errorDescription << QStringLiteral(", tag local uid = ") << tagLocalUid
                  << QStringLiteral(", tag model item: ") << *pModelItem);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    m_currentNote.removeTagLocalUid(tagLocalUid);

    const QString & tagGuid = pTagItem->guid();
    if (!tagGuid.isEmpty()) {
        m_currentNote.removeTagGuid(tagGuid);
    }

    m_currentNote.setDirty(true);
    m_currentNote.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    QUuid updateNoteRequestId = QUuid::createUuid();
    m_updateNoteRequestIdToRemovedTagLocalUidAndGuid[updateNoteRequestId] =
            std::pair<QString, QString>(tagLocalUid, tagGuid);

    QNTRACE(QStringLiteral("Emitting the request to update the note in the local storage: request id = ")
            << updateNoteRequestId << QStringLiteral(", note = ") << m_currentNote);
    Q_EMIT updateNote(m_currentNote, /* update resources = */ false, /* update tags = */ true, updateNoteRequestId);
}

void NoteTagsWidget::onNewTagNameEntered()
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::onNewTagNameEntered"));

    NewListItemLineEdit * pNewItemLineEdit = qobject_cast<NewListItemLineEdit*>(sender());
    if (Q_UNLIKELY(!pNewItemLineEdit)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't process the addition of a new tag: "
                                     "can't cast the signal sender to NewListLineEdit"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    QString newTagName = pNewItemLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(newTagName);

    QNDEBUG(QStringLiteral("New tag name: ") << newTagName);

    if (newTagName.isEmpty()) {
        return;
    }

    pNewItemLineEdit->setText(QString());
    if (!pNewItemLineEdit->hasFocus()) {
        pNewItemLineEdit->setFocus();
    }

    if (Q_UNLIKELY(newTagName.isEmpty())) {
        QNDEBUG(QStringLiteral("New note's tag name is empty, skipping"));
        return;
    }

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        ErrorString error(QT_TR_NOOP("Can't process the addition of a new tag: the tag model is null"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    if (Q_UNLIKELY(m_currentNote.localUid().isEmpty())) {
        QNDEBUG(QStringLiteral("No current note is set, ignoring the tag addition event"));
        return;
    }

    QModelIndex tagIndex = m_pTagModel->indexForTagName(newTagName, m_currentLinkedNotebookGuid);
    if (!tagIndex.isValid())
    {
        QNDEBUG(QStringLiteral("The tag with such name doesn't exist, adding it"));

        ErrorString errorDescription;
        tagIndex = m_pTagModel->createTag(newTagName, /* parent tag name = */ QString(),
                                          m_currentLinkedNotebookGuid, errorDescription);
        if (Q_UNLIKELY(!tagIndex.isValid())) {
            ErrorString error(QT_TR_NOOP("Can't add a new tag"));
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            QNINFO(error);
            Q_EMIT notifyError(error);
            return;
        }
    }

    const TagModelItem * pModelItem = m_pTagModel->itemForIndex(tagIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't process the addition of a new tag: "
                                     "can't find the tag model item for index within the tag model"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    if (Q_UNLIKELY(pModelItem->type() != TagModelItem::Type::Tag)) {
        ErrorString error(QT_TR_NOOP("Internal error: the tag model item found by tag name "
                                     "is not of a tag type"));
        QNWARNING(error << QStringLiteral(", tag model item: ") << *pModelItem);
        Q_EMIT notifyError(error);
        return;
    }

    const TagItem * pTagItem = pModelItem->tagItem();
    if (Q_UNLIKELY(!pTagItem)) {
        ErrorString error(QT_TR_NOOP("Internal error: the tag model item found by tag name "
                                     "contains no actual tag item even though it is of a tag type"));
        QNWARNING(error << QStringLiteral(", tag model item: ") << *pModelItem);
        Q_EMIT notifyError(error);
        return;
    }

    if (!m_currentLinkedNotebookGuid.isEmpty() && (pTagItem->linkedNotebookGuid() != m_currentLinkedNotebookGuid)) {
        ErrorString error(QT_TR_NOOP("Can't link the note from the external (linked) notebook with tag from your own account"));
        QNDEBUG(error);
        Q_EMIT notifyError(error);
        return;
    }

    if (m_currentNote.hasTagLocalUids())
    {
        const QStringList & tagLocalUids = m_currentNote.tagLocalUids();
        if (tagLocalUids.contains(pTagItem->localUid())) {
            QNDEBUG(QStringLiteral("The current note already contains tag local uid ")
                    << pTagItem->localUid());
            return;
        }
    }

    m_currentNote.addTagLocalUid(pTagItem->localUid());

    const QString & tagGuid = pTagItem->guid();
    if (!tagGuid.isEmpty()) {
        m_currentNote.addTagGuid(pTagItem->guid());
    }

    m_currentNote.setDirty(true);
    m_currentNote.setModificationTimestamp(QDateTime::currentMSecsSinceEpoch());

    QUuid updateNoteRequestId = QUuid::createUuid();
    m_updateNoteRequestIdToAddedTagLocalUidAndGuid[updateNoteRequestId] =
            std::pair<QString, QString>(pTagItem->localUid(), pTagItem->guid());

    QNTRACE(QStringLiteral("Emitting the request to update the note in the local storage: request id = ")
            << updateNoteRequestId << QStringLiteral(", note = ") << m_currentNote);
    Q_EMIT updateNote(m_currentNote, /* update resources = */ false, /* update tags = */ true, updateNoteRequestId);
}

void NoteTagsWidget::onAllTagsListed()
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::onAllTagsListed"));

    if (m_pTagModel.isNull()) {
        return;
    }

    QObject::disconnect(m_pTagModel.data(), QNSIGNAL(TagModel,notifyAllTagsListed),
                        this, QNSLOT(NoteTagsWidget,onAllTagsListed));
    updateLayout();
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

    auto ait = m_updateNoteRequestIdToAddedTagLocalUidAndGuid.find(requestId);
    if (ait != m_updateNoteRequestIdToAddedTagLocalUidAndGuid.end())
    {
        const QString & addedTagLocalUid = ait.value().first;

        const TagModelItem * pModelItem = m_pTagModel->itemForLocalUid(addedTagLocalUid);
        if (Q_UNLIKELY(!pModelItem)) {
            ErrorString error(QT_TR_NOOP("Can't process the tag addition to note: "
                                         "the tag model item was not found within the model"));
            QNWARNING(error << QStringLiteral(", request id = ") << requestId
                      << QStringLiteral(", tag local uid = ") << addedTagLocalUid);
            Q_EMIT notifyError(error);
            return;
        }

        if (Q_UNLIKELY(pModelItem->type() != TagModelItem::Type::Tag)) {
            ErrorString error(QT_TR_NOOP("Internal error: the tag model item found by tag local uid "
                                         "is not of a tag type"));
            QNWARNING(error << QStringLiteral(", tag model item: ") << *pModelItem);
            Q_EMIT notifyError(error);
            return;
        }

        const TagItem * pTagItem = pModelItem->tagItem();
        if (Q_UNLIKELY(!pTagItem)) {
            ErrorString error(QT_TR_NOOP("Internal error: the tag model item found by tag local uid "
                                         "contains no actual tag item even though it is of a tag type"));
            QNWARNING(error << QStringLiteral(", tag model item: ") << *pModelItem);
            Q_EMIT notifyError(error);
            return;
        }

        const QString & tagName = pTagItem->name();

        m_lastDisplayedTagLocalUids << addedTagLocalUid;
        m_currentNoteTagLocalUidToNameBimap.insert(TagLocalUidToNameBimap::value_type(addedTagLocalUid, tagName));

        bool newItemLineEditHadFocus = false;
        NewListItemLineEdit * pNewItemLineEdit = findNewItemWidget();
        if (pNewItemLineEdit)
        {
            QStringList reservedTagNames = pNewItemLineEdit->reservedItemNames();
            reservedTagNames << tagName;
            pNewItemLineEdit->updateReservedItemNames(reservedTagNames);

            newItemLineEditHadFocus = pNewItemLineEdit->hasFocus();
            Q_UNUSED(m_pLayout->removeWidget(pNewItemLineEdit);)
        }

        ListItemWidget * pTagWidget = new ListItemWidget(tagName, this);
        pTagWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        pTagWidget->setItemRemovable(m_tagRestrictions.m_canUpdateNote);
        QObject::connect(pTagWidget, QNSIGNAL(ListItemWidget,itemRemovedFromList,QString),
                         this, QNSLOT(NoteTagsWidget,onTagRemoved,QString));
        QObject::connect(this, QNSIGNAL(NoteTagsWidget,canUpdateNoteRestrictionChanged,bool),
                         pTagWidget, QNSLOT(ListItemWidget,setItemRemovable,bool));

        m_pLayout->addWidget(pTagWidget);

        if (pNewItemLineEdit)
        {
            m_pLayout->addWidget(pNewItemLineEdit);
            pNewItemLineEdit->setText(QString());

            if (newItemLineEditHadFocus) {
                pNewItemLineEdit->setFocus();
            }
        }

        Q_UNUSED(m_updateNoteRequestIdToAddedTagLocalUidAndGuid.erase(ait))
        return;
    }

    auto rit = m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.find(requestId);
    if (rit != m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.end())
    {
        const QString & removedTagLocalUid = rit.value().first;

        for(int i = 0, size = m_pLayout->count(); i < size; ++i)
        {
            QLayoutItem * pItem = m_pLayout->itemAt(i);
            if (Q_UNLIKELY(!pItem)) {
                QNWARNING(QStringLiteral("Detected null item within the layout"));
                continue;
            }

            ListItemWidget * pTagItemWidget = qobject_cast<ListItemWidget*>(pItem->widget());
            if (Q_UNLIKELY(!pTagItemWidget)) {
                continue;
            }

            QString tagName = pTagItemWidget->name();

            auto lit = m_currentNoteTagLocalUidToNameBimap.right.find(tagName);
            if (Q_UNLIKELY(lit == m_currentNoteTagLocalUidToNameBimap.right.end())) {
                QNWARNING(QStringLiteral("Found tag item widget which name doesn't correspond "
                                         "to any registered local uid, tag item name = ") << tagName);
                continue;
            }

            const QString & tagLocalUid = lit->second;
            if (tagLocalUid != removedTagLocalUid) {
                continue;
            }

            NewListItemLineEdit * pNewItemLineEdit = findNewItemWidget();
            if (pNewItemLineEdit)
            {
                QStringList reservedTagNames = pNewItemLineEdit->reservedItemNames();
                QNTRACE(QStringLiteral("Reserved tag names before removing the name of the just removed tag from it: ")
                        << reservedTagNames.join(QStringLiteral(", ")) << QStringLiteral("; the name of the removed tag: ")
                        << tagName);
                if (reservedTagNames.removeOne(tagName)) {
                    pNewItemLineEdit->updateReservedItemNames(reservedTagNames);
                }
            }

            Q_UNUSED(m_pLayout->takeAt(i));
            pTagItemWidget->hide();
            pTagItemWidget->deleteLater();
            break;
        }

        m_lastDisplayedTagLocalUids.removeOne(removedTagLocalUid);

        auto lit = m_currentNoteTagLocalUidToNameBimap.left.find(removedTagLocalUid);
        if (lit != m_currentNoteTagLocalUidToNameBimap.left.end()) {
            Q_UNUSED(m_currentNoteTagLocalUidToNameBimap.left.erase(lit))
        }

        Q_UNUSED(m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.erase(rit))
        return;
    }

    updateLayout();
}

void NoteTagsWidget::onUpdateNoteFailed(Note note, bool updateResources, bool updateTags,
                                        ErrorString errorDescription, QUuid requestId)
{
    Q_UNUSED(updateResources)
    Q_UNUSED(updateTags)

    auto rit = m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.find(requestId);
    auto ait = m_updateNoteRequestIdToAddedTagLocalUidAndGuid.end();
    if (rit == m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.end()) {
        ait = m_updateNoteRequestIdToAddedTagLocalUidAndGuid.find(requestId);
    }

    if ( (rit == m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.end()) &&
         (ait == m_updateNoteRequestIdToAddedTagLocalUidAndGuid.end()) )
    {
        return;
    }

    QNDEBUG(QStringLiteral("NoteTagsWidget::onUpdateNoteFailed: note local uid = ") << note.localUid()
            << QStringLiteral(", error description: ") << errorDescription << QStringLiteral(", request id = ")
            << requestId);

    if (rit != m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.end())
    {
        const auto & idsPair = rit.value();

        m_currentNote.addTagLocalUid(idsPair.first);
        if (!idsPair.second.isEmpty()) {
            m_currentNote.addTagGuid(idsPair.second);
        }

        Q_UNUSED(m_updateNoteRequestIdToRemovedTagLocalUidAndGuid.erase(rit))
    }
    else if (ait != m_updateNoteRequestIdToAddedTagLocalUidAndGuid.end())
    {
        const auto & idsPair = ait.value();

        m_currentNote.removeTagLocalUid(idsPair.first);
        if (!idsPair.second.isEmpty()) {
            m_currentNote.removeTagGuid(idsPair.second);
        }

        Q_UNUSED(m_updateNoteRequestIdToAddedTagLocalUidAndGuid.erase(ait))
    }

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

    Q_EMIT canUpdateNoteRestrictionChanged(m_tagRestrictions.m_canUpdateNote);

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

    auto previousNameIt = m_currentNoteTagLocalUidToNameBimap.left.find(tag.localUid());
    if (Q_UNLIKELY(previousNameIt == m_currentNoteTagLocalUidToNameBimap.left.end())) {
        ErrorString errorDescription(QT_TR_NOOP("Detected the update of a tag, however, its previous name cannot be found"));
        QNWARNING(errorDescription << QStringLiteral(", tag = ") << tag);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    QString tagName = (tag.hasName() ? tag.name() : QString());
    QString previousName = previousNameIt->second;
    if (tag.hasName() && (previousName == tagName)) {
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

        ListItemWidget * pNoteTagWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (!pNoteTagWidget) {
            continue;
        }

        if (pNoteTagWidget->name() != previousName) {
            continue;
        }

        if (!tag.hasName()) {
            QNDEBUG(QStringLiteral("Detected the update of tag not having any name... Strange enough, will just remove that tag's widget"));
            pItem = m_pLayout->takeAt(i);
            pItem->widget()->hide();
            pItem->widget()->deleteLater();
        }
        else {
            pNoteTagWidget->setName(tagName);
        }

        break;
    }
}

void NoteTagsWidget::onExpungeTagComplete(Tag tag, QStringList expungedChildTagLocalUids, QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::onExpungeTagComplete: tag = ") << tag
            << QStringLiteral("\nRequest id = ") << requestId);

    Q_UNUSED(requestId)

    QStringList expungedTagLocalUids;
    expungedTagLocalUids << tag.localUid();
    expungedTagLocalUids << expungedChildTagLocalUids;
    for(auto it = expungedTagLocalUids.constBegin(), end = expungedTagLocalUids.constEnd(); it != end; ++it) {
        removeTagWidgetFromLayout(*it);
    }
}

void NoteTagsWidget::clearLayout(const bool skipNewTagWidget)
{
    while(m_pLayout->count() > 0) {
        QLayoutItem * pItem = m_pLayout->takeAt(0);
        pItem->widget()->hide();
        pItem->widget()->deleteLater();
    }

    m_lastDisplayedTagLocalUids.clear();
    m_currentNoteTagLocalUidToNameBimap.clear();

    addTagIconToLayout();

    if (skipNewTagWidget || m_currentNote.localUid().isEmpty() ||
        m_currentNotebookLocalUid.isEmpty() || !m_tagRestrictions.m_canUpdateNote)
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
        QNTRACE(QStringLiteral("Note's tag local uids haven't changed, no need to update the layout"));
        return;
    }

    clearLayout(/* skip new tag widget = */ true);

    if (!m_pTagModel->allTagsListed())
    {
        QNDEBUG(QStringLiteral("Not all tags have been listed within the tag model yet"));
        QObject::connect(m_pTagModel.data(), QNSIGNAL(TagModel,notifyAllTagsListed),
                         this, QNSLOT(NoteTagsWidget,onAllTagsListed), Qt::UniqueConnection);
        return;
    }

    m_lastDisplayedTagLocalUids.reserve(numTags);

    QStringList tagNames;
    tagNames.reserve(numTags);

    for(int i = 0; i < numTags; ++i)
    {
        const QString & tagLocalUid = tagLocalUids[i];

        const TagModelItem * pModelItem = m_pTagModel->itemForLocalUid(tagLocalUid);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING(QStringLiteral("Can't find tag model item for tag with local uid ") << tagLocalUid);
            continue;
        }

        const TagItem * pTagItem = pModelItem->tagItem();
        if (Q_UNLIKELY(!pTagItem)) {
            QNWARNING(QStringLiteral("Skipping the tag model item found by tag local uid yet containing no actual tag item: ")
                      << *pModelItem);
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

    for(int i = 0, size = tagNames.size(); i < size; ++i)
    {
        const QString & tagName = tagNames[i];

        ListItemWidget * pTagWidget = new ListItemWidget(tagName, this);
        pTagWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        pTagWidget->setItemRemovable(m_tagRestrictions.m_canUpdateNote);
        QObject::connect(pTagWidget, QNSIGNAL(ListItemWidget,itemRemovedFromList,QString),
                         this, QNSLOT(NoteTagsWidget,onTagRemoved,QString));
        QObject::connect(this, QNSIGNAL(NoteTagsWidget,canUpdateNoteRestrictionChanged,bool),
                         pTagWidget, QNSLOT(ListItemWidget,setItemRemovable,bool));

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

    QPixmap tagIconImage(QStringLiteral(":/tag/tag.png"));
    QLabel * pTagIconLabel = new QLabel(this);
    pTagIconLabel->setPixmap(tagIconImage.scaled(QSize(20, 20)));
    pTagIconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pLayout->addWidget(pTagIconLabel);
}

void NoteTagsWidget::addNewTagWidgetToLayout()
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::addNewTagWidgetToLayout"));

    const int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        NewListItemLineEdit * pNewItemWidget = qobject_cast<NewListItemLineEdit*>(pItem->widget());
        if (!pNewItemWidget) {
            continue;
        }

        pItem = m_pLayout->takeAt(i);
        pItem->widget()->hide();
        pItem->widget()->deleteLater();
        break;
    }

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        QNWARNING(QStringLiteral("No tag model is set, won't add the new tag widget"));
        return;
    }

    if (!m_pTagModel->allTagsListed())
    {
        QNDEBUG(QStringLiteral("Not all tags have been listed within the tag model yet"));
        QObject::connect(m_pTagModel.data(), QNSIGNAL(TagModel,notifyAllTagsListed),
                         this, QNSLOT(NoteTagsWidget,onAllTagsListed), Qt::UniqueConnection);
        return;
    }

    QStringList existingTagNames;
    existingTagNames.reserve(static_cast<int>(m_currentNoteTagLocalUidToNameBimap.size()));
    for(auto it = m_currentNoteTagLocalUidToNameBimap.right.begin(), end = m_currentNoteTagLocalUidToNameBimap.right.end();
        it != end; ++it)
    {
        existingTagNames << it->first;
    }

    NewListItemLineEdit * pNewTagLineEdit = new NewListItemLineEdit(m_pTagModel, existingTagNames,
                                                                    m_currentLinkedNotebookGuid, this);
    QObject::connect(pNewTagLineEdit, QNSIGNAL(NewListItemLineEdit,returnPressed),
                     this, QNSLOT(NoteTagsWidget,onNewTagNameEntered));
    QObject::connect(pNewTagLineEdit, QNSIGNAL(NewListItemLineEdit,receivedFocusFromWindowSystem),
                     this, QNSIGNAL(NoteTagsWidget,newTagLineEditReceivedFocusFromWindowSystem));
    m_pLayout->addWidget(pNewTagLineEdit);
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

        NewListItemLineEdit * pNewTagLineEdit = qobject_cast<NewListItemLineEdit*>(pItem->widget());
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
    QNDEBUG(QStringLiteral("NoteTagsWidget::removeTagWidgetFromLayout: tag local uid = ") << tagLocalUid);

    int tagIndex = m_lastDisplayedTagLocalUids.indexOf(tagLocalUid);
    if (tagIndex < 0) {
        QNDEBUG(QStringLiteral("This tag local uid is not listed within the ones displayed"));
        return;
    }

    m_lastDisplayedTagLocalUids.removeAt(tagIndex);

    QString tagName;

    auto it = m_currentNoteTagLocalUidToNameBimap.left.find(tagLocalUid);
    if (Q_UNLIKELY(it == m_currentNoteTagLocalUidToNameBimap.left.end())) {
        ErrorString errorDescription(QT_TR_NOOP("Detected the expunge of a tag, however, its name cannot be found"));
        QNWARNING(errorDescription << QStringLiteral(", tag local uid = ") << tagLocalUid);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    tagName = it->second;

    // Need to find the note tag widget responsible for this tag and remove it from the layout
    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        ListItemWidget * pNoteTagWidget = qobject_cast<ListItemWidget*>(pItem->widget());
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
    QNDEBUG(QStringLiteral("NoteTagsWidget::setTagItemsRemovable: removable = ")
            << (removable ? QStringLiteral("true") : QStringLiteral("false")));

    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        ListItemWidget * pNoteTagWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (!pNoteTagWidget) {
            continue;
        }

        pNoteTagWidget->setItemRemovable(removable);
    }
}

void NoteTagsWidget::createConnections(LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::createConnections"));

    // Connect local storage signals to local slots
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteComplete,Note,bool,bool,QUuid),
                     this, QNSLOT(NoteTagsWidget,onUpdateNoteComplete,Note,bool,bool,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNoteFailed,Note,bool,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteTagsWidget,onUpdateNoteFailed,Note,bool,bool,ErrorString,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNoteComplete,Note,QUuid),
                     this, QNSLOT(NoteTagsWidget,onExpungeNoteComplete,Note,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteTagsWidget,onUpdateNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeNotebookComplete,Notebook,QUuid),
                     this, QNSLOT(NoteTagsWidget,onExpungeNotebookComplete,Notebook,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,updateTagComplete,Tag,QUuid),
                     this, QNSLOT(NoteTagsWidget,onUpdateTagComplete,Tag,QUuid));
    QObject::connect(&localStorageManagerAsync, QNSIGNAL(LocalStorageManagerAsync,expungeTagComplete,Tag,QStringList,QUuid),
                     this, QNSLOT(NoteTagsWidget,onExpungeTagComplete,Tag,QStringList,QUuid));

    // Connect local signals to local storage slots
    QObject::connect(this, QNSIGNAL(NoteTagsWidget,updateNote,Note,bool,bool,QUuid),
                     &localStorageManagerAsync, QNSLOT(LocalStorageManagerAsync,onUpdateNoteRequest,Note,bool,bool,QUuid));
}

NewListItemLineEdit * NoteTagsWidget::findNewItemWidget()
{
    const int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        NewListItemLineEdit * pNewItemWidget = qobject_cast<NewListItemLineEdit*>(pItem->widget());
        if (!pNewItemWidget) {
            continue;
        }

        return pNewItemWidget;
    }

    return Q_NULLPTR;
}

bool NoteTagsWidget::isActive() const
{
    return !m_currentNote.localUid().isEmpty();
}

QStringList NoteTagsWidget::tagNames() const
{
    QNDEBUG(QStringLiteral("NoteTagsWidget::tagNames"));

    if (!isActive()) {
        QNDEBUG(QStringLiteral("NoteTagsWidget is not active"));
        return QStringList();
    }

    QStringList result;
    result.reserve(static_cast<int>(m_currentNoteTagLocalUidToNameBimap.size()));

    for(auto it = m_currentNoteTagLocalUidToNameBimap.right.begin(),
        end = m_currentNoteTagLocalUidToNameBimap.right.end(); it != end; ++it)
    {
        result << it->first;
        QNTRACE(QStringLiteral("Added tag name ") << result.back());
    }

    return result;
}

} // namespace quentier
