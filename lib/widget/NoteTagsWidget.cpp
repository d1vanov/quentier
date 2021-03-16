/*
 * Copyright 2016-2021 Dmitry Ivanov
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
    QWidget(parent), m_pLayout(new FlowLayout)
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
            m_pTagModel.data(), &TagModel::notifyAllTagsListed, this,
            &NoteTagsWidget::onAllTagsListed);
    }

    m_pTagModel = QPointer<TagModel>(pTagModel);
}

void NoteTagsWidget::setCurrentNoteAndNotebook(
    const qevercloud::Note & note, const qevercloud::Notebook & notebook)
{
    QNTRACE(
        "widget:note_tags",
        "NoteTagsWidget::setCurrentNoteAndNotebook: "
            << "note local id = " << note << ", notebook: " << notebook);

    bool changed = (note.localId() != m_currentNote.localId());
    if (!changed) {
        QNDEBUG(
            "widget:note_tags",
            "The note is the same as the current one "
                << "already, checking whether the tag information has changed");

        changed |= (note.tagLocalIds() != m_currentNote.tagLocalIds());
        changed |= (note.tagGuids() != m_currentNote.tagGuids());

        if (!changed) {
            QNDEBUG("widget:note_tags", "Tag info hasn't changed");
        }
        else {
            clear();
        }

        m_currentNote = note; // Accepting the update just in case
    }
    else {
        clear();

        if (Q_UNLIKELY(note.localId().isEmpty())) {
            QNWARNING(
                "widget:note_tags",
                "Skipping the note with empty local "
                    << "uid");
            return;
        }

        m_currentNote = note;
    }

    changed |= (m_currentNotebookLocalId != notebook.localId());

    QString linkedNotebookGuid;
    if (notebook.linkedNotebookGuid()) {
        linkedNotebookGuid = *notebook.linkedNotebookGuid();
    }

    changed |= (m_currentLinkedNotebookGuid != linkedNotebookGuid);

    m_currentNotebookLocalId = notebook.localId();
    m_currentLinkedNotebookGuid = linkedNotebookGuid;

    bool couldUpdateNote = m_tagRestrictions.m_canUpdateNote;
    bool couldUpdateTags = m_tagRestrictions.m_canUpdateTags;

    if (notebook.restrictions()) {
        const auto & restrictions = *notebook.restrictions();

        m_tagRestrictions.m_canUpdateNote =
            !(restrictions.noCreateTags() && *restrictions.noCreateTags());

        m_tagRestrictions.m_canUpdateTags =
            !(restrictions.noUpdateTags() && *restrictions.noUpdateTags());
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
    m_currentNote = qevercloud::Note{};
    m_currentNote.setLocalId(QString{});

    m_lastDisplayedTagLocalIds.clear();

    m_currentNotebookLocalId.clear();
    m_currentLinkedNotebookGuid.clear();
    m_currentNoteTagLocalIdToNameBimap.clear();
    m_tagRestrictions = Restrictions();
}

void NoteTagsWidget::onTagRemoved(QString tagName) // NOLINT
{
    QNTRACE(
        "widget:note_tags",
        "NoteTagsWidget::onTagRemoved: tag name = " << tagName);

    if (Q_UNLIKELY(m_currentNote.localId().isEmpty())) {
        QNDEBUG(
            "widget:note_tags",
            "No current note is set, ignoring the tag "
                << "removal event");
        return;
    }

    const auto tagNameIt =
        m_currentNoteTagLocalIdToNameBimap.right.find(tagName);

    if (Q_UNLIKELY(
            tagNameIt == m_currentNoteTagLocalIdToNameBimap.right.end())) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't determine the tag which has been removed from "
                       "the note"));
        QNWARNING("widget:note_tags", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    const QString tagLocalId = tagNameIt->second;

    QNTRACE(
        "widget:note_tags", "Local uid of the removed tag: " << tagLocalId);

    const auto * pModelItem = m_pTagModel->itemForLocalId(tagLocalId);
    if (Q_UNLIKELY(!pModelItem)) {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't find the tag model item attempted to be removed "
                       "from the note"));

        QNWARNING(
            "widget:note_tags",
            errorDescription << ", tag local id = " << tagLocalId);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    const auto * pTagItem = pModelItem->cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem)) {
        ErrorString errorDescription(
            QT_TR_NOOP("Internal error: the tag model item found by tag local "
                       "uid is not of a tag type"));

        QNWARNING(
            "widget:note_tags",
            errorDescription << ", tag local id = " << tagLocalId
                             << ", tag model item: " << *pModelItem);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    m_currentNote.mutableTagLocalIds().removeAll(tagLocalId);

    const QString & tagGuid = pTagItem->guid();
    if (!tagGuid.isEmpty() && m_currentNote.tagGuids()) {
        m_currentNote.mutableTagGuids()->removeAll(tagGuid);
    }

    QNTRACE(
        "widget:note_tags",
        "Emitting note tags list changed signal: tag "
            << "removed: local id = " << tagLocalId << ", guid = " << tagGuid
            << "; note local id = " << m_currentNote.localId());

    Q_EMIT noteTagsListChanged(m_currentNote);

    for (int i = 0, size = m_pLayout->count(); i < size; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            QNWARNING(
                "widget:note_tags",
                "Detected null item within "
                    << "the layout");
            continue;
        }

        auto * pTagItemWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (Q_UNLIKELY(!pTagItemWidget)) {
            continue;
        }

        const QString tagName = pTagItemWidget->name();

        const auto lit = m_currentNoteTagLocalIdToNameBimap.right.find(tagName);
        if (Q_UNLIKELY(lit == m_currentNoteTagLocalIdToNameBimap.right.end()))
        {
            QNWARNING(
                "widget:note_tags",
                "Found tag item widget which name doesn't correspond to any "
                    << "registered local id, tag item name = " << tagName);

            continue;
        }

        const QString & currentTagLocalId = lit->second;
        if (currentTagLocalId != tagLocalId) {
            continue;
        }

        auto * pNewItemLineEdit = findNewItemWidget();
        if (pNewItemLineEdit) {
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

    m_lastDisplayedTagLocalIds.removeOne(tagLocalId);

    const auto lit = m_currentNoteTagLocalIdToNameBimap.left.find(tagLocalId);
    if (lit != m_currentNoteTagLocalIdToNameBimap.left.end()) {
        Q_UNUSED(m_currentNoteTagLocalIdToNameBimap.left.erase(lit))
    }
}

void NoteTagsWidget::onNewTagNameEntered()
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::onNewTagNameEntered");

    auto * pNewItemLineEdit = qobject_cast<NewListItemLineEdit *>(sender());
    if (Q_UNLIKELY(!pNewItemLineEdit)) {
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

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        ErrorString error(
            QT_TR_NOOP("Can't process the addition of a new tag: the tag model "
                       "is null"));

        QNWARNING("widget:note_tags", error);
        Q_EMIT notifyError(error);
        return;
    }

    if (Q_UNLIKELY(m_currentNote.localId().isEmpty())) {
        QNDEBUG(
            "widget:note_tags",
            "No current note is set, ignoring the tag "
                << "addition event");
        return;
    }

    auto tagIndex =
        m_pTagModel->indexForTagName(newTagName, m_currentLinkedNotebookGuid);

    if (!tagIndex.isValid()) {
        QNDEBUG(
            "widget:note_tags",
            "The tag with such name doesn't exist, "
                << "adding it");

        ErrorString errorDescription;

        tagIndex = m_pTagModel->createTag(
            newTagName, /* parent tag name = */ QString(),
            m_currentLinkedNotebookGuid, errorDescription);

        if (Q_UNLIKELY(!tagIndex.isValid())) {
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
    if (Q_UNLIKELY(!pModelItem)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't process the addition of a new "
                       "tag: can't find the tag model item for index within "
                       "the tag model"));
        QNWARNING("widget:note_tags", error);
        Q_EMIT notifyError(error);
        return;
    }

    const auto * pTagItem = pModelItem->cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: the tag model item found by tag name "
                       "is not of a tag type"));

        QNWARNING(
            "widget:note_tags", error << ", tag model item: " << *pModelItem);

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

    const QString & tagLocalId = pTagItem->localId();
    const auto & tagLocalIds = m_currentNote.tagLocalIds();
    if (tagLocalIds.contains(tagLocalId)) {
        QNDEBUG(
            "widget:note_tags",
            "Current note already contains tag local id " << tagLocalId);
        return;
    }

    m_currentNote.mutableTagLocalIds().push_back(tagLocalId);

    const QString & tagGuid = pTagItem->guid();
    if (!tagGuid.isEmpty()) {
        if (m_currentNote.tagGuids()) {
            m_currentNote.mutableTagGuids()->push_back(tagGuid);
        }
        else {
            m_currentNote.setTagGuids(QList<qevercloud::Guid>{} << tagGuid);
        }
    }

    QNTRACE(
        "widget:note_tags",
        "Emitting note tags list changed signal: tag "
            << "added: local id = " << tagLocalId << ", guid = " << tagGuid
            << "; note local id = " << m_currentNote.localId());

    Q_EMIT noteTagsListChanged(m_currentNote);

    const QString & tagName = pTagItem->name();

    m_lastDisplayedTagLocalIds << tagLocalId;
    m_currentNoteTagLocalIdToNameBimap.insert(
        TagLocalIdToNameBimap::value_type(tagLocalId, tagName));

    bool newItemLineEditHadFocus = false;

    NewListItemLineEdit::ItemInfo reservedItemInfo;
    reservedItemInfo.m_name = tagName;
    reservedItemInfo.m_linkedNotebookGuid = pTagItem->linkedNotebookGuid();

    if (!reservedItemInfo.m_linkedNotebookGuid.isEmpty()) {
        auto linkedNotebooksInfo = m_pTagModel->linkedNotebooksInfo();
        for (const auto & linkedNotebookInfo: qAsConst(linkedNotebooksInfo)) {
            if (linkedNotebookInfo.m_guid ==
                reservedItemInfo.m_linkedNotebookGuid) {
                reservedItemInfo.m_linkedNotebookUsername =
                    linkedNotebookInfo.m_username;

                break;
            }
        }
    }

    pNewItemLineEdit->addReservedItem(std::move(reservedItemInfo));

    newItemLineEditHadFocus = pNewItemLineEdit->hasFocus();
    Q_UNUSED(m_pLayout->removeWidget(pNewItemLineEdit))

    auto * pTagWidget = new ListItemWidget(tagName, tagLocalId, this);
    pTagWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    pTagWidget->setItemRemovable(m_tagRestrictions.m_canUpdateNote);

    QObject::connect(
        pTagWidget, &ListItemWidget::itemRemovedFromList, this,
        &NoteTagsWidget::onTagRemoved);

    QObject::connect(
        this, &NoteTagsWidget::canUpdateNoteRestrictionChanged, pTagWidget,
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
        m_pTagModel.data(), &TagModel::notifyAllTagsListed, this,
        &NoteTagsWidget::onAllTagsListed);

    updateLayout();
}

void NoteTagsWidget::onUpdateNoteComplete(
    qevercloud::Note note, // NOLINT
    LocalStorageManager::UpdateNoteOptions options, QUuid requestId)
{
    Q_UNUSED(requestId)

    if (!(options & LocalStorageManager::UpdateNoteOption::UpdateTags)) {
        return;
    }

    if (note.localId() != m_currentNote.localId()) {
        return;
    }

    QNTRACE(
        "widget:note_tags",
        "NoteTagsWidget::onUpdateNoteComplete: "
            << "note local id = " << note.localId()
            << ", request id = " << requestId);

    if ((options &
         LocalStorageManager::UpdateNoteOption::UpdateResourceMetadata) &&
        (options &
         LocalStorageManager::UpdateNoteOption::UpdateResourceBinaryData))
    {
        m_currentNote = note;
    }
    else {
        auto resources = m_currentNote.resources();
        m_currentNote = note;
        m_currentNote.setResources(resources);
    }

    updateLayout();
}

void NoteTagsWidget::onExpungeNoteComplete(
    qevercloud::Note note, QUuid requestId) // NOLINT
{
    if (note.localId() != m_currentNote.localId()) {
        return;
    }

    QNDEBUG(
        "widget:note_tags",
        "NoteTagsWidget::onExpungeNoteComplete: "
            << "note local id = " << note.localId()
            << ", request id = " << requestId);

    clear();
    clearLayout();
}

void NoteTagsWidget::onUpdateNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
{
    Q_UNUSED(requestId)

    if (notebook.localId() != m_currentNotebookLocalId) {
        return;
    }

    QNTRACE(
        "widget:note_tags",
        "NoteTagsWidget::onUpdateNotebookComplete: "
            << "notebook = " << notebook << "\nRequest id = " << requestId);

    bool changed = false;
    bool canUpdateNote = false;
    bool canUpdateTags = false;

    if (notebook.restrictions()) {
        const auto & restrictions = *notebook.restrictions();

        canUpdateNote =
            !(restrictions.noCreateTags() && *restrictions.noCreateTags());

        canUpdateTags =
            !(restrictions.noUpdateTags() && *restrictions.noUpdateTags());
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
        QNTRACE(
            "widget:note_tags",
            "No notebook restrictions were changed, "
                << "nothing to do");
        return;
    }

    Q_EMIT canUpdateNoteRestrictionChanged(m_tagRestrictions.m_canUpdateNote);

    if (!m_tagRestrictions.m_canUpdateNote) {
        removeNewTagWidgetFromLayout();
    }
}

void NoteTagsWidget::onExpungeNotebookComplete(
    qevercloud::Notebook notebook, QUuid requestId) // NOLINT
{
    Q_UNUSED(requestId)

    if (notebook.localId() != m_currentNotebookLocalId) {
        return;
    }

    QNTRACE(
        "widget:note_tags",
        "NoteTagsWidget::onExpungeNotebookComplete: "
            << "notebook = " << notebook << "\nRequest id = " << requestId);

    clear();
    clearLayout();
}

void NoteTagsWidget::onUpdateTagComplete(
    qevercloud::Tag tag, QUuid requestId) // NOLINT
{
    Q_UNUSED(requestId)

    const int tagIndex = m_lastDisplayedTagLocalIds.indexOf(tag.localId());
    if (tagIndex < 0) {
        return;
    }

    QNDEBUG(
        "widget:note_tags",
        "NoteTagsWidget::onUpdateTagComplete: tag = "
            << tag << "\nRequest id = " << requestId);

    const auto previousNameIt =
        m_currentNoteTagLocalIdToNameBimap.left.find(tag.localId());

    if (Q_UNLIKELY(
            previousNameIt == m_currentNoteTagLocalIdToNameBimap.left.end()))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Detected the update of a tag, however, its previous "
                       "name cannot be found"));
        QNWARNING("widget:note_tags", errorDescription << ", tag = " << tag);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    QString tagName = (tag.name() ? *tag.name() : QString{});
    QString previousName = previousNameIt->second;
    if (tag.name() && (previousName == tagName)) {
        QNDEBUG(
            "widget:note_tags",
            "The tag's name hasn't changed, nothing to do");
        return;
    }

    m_currentNoteTagLocalIdToNameBimap.left.replace_data(
        previousNameIt, tagName);

    // Need to find the note tag widget responsible for this tag and to change
    // its displayed name
    const int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNoteTagWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (!pNoteTagWidget) {
            continue;
        }

        if (pNoteTagWidget->name() != previousName) {
            continue;
        }

        if (!tag.name()) {
            QNDEBUG(
                "widget:note_tags",
                "Detected the update of tag not having any name... Strange "
                    << "enough, will just remove that tag's widget");

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

void NoteTagsWidget::onExpungeTagComplete(
    qevercloud::Tag tag, QStringList expungedChildTagLocalIds, // NOLINT
    QUuid requestId)
{
    QNTRACE(
        "widget:note_tags",
        "NoteTagsWidget::onExpungeTagComplete: tag = "
            << tag << "\nRequest id = " << requestId);

    Q_UNUSED(requestId)

    QStringList expungedTagLocalIds;
    expungedTagLocalIds << tag.localId();
    expungedTagLocalIds << expungedChildTagLocalIds;

    for (const auto & tagLocalId: qAsConst(expungedTagLocalIds)) {
        removeTagWidgetFromLayout(tagLocalId);
    }
}

void NoteTagsWidget::clearLayout(const bool skipNewTagWidget)
{
    while (m_pLayout->count() > 0) {
        auto * pItem = m_pLayout->takeAt(0);
        pItem->widget()->hide();
        pItem->widget()->deleteLater();
    }

    m_lastDisplayedTagLocalIds.clear();
    m_currentNoteTagLocalIdToNameBimap.clear();

    addTagIconToLayout();

    if (skipNewTagWidget || m_currentNote.localId().isEmpty() ||
        m_currentNotebookLocalId.isEmpty() ||
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

    const QString & noteLocalId = m_currentNote.localId();
    if (Q_UNLIKELY(noteLocalId.isEmpty())) {
        QNTRACE("widget:note_tags", "Note local id is empty, nothing to do");
        return;
    }

    if (m_currentNote.tagLocalIds().isEmpty()) {
        if (m_lastDisplayedTagLocalIds.isEmpty()) {
            QNTRACE(
                "widget:note_tags",
                "Note tags are still empty, nothing to do");
        }
        else {
            QNTRACE(
                "widget:note_tags",
                "The last tag has been removed from the note");
        }

        clearLayout();
        return;
    }

    bool shouldUpdateLayout = false;

    const QStringList & tagLocalIds = m_currentNote.tagLocalIds();
    const int numTags = tagLocalIds.size();
    if (numTags == m_lastDisplayedTagLocalIds.size()) {
        for (int i = 0, size = tagLocalIds.size(); i < size; ++i) {
            const int index =
                m_lastDisplayedTagLocalIds.indexOf(tagLocalIds[i]);

            if (index < 0) {
                shouldUpdateLayout = true;
                break;
            }
        }
    }
    else {
        shouldUpdateLayout = true;
    }

    if (!shouldUpdateLayout) {
        QNTRACE(
            "widget:note_tags",
            "Note's tag local ids haven't changed, no "
                << "need to update the layout");
        return;
    }

    clearLayout(/* skip new tag widget = */ true);

    if (!m_pTagModel->allTagsListed()) {
        QNDEBUG(
            "widget:note_tags",
            "Not all tags have been listed within the tag model yet");

        QObject::connect(
            m_pTagModel.data(), &TagModel::notifyAllTagsListed, this,
            &NoteTagsWidget::onAllTagsListed, Qt::UniqueConnection);

        return;
    }

    m_lastDisplayedTagLocalIds.reserve(numTags);

    QStringList tagNames;
    tagNames.reserve(numTags);

    for (int i = 0; i < numTags; ++i) {
        const QString & tagLocalId = tagLocalIds[i];

        const auto * pModelItem = m_pTagModel->itemForLocalId(tagLocalId);
        if (Q_UNLIKELY(!pModelItem)) {
            QNWARNING(
                "widget:note_tags",
                "Can't find tag model item for tag "
                    << "with local id " << tagLocalId);
            continue;
        }

        const auto * pTagItem = pModelItem->cast<TagItem>();
        if (Q_UNLIKELY(!pTagItem)) {
            QNWARNING(
                "widget:note_tags",
                "Skipping the tag model item found "
                    << "by tag local id yet containing no actual tag item: "
                    << *pModelItem);
            continue;
        }

        m_lastDisplayedTagLocalIds << tagLocalId;

        const QString & tagName = pTagItem->name();
        if (Q_UNLIKELY(tagName.isEmpty())) {
            QNDEBUG(
                "widget:note_tags",
                "Skipping the tag with empty name, local id = " << tagLocalId);
            continue;
        }

        m_currentNoteTagLocalIdToNameBimap.insert(
            TagLocalIdToNameBimap::value_type(tagLocalId, tagName));

        tagNames << tagName;
    }

    for (int i = 0, size = tagNames.size(); i < size; ++i) {
        const QString & tagName = tagNames[i];
        const QString & tagLocalId = tagLocalIds[i];

        auto * pTagWidget = new ListItemWidget(tagName, tagLocalId, this);
        pTagWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        pTagWidget->setItemRemovable(m_tagRestrictions.m_canUpdateNote);

        QObject::connect(
            pTagWidget, &ListItemWidget::itemRemovedFromList, this,
            &NoteTagsWidget::onTagRemoved);

        QObject::connect(
            this, &NoteTagsWidget::canUpdateNoteRestrictionChanged, pTagWidget,
            &ListItemWidget::setItemRemovable);

        m_pLayout->addWidget(pTagWidget);
    }

    if (Q_LIKELY(
            (tagNames.size() < m_pTagModel->account().noteTagCountMax()) &&
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
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNewItemWidget =
            qobject_cast<NewListItemLineEdit *>(pItem->widget());

        if (!pNewItemWidget) {
            continue;
        }

        pItem = m_pLayout->takeAt(i);
        pItem->widget()->hide();
        pItem->widget()->deleteLater();
        break;
    }

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        QNWARNING(
            "widget:note_tags",
            "No tag model is set, won't add the new tag widget");
        return;
    }

    if (!m_pTagModel->allTagsListed()) {
        QNDEBUG(
            "widget:note_tags",
            "Not all tags have been listed within the tag model yet");

        QObject::connect(
            m_pTagModel.data(), &TagModel::notifyAllTagsListed, this,
            &NoteTagsWidget::onAllTagsListed, Qt::UniqueConnection);

        return;
    }

    QVector<NewListItemLineEdit::ItemInfo> reservedItems;

    reservedItems.reserve(
        static_cast<int>(m_currentNoteTagLocalIdToNameBimap.size()));

    for (auto it = m_currentNoteTagLocalIdToNameBimap.right.begin(),
              end = m_currentNoteTagLocalIdToNameBimap.right.end();
         it != end; ++it)
    {
        NewListItemLineEdit::ItemInfo item;
        item.m_name = it->first;
        item.m_linkedNotebookGuid = m_currentLinkedNotebookGuid;

        if (!item.m_linkedNotebookGuid.isEmpty()) {
            const auto linkedNotebooksInfo = m_pTagModel->linkedNotebooksInfo();
            for (const auto & linkedNotebookInfo: qAsConst(linkedNotebooksInfo))
            {
                if (linkedNotebookInfo.m_guid == item.m_linkedNotebookGuid) {
                    item.m_linkedNotebookUsername =
                        linkedNotebookInfo.m_username;

                    break;
                }
            }
        }
    }

    auto * pNewTagLineEdit =
        new NewListItemLineEdit(m_pTagModel, reservedItems, this);

    pNewTagLineEdit->setTargetLinkedNotebookGuid(
        m_currentLinkedNotebookGuid.isEmpty() ? QLatin1String("")
                                              : m_currentLinkedNotebookGuid);

    QObject::connect(
        pNewTagLineEdit, &NewListItemLineEdit::returnPressed, this,
        &NoteTagsWidget::onNewTagNameEntered);

    QObject::connect(
        pNewTagLineEdit, &NewListItemLineEdit::receivedFocusFromWindowSystem,
        this, &NoteTagsWidget::newTagLineEditReceivedFocusFromWindowSystem);

    m_pLayout->addWidget(pNewTagLineEdit);
}

void NoteTagsWidget::removeNewTagWidgetFromLayout()
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::removeNewTagWidgetFromLayout");

    const int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNewTagLineEdit =
            qobject_cast<NewListItemLineEdit *>(pItem->widget());

        if (!pNewTagLineEdit) {
            continue;
        }

        pItem = m_pLayout->takeAt(i);
        delete pItem->widget();
        delete pItem;
        break;
    }
}

void NoteTagsWidget::removeTagWidgetFromLayout(const QString & tagLocalId)
{
    QNTRACE(
        "widget:note_tags",
        "NoteTagsWidget::removeTagWidgetFromLayout: "
            << "tag local id = " << tagLocalId);

    const int tagIndex = m_lastDisplayedTagLocalIds.indexOf(tagLocalId);
    if (tagIndex < 0) {
        QNDEBUG(
            "widget:note_tags",
            "This tag local id is not listed within the ones displayed");
        return;
    }

    m_lastDisplayedTagLocalIds.removeAt(tagIndex);

    QString tagName;

    const auto it = m_currentNoteTagLocalIdToNameBimap.left.find(tagLocalId);
    if (Q_UNLIKELY(it == m_currentNoteTagLocalIdToNameBimap.left.end())) {
        ErrorString errorDescription(
            QT_TR_NOOP("Detected the expunge of a tag, however, its name "
                       "cannot be found"));

        QNWARNING(
            "widget:note_tags",
            errorDescription << ", tag local id = " << tagLocalId);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    tagName = it->second;

    // Need to find the note tag widget responsible for this tag and remove it
    // from the layout
    const int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNoteTagWidget = qobject_cast<ListItemWidget *>(pItem->widget());
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
    QNTRACE(
        "widget:note_tags",
        "NoteTagsWidget::setTagItemsRemovable: "
            << "removable = " << (removable ? "true" : "false"));

    const int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNoteTagWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (!pNoteTagWidget) {
            continue;
        }

        pNoteTagWidget->setItemRemovable(removable);
    }
}

void NoteTagsWidget::createConnections( // NOLINT
    LocalStorageManagerAsync & localStorageManagerAsync)
{
    QNTRACE("widget:note_tags", "NoteTagsWidget::createConnections");

    // Connect local storage signals to local slots
    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNoteComplete, this,
        &NoteTagsWidget::onUpdateNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNoteComplete, this,
        &NoteTagsWidget::onExpungeNoteComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::updateNotebookComplete, this,
        &NoteTagsWidget::onUpdateNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeNotebookComplete, this,
        &NoteTagsWidget::onExpungeNotebookComplete);

    QObject::connect(
        &localStorageManagerAsync, &LocalStorageManagerAsync::updateTagComplete,
        this, &NoteTagsWidget::onUpdateTagComplete);

    QObject::connect(
        &localStorageManagerAsync,
        &LocalStorageManagerAsync::expungeTagComplete, this,
        &NoteTagsWidget::onExpungeTagComplete);
}

NewListItemLineEdit * NoteTagsWidget::findNewItemWidget()
{
    const int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNewItemWidget =
            qobject_cast<NewListItemLineEdit *>(pItem->widget());

        if (!pNewItemWidget) {
            continue;
        }

        return pNewItemWidget;
    }

    return nullptr;
}

bool NoteTagsWidget::isActive() const noexcept
{
    return !m_currentNote.localId().isEmpty();
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
        static_cast<int>(m_currentNoteTagLocalIdToNameBimap.size()));

    for (auto it = m_currentNoteTagLocalIdToNameBimap.right.begin(),
              end = m_currentNoteTagLocalIdToNameBimap.right.end();
         it != end; ++it)
    {
        result << it->first;
        QNTRACE("widget:note_tags", "Added tag name " << result.back());
    }

    return result;
}

} // namespace quentier
