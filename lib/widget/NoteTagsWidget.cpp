/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#include <quentier/local_storage/ILocalStorage.h>
#include <quentier/local_storage/ILocalStorageNotifier.h>
#include <quentier/logging/QuentierLogger.h>

#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QList>

#include <utility>

namespace quentier {

NoteTagsWidget::NoteTagsWidget(QWidget * parent) :
    QWidget{parent}, m_layout{new FlowLayout}
{
    addTagIconToLayout();
    setLayout(m_layout);
}

NoteTagsWidget::~NoteTagsWidget() = default;

void NoteTagsWidget::setLocalStorage(
    const local_storage::ILocalStorage & localStorage)
{
    connectToLocalStorageEvents(localStorage.notifier());
}

void NoteTagsWidget::setTagModel(TagModel * tagModel)
{
    if (!m_tagModel.isNull()) {
        QObject::disconnect(
            m_tagModel.data(), &TagModel::notifyAllTagsListed, this,
            &NoteTagsWidget::onAllTagsListed);
    }

    m_tagModel = QPointer<TagModel>(tagModel);
}

void NoteTagsWidget::setCurrentNoteAndNotebook(
    const qevercloud::Note & note, const qevercloud::Notebook & notebook)
{
    QNTRACE(
        "widget::NoteTagsWidget",
        "NoteTagsWidget::setCurrentNoteAndNotebook: "
            << "note local id = " << note.localId()
            << ", notebook: " << notebook);

    bool changed = false;
    if (!m_currentNote) {
        changed = true;
    }

    if (!changed) {
        changed = (note.localId() != m_currentNote->localId());
    }

    if (!changed) {
        QNDEBUG(
            "widget::NoteTagsWidget",
            "The note is the same as the current one already, checking whether "
            "tag information changed");

        changed |= (note.tagLocalIds() != m_currentNote->tagLocalIds());
        changed |= (note.tagGuids() != m_currentNote->tagGuids());

        if (!changed) {
            QNDEBUG("widget::NoteTagsWidget", "Tag info hasn't changed");
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
                "widget::NoteTagsWidget",
                "Skipping the note with empty local id");
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

    const bool couldUpdateNote = m_tagRestrictions.m_canUpdateNote;
    const bool couldUpdateTags = m_tagRestrictions.m_canUpdateTags;

    if (notebook.restrictions()) {
        const auto & restrictions = *notebook.restrictions();

        m_tagRestrictions.m_canUpdateNote =
            !restrictions.noUpdateNotes().value_or(false);

        m_tagRestrictions.m_canUpdateTags =
            !restrictions.noUpdateTags().value_or(false);
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
    m_currentNote.reset();
    m_lastDisplayedTagLocalIds.clear();

    m_currentNotebookLocalId.clear();
    m_currentLinkedNotebookGuid.clear();
    m_currentNoteTagLocalIdToNameBimap.clear();
    m_tagRestrictions = Restrictions();
}

void NoteTagsWidget::onTagRemoved(
    QString tagLocalId, QString tagName, QString linkedNotebookGuid,
    QString linkedNotebookUsername)
{
    QNTRACE(
        "widget::NoteTagsWidget",
        "NoteTagsWidget::onTagRemoved: tag localId = "
            << tagLocalId << ", tag name = " << tagName
            << ", linkedNotebookGuid = " << linkedNotebookGuid
            << ", linkedNotebookUsername = " << linkedNotebookUsername);

    if (Q_UNLIKELY(!m_currentNote)) {
        QNDEBUG(
            "widget::NoteTagsWidget",
            "No current note is set, ignoring tag removal event");
        return;
    }

    const auto tagLocalIdIt =
        m_currentNoteTagLocalIdToNameBimap.left.find(tagLocalId);
    if (Q_UNLIKELY(
            tagLocalIdIt == m_currentNoteTagLocalIdToNameBimap.left.end()))
    {
        ErrorString errorDescription(
            QT_TR_NOOP("Can't determine the tag which has been removed from "
                       "the note"));
        QNWARNING("widget::NoteTagsWidget", errorDescription);
        Q_EMIT notifyError(errorDescription);
        return;
    }

    QNTRACE(
        "widget::NoteTagsWidget",
        "Local id of the removed tag: " << tagLocalId);

    const auto * modelItem = m_tagModel->itemForLocalId(tagLocalId);
    if (Q_UNLIKELY(!modelItem)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Can't find the tag model item attempted to be removed "
                       "from the note")};

        QNWARNING(
            "widget::NoteTagsWidget",
            errorDescription << ", tag local id = " << tagLocalId);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    const TagItem * tagItem = modelItem->cast<TagItem>();
    if (Q_UNLIKELY(!tagItem)) {
        ErrorString errorDescription{
            QT_TR_NOOP("Internal error: the tag model item found by tag local "
                       "id is not of a tag type")};

        QNWARNING(
            "widget::NoteTagsWidget",
            errorDescription << ", tag local id = " << tagLocalId
                             << ", tag model item: " << *modelItem);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    m_currentNote->mutableTagLocalIds().removeAll(tagLocalId);

    const QString & tagGuid = tagItem->guid();
    if (!tagGuid.isEmpty() && m_currentNote->tagGuids()) {
        m_currentNote->mutableTagGuids()->removeAll(tagGuid);
    }

    QNTRACE(
        "widget::NoteTagsWidget",
        "Emitting note tags list changed signal: tag "
            << "removed: local id = " << tagLocalId << ", guid = " << tagGuid
            << "; note local id = " << m_currentNote->localId());

    Q_EMIT noteTagsListChanged(*m_currentNote);

    for (int i = 0, size = m_layout->count(); i < size; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            QNWARNING(
                "widget::NoteTagsWidget",
                "Detected null item within the layout");
            continue;
        }

        auto * tagItemWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (Q_UNLIKELY(!tagItemWidget)) {
            continue;
        }

        QString tagName = tagItemWidget->name();

        const auto lit = m_currentNoteTagLocalIdToNameBimap.right.find(tagName);
        if (Q_UNLIKELY(lit == m_currentNoteTagLocalIdToNameBimap.right.end())) {
            QNWARNING(
                "widget::NoteTagsWidget",
                "Found tag item widget which name doesn't correspond to any "
                    << "registered local id, tag item name = " << tagName);

            continue;
        }

        const QString & currentTagLocalId = lit->second;
        if (currentTagLocalId != tagLocalId) {
            continue;
        }

        auto * newItemLineEdit = findNewItemWidget();
        if (newItemLineEdit) {
            NewListItemLineEdit::ItemInfo removedItemInfo;
            removedItemInfo.m_name = std::move(tagName);

            removedItemInfo.m_linkedNotebookGuid =
                tagItemWidget->linkedNotebookGuid();

            removedItemInfo.m_linkedNotebookUsername =
                tagItemWidget->linkedNotebookUsername();

            newItemLineEdit->removeReservedItem(removedItemInfo);
        }

        Q_UNUSED(m_layout->takeAt(i));
        tagItemWidget->hide();
        tagItemWidget->deleteLater();
        break;
    }

    m_lastDisplayedTagLocalIds.removeOne(tagLocalId);
    m_currentNoteTagLocalIdToNameBimap.left.erase(tagLocalIdIt);
}

void NoteTagsWidget::onNewTagNameEntered()
{
    QNTRACE("widget::NoteTagsWidget", "NoteTagsWidget::onNewTagNameEntered");

    auto * newItemLineEdit = qobject_cast<NewListItemLineEdit *>(sender());
    if (Q_UNLIKELY(!newItemLineEdit)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't process the addition of a new "
                       "tag: can't cast the signal sender to NewListLineEdit")};

        QNWARNING("widget::NoteTagsWidget", error);
        Q_EMIT notifyError(error);
        return;
    }

    QString newTagName = newItemLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(newTagName);

    QNDEBUG("widget::NoteTagsWidget", "New tag name: " << newTagName);

    if (newTagName.isEmpty()) {
        return;
    }

    newItemLineEdit->setText(QString{});
    if (!newItemLineEdit->hasFocus()) {
        newItemLineEdit->setFocus();
    }

    if (Q_UNLIKELY(newTagName.isEmpty())) {
        QNDEBUG(
            "widget::NoteTagsWidget", "New note's tag name is empty, skipping");
        return;
    }

    if (Q_UNLIKELY(m_tagModel.isNull())) {
        ErrorString error{
            QT_TR_NOOP("Can't process the addition of a new tag: the tag model "
                       "is null")};

        QNWARNING("widget::NoteTagsWidget", error);
        Q_EMIT notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!m_currentNote)) {
        QNDEBUG(
            "widget::NoteTagsWidget",
            "No current note is set, ignoring the tag addition event");
        return;
    }

    auto tagIndex =
        m_tagModel->indexForTagName(newTagName, m_currentLinkedNotebookGuid);

    if (!tagIndex.isValid()) {
        QNDEBUG(
            "widget::NoteTagsWidget",
            "The tag with such name doesn't exist, adding it");

        ErrorString errorDescription;

        tagIndex = m_tagModel->createTag(
            newTagName, /* parent tag name = */ QString(),
            m_currentLinkedNotebookGuid, errorDescription);

        if (Q_UNLIKELY(!tagIndex.isValid())) {
            ErrorString error{QT_TR_NOOP("Can't add a new tag")};
            error.appendBase(errorDescription.base());
            error.appendBase(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            QNINFO("widget::NoteTagsWidget", error);
            Q_EMIT notifyError(error);
            return;
        }
    }

    const auto * modelItem = m_tagModel->itemForIndex(tagIndex);
    if (Q_UNLIKELY(!modelItem)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't process the addition of a new "
                       "tag: can't find the tag model item for index within "
                       "the tag model")};
        QNWARNING("widget::NoteTagsWidget", error);
        Q_EMIT notifyError(error);
        return;
    }

    const TagItem * tagItem = modelItem->cast<TagItem>();
    if (Q_UNLIKELY(!tagItem)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: the tag model item found by tag name "
                       "is not of a tag type")};

        QNWARNING(
            "widget::NoteTagsWidget",
            error << ", tag model item: " << *modelItem);

        Q_EMIT notifyError(error);
        return;
    }

    if (!m_currentLinkedNotebookGuid.isEmpty() &&
        (tagItem->linkedNotebookGuid() != m_currentLinkedNotebookGuid))
    {
        ErrorString error{
            QT_TR_NOOP("Can't link note from external (linked) notebook with "
                       "tag from your own account")};
        QNDEBUG("widget::NoteTagsWidget", error);
        Q_EMIT notifyError(error);
        return;
    }

    const QString & tagLocalId = tagItem->localId();
    if (m_currentNote->tagLocalIds().contains(tagLocalId)) {
        QNDEBUG(
            "widget::NoteTagsWidget",
            "Current note already contains tag local id " << tagLocalId);
        return;
    }

    m_currentNote->mutableTagLocalIds().append(tagLocalId);

    const QString & tagGuid = tagItem->guid();
    if (!tagGuid.isEmpty()) {
        if (!m_currentNote->tagGuids()) {
            m_currentNote->mutableTagGuids().emplace(
                QList<qevercloud::Guid>{tagGuid});
        }
        else {
            m_currentNote->mutableTagGuids()->append(tagGuid);
        }
    }

    QNTRACE(
        "widget::NoteTagsWidget",
        "Emitting note tags list changed signal: tag "
            << "added: local id = " << tagLocalId << ", guid = " << tagGuid
            << "; note local id = " << m_currentNote->localId());

    Q_EMIT noteTagsListChanged(*m_currentNote);

    const QString & tagName = tagItem->name();

    m_lastDisplayedTagLocalIds << tagLocalId;
    m_currentNoteTagLocalIdToNameBimap.insert(
        TagLocalIdToNameBimap::value_type(tagLocalId, tagName));

    bool newItemLineEditHadFocus = false;

    NewListItemLineEdit::ItemInfo reservedItemInfo;
    reservedItemInfo.m_name = tagName;
    reservedItemInfo.m_linkedNotebookGuid = tagItem->linkedNotebookGuid();

    if (!reservedItemInfo.m_linkedNotebookGuid.isEmpty()) {
        auto linkedNotebooksInfo = m_tagModel->linkedNotebooksInfo();
        for (const auto & linkedNotebookInfo:
             std::as_const(linkedNotebooksInfo))
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

    newItemLineEdit->addReservedItem(std::move(reservedItemInfo));

    newItemLineEditHadFocus = newItemLineEdit->hasFocus();
    Q_UNUSED(m_layout->removeWidget(newItemLineEdit))

    auto * tagWidget = new ListItemWidget(tagName, tagLocalId, this);
    tagWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    tagWidget->setItemRemovable(m_tagRestrictions.m_canUpdateNote);

    QObject::connect(
        tagWidget, &ListItemWidget::itemRemovedFromList, this,
        &NoteTagsWidget::onTagRemoved);

    QObject::connect(
        this, &NoteTagsWidget::canUpdateNoteRestrictionChanged, tagWidget,
        &ListItemWidget::setItemRemovable);

    m_layout->addWidget(tagWidget);

    m_layout->addWidget(newItemLineEdit);
    newItemLineEdit->setText(QString{});

    if (newItemLineEditHadFocus) {
        newItemLineEdit->setFocus();
    }
}

void NoteTagsWidget::onAllTagsListed()
{
    QNTRACE("widget::NoteTagsWidget", "NoteTagsWidget::onAllTagsListed");

    if (m_tagModel.isNull()) {
        return;
    }

    QObject::disconnect(
        m_tagModel.data(), &TagModel::notifyAllTagsListed, this,
        &NoteTagsWidget::onAllTagsListed);

    updateLayout();
}

void NoteTagsWidget::clearLayout(const bool skipNewTagWidget)
{
    while (m_layout->count() > 0) {
        auto * item = m_layout->takeAt(0);
        item->widget()->hide();
        item->widget()->deleteLater();
    }

    m_lastDisplayedTagLocalIds.clear();
    m_currentNoteTagLocalIdToNameBimap.clear();

    addTagIconToLayout();

    if (skipNewTagWidget || !m_currentNote ||
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
    QNTRACE("widget::NoteTagsWidget", "NoteTagsWidget::updateLayout");

    if (Q_UNLIKELY(!m_currentNote)) {
        QNTRACE("widget::NoteTagsWidget", "No note is set, nothing to do");
        return;
    }

    if (m_currentNote->tagLocalIds().isEmpty()) {
        if (m_lastDisplayedTagLocalIds.isEmpty()) {
            QNTRACE(
                "widget::NoteTagsWidget",
                "Note tags are still empty, nothing to do");
        }
        else {
            QNTRACE(
                "widget::NoteTagsWidget",
                "The last tag has been removed from the note");
        }

        clearLayout();
        return;
    }

    bool shouldUpdateLayout = false;

    const QStringList & tagLocalIds = m_currentNote->tagLocalIds();
    const int numTags = tagLocalIds.size();
    if (numTags == m_lastDisplayedTagLocalIds.size()) {
        for (int i = 0; i < numTags; ++i) {
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
            "widget::NoteTagsWidget",
            "Note's tag local ids haven't changed, no need to update the "
            "layout");
        return;
    }

    clearLayout(/* skip new tag widget = */ true);

    if (!m_tagModel->allTagsListed()) {
        QNDEBUG(
            "widget::NoteTagsWidget",
            "Not all tags have been listed within the tag model yet");

        QObject::connect(
            m_tagModel.data(), &TagModel::notifyAllTagsListed, this,
            &NoteTagsWidget::onAllTagsListed, Qt::UniqueConnection);

        return;
    }

    m_lastDisplayedTagLocalIds.reserve(numTags);

    QStringList tagNames;
    tagNames.reserve(numTags);

    for (int i = 0; i < numTags; ++i) {
        const QString & tagLocalId = tagLocalIds[i];

        const auto * modelItem = m_tagModel->itemForLocalId(tagLocalId);
        if (Q_UNLIKELY(!modelItem)) {
            QNWARNING(
                "widget::NoteTagsWidget",
                "Can't find tag model item for tag with local id "
                    << tagLocalId);
            continue;
        }

        const auto * tagItem = modelItem->cast<TagItem>();
        if (Q_UNLIKELY(!tagItem)) {
            QNWARNING(
                "widget::NoteTagsWidget",
                "Skipping the tag model item found by tag local id yet "
                    << "containing no actual tag item: " << *modelItem);
            continue;
        }

        m_lastDisplayedTagLocalIds << tagLocalId;

        const QString & tagName = tagItem->name();
        if (Q_UNLIKELY(tagName.isEmpty())) {
            QNDEBUG(
                "widget::NoteTagsWidget",
                "Skipping the tag with empty name, local id = " << tagLocalId);
            continue;
        }

        m_currentNoteTagLocalIdToNameBimap.insert(
            TagLocalIdToNameBimap::value_type{tagLocalId, tagName});

        tagNames << tagName;
    }

    for (int i = 0, size = tagNames.size(); i < size; ++i) {
        const QString & tagName = tagNames[i];
        const QString & tagLocalId = tagLocalIds[i];

        auto * tagWidget = new ListItemWidget(tagName, tagLocalId, this);
        tagWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        tagWidget->setItemRemovable(m_tagRestrictions.m_canUpdateNote);

        QObject::connect(
            tagWidget, &ListItemWidget::itemRemovedFromList, this,
            &NoteTagsWidget::onTagRemoved);

        QObject::connect(
            this, &NoteTagsWidget::canUpdateNoteRestrictionChanged, tagWidget,
            &ListItemWidget::setItemRemovable);

        m_layout->addWidget(tagWidget);
    }

    if (Q_LIKELY(
            tagNames.size() < m_tagModel->account().noteTagCountMax() &&
            m_tagRestrictions.m_canUpdateNote))
    {
        addNewTagWidgetToLayout();
    }
}

void NoteTagsWidget::addTagIconToLayout()
{
    QNTRACE("widget::NoteTagsWidget", "NoteTagsWidget::addTagIconToLayout");

    QPixmap tagIconImage{QStringLiteral(":/tag/tag.png")};
    auto * tagIconLabel = new QLabel(this);
    tagIconLabel->setPixmap(tagIconImage.scaled(QSize{20, 20}));
    tagIconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_layout->addWidget(tagIconLabel);
}

void NoteTagsWidget::addNewTagWidgetToLayout()
{
    QNTRACE(
        "widget::NoteTagsWidget", "NoteTagsWidget::addNewTagWidgetToLayout");

    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * newItemWidget =
            qobject_cast<NewListItemLineEdit *>(item->widget());

        if (!newItemWidget) {
            continue;
        }

        item = m_layout->takeAt(i);
        item->widget()->hide();
        item->widget()->deleteLater();
        break;
    }

    if (Q_UNLIKELY(m_tagModel.isNull())) {
        QNWARNING(
            "widget::NoteTagsWidget",
            "No tag model is set, won't add the new tag widget");
        return;
    }

    if (!m_tagModel->allTagsListed()) {
        QNDEBUG(
            "widget::NoteTagsWidget",
            "Not all tags have been listed within the tag model yet");

        QObject::connect(
            m_tagModel.data(), &TagModel::notifyAllTagsListed, this,
            &NoteTagsWidget::onAllTagsListed, Qt::UniqueConnection);

        return;
    }

    QList<NewListItemLineEdit::ItemInfo> reservedItems;

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
            auto linkedNotebooksInfo = m_tagModel->linkedNotebooksInfo();
            for (const auto & linkedNotebookInfo:
                 std::as_const(linkedNotebooksInfo))
            {
                if (linkedNotebookInfo.m_guid == item.m_linkedNotebookGuid) {
                    item.m_linkedNotebookUsername =
                        linkedNotebookInfo.m_username;
                    break;
                }
            }
        }
    }

    auto * newTagLineEdit =
        new NewListItemLineEdit(m_tagModel, reservedItems, this);

    newTagLineEdit->setTargetLinkedNotebookGuid(
        m_currentLinkedNotebookGuid.isEmpty() ? QLatin1String{""}
                                              : m_currentLinkedNotebookGuid);

    QObject::connect(
        newTagLineEdit, &NewListItemLineEdit::returnPressed, this,
        &NoteTagsWidget::onNewTagNameEntered);

    QObject::connect(
        newTagLineEdit, &NewListItemLineEdit::receivedFocusFromWindowSystem,
        this, &NoteTagsWidget::newTagLineEditReceivedFocusFromWindowSystem);

    m_layout->addWidget(newTagLineEdit);
}

void NoteTagsWidget::removeNewTagWidgetFromLayout()
{
    QNTRACE(
        "widget::NoteTagsWidget",
        "NoteTagsWidget::removeNewTagWidgetFromLayout");

    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * newTagLineEdit =
            qobject_cast<NewListItemLineEdit *>(item->widget());

        if (!newTagLineEdit) {
            continue;
        }

        item = m_layout->takeAt(i);
        delete item->widget();
        delete item;
        break;
    }
}

void NoteTagsWidget::removeTagWidgetFromLayout(const QString & tagLocalId)
{
    QNTRACE(
        "widget::NoteTagsWidget",
        "NoteTagsWidget::removeTagWidgetFromLayout: tag local id = "
            << tagLocalId);

    const int tagIndex = m_lastDisplayedTagLocalIds.indexOf(tagLocalId);
    if (tagIndex < 0) {
        QNDEBUG(
            "widget::NoteTagsWidget",
            "This tag local id is not listed within the ones displayed");
        return;
    }

    m_lastDisplayedTagLocalIds.removeAt(tagIndex);

    QString tagName;

    const auto it = m_currentNoteTagLocalIdToNameBimap.left.find(tagLocalId);
    if (Q_UNLIKELY(it == m_currentNoteTagLocalIdToNameBimap.left.end())) {
        ErrorString errorDescription{
            QT_TR_NOOP("Detected the expunge of a tag, however, its name "
                       "cannot be found")};

        QNWARNING(
            "widget::NoteTagsWidget",
            errorDescription << ", tag local id = " << tagLocalId);

        Q_EMIT notifyError(errorDescription);
        return;
    }

    tagName = it->second;

    // Need to find the note tag widget responsible for this tag and remove it
    // from the layout
    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * noteTagWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (!noteTagWidget) {
            continue;
        }

        if (noteTagWidget->name() != tagName) {
            continue;
        }

        item = m_layout->takeAt(i);
        item->widget()->hide();
        item->widget()->deleteLater();
        break;
    }
}

void NoteTagsWidget::setTagItemsRemovable(const bool removable)
{
    QNTRACE(
        "widget::NoteTagsWidget",
        "NoteTagsWidget::setTagItemsRemovable: "
            << "removable = " << (removable ? "true" : "false"));

    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * noteTagWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (!noteTagWidget) {
            continue;
        }

        noteTagWidget->setItemRemovable(removable);
    }
}

void NoteTagsWidget::connectToLocalStorageEvents(
    local_storage::ILocalStorageNotifier * notifier)
{
    QNDEBUG(
        "widget::NoteTagsWidget",
        "NoteTagsWidget::connectToLocalStorageEvents");

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteUpdated, this,
        [this](
            const qevercloud::Note & note,
            const local_storage::ILocalStorage::UpdateNoteOptions options) {
            if (!m_currentNote || m_currentNote->localId() != note.localId()) {
                return;
            }

            using UpdateNoteOption =
                local_storage::ILocalStorage::UpdateNoteOption;

            if (!options.testFlag(UpdateNoteOption::UpdateTags)) {
                return;
            }

            QNDEBUG(
                "widget::NoteTagsWidget",
                "Note updated with tags: note local id = "
                    << note.localId()
                    << ", tag local ids = " << note.tagLocalIds());

            if (options.testFlag(UpdateNoteOption::UpdateResourceMetadata) &&
                options.testFlag(UpdateNoteOption::UpdateResourceBinaryData))
            {
                m_currentNote = note;
            }
            else {
                auto resources = m_currentNote->resources();
                m_currentNote = note;
                m_currentNote->setResources(resources);
            }

            updateLayout();
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notePut, this,
        [this](const qevercloud::Note & note) {
            if (!m_currentNote || m_currentNote->localId() != note.localId()) {
                return;
            }

            QNDEBUG(
                "widget::NoteTagsWidget",
                "Note fully updated: note local id = " << note.localId()
                                                       << ", tag local ids = "
                                                       << note.tagLocalIds());

            m_currentNote = note;
            updateLayout();
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::noteExpunged, this,
        [this](const QString & noteLocalId) {
            if (!m_currentNote || m_currentNote->localId() != noteLocalId) {
                return;
            }

            QNDEBUG(
                "widget::NoteTagsWidget",
                "Note expunged: local id = " << noteLocalId);

            clear();
            clearLayout();
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookPut, this,
        [this](const qevercloud::Notebook & notebook) {
            if (notebook.localId() != m_currentNotebookLocalId) {
                return;
            }

            QNDEBUG("widget::NoteTagsWidget", "Notebook updated: " << notebook);

            bool changed = false;
            bool canUpdateNote = false;
            bool canUpdateTags = false;

            if (notebook.restrictions()) {
                const auto & restrictions = *notebook.restrictions();
                canUpdateNote = !restrictions.noCreateTags().value_or(false);
                canUpdateTags = !restrictions.noUpdateTags().value_or(false);
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
                QNDEBUG(
                    "widget::NoteTagsWidget",
                    "No notebook restrictions were changed, nothing to do");
                return;
            }

            Q_EMIT canUpdateNoteRestrictionChanged(
                m_tagRestrictions.m_canUpdateNote);

            if (!m_tagRestrictions.m_canUpdateNote) {
                removeNewTagWidgetFromLayout();
            }
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::notebookExpunged, this,
        [this](const QString & notebookLocalId) {
            if (notebookLocalId != m_currentNotebookLocalId) {
                return;
            }

            QNDEBUG(
                "widget::NoteTagsWidget",
                "Notebook expunged: notebook local id = " << notebookLocalId);

            clear();
            clearLayout();
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::tagPut, this,
        [this](const qevercloud::Tag & tag) {
            const int tagIndex =
                m_lastDisplayedTagLocalIds.indexOf(tag.localId());

            if (tagIndex < 0) {
                return;
            }

            QNDEBUG("widget::NoteTagsWidget", "Tag updated: " << tag);

            const auto previousNameIt =
                m_currentNoteTagLocalIdToNameBimap.left.find(tag.localId());

            if (Q_UNLIKELY(
                    previousNameIt ==
                    m_currentNoteTagLocalIdToNameBimap.left.end()))
            {
                ErrorString errorDescription{QT_TR_NOOP(
                    "Detected the update of a tag, however, its previous "
                    "name cannot be found")};
                QNWARNING(
                    "widget::NoteTagsWidget",
                    errorDescription << ", tag = " << tag);

                Q_EMIT notifyError(errorDescription);
                return;
            }

            const QString tagName = tag.name().value_or(QString{});
            const QString previousName = previousNameIt->second;
            if (tag.name() && (previousName == tagName)) {
                QNDEBUG(
                    "widget::NoteTagsWidget",
                    "The tag's name hasn't changed, nothing to do");
                return;
            }

            m_currentNoteTagLocalIdToNameBimap.left.replace_data(
                previousNameIt, tagName);

            // Need to find the note tag widget responsible for this tag and to
            // change its displayed name
            const int numItems = m_layout->count();
            for (int i = 0; i < numItems; ++i) {
                auto * item = m_layout->itemAt(i);
                if (Q_UNLIKELY(!item)) {
                    continue;
                }

                auto * noteTagWidget =
                    qobject_cast<ListItemWidget *>(item->widget());
                if (!noteTagWidget) {
                    continue;
                }

                if (noteTagWidget->name() != previousName) {
                    continue;
                }

                if (!tag.name()) {
                    QNDEBUG(
                        "widget::NoteTagsWidget",
                        "Detected the update of tag not having any name... "
                        "Strange enough, will just remove that tag's widget");

                    item = m_layout->takeAt(i);
                    item->widget()->hide();
                    item->widget()->deleteLater();
                }
                else {
                    noteTagWidget->setName(tagName);
                }

                break;
            }
        });

    QObject::connect(
        notifier, &local_storage::ILocalStorageNotifier::tagExpunged, this,
        [this](
            const QString & tagLocalId, const QStringList & childTagLocalIds) {
            QNDEBUG(
                "widget::NoteTagsWidget",
                "Tag expunged: "
                    << tagLocalId << ", child tag local ids: "
                    << childTagLocalIds.join(QStringLiteral(", ")));

            QStringList expungedTagLocalIds;
            expungedTagLocalIds << tagLocalId;
            expungedTagLocalIds << childTagLocalIds;

            for (const auto & tagLocalId: std::as_const(expungedTagLocalIds)) {
                removeTagWidgetFromLayout(tagLocalId);
            }
        });
}

NewListItemLineEdit * NoteTagsWidget::findNewItemWidget()
{
    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * newItemWidget =
            qobject_cast<NewListItemLineEdit *>(item->widget());

        if (!newItemWidget) {
            continue;
        }

        return newItemWidget;
    }

    return nullptr;
}

bool NoteTagsWidget::isActive() const noexcept
{
    return m_currentNote.has_value();
}

QStringList NoteTagsWidget::tagNames() const
{
    QNTRACE("widget::NoteTagsWidget", "NoteTagsWidget::tagNames");

    if (!isActive()) {
        QNDEBUG("widget::NoteTagsWidget", "NoteTagsWidget is not active");
        return {};
    }

    QStringList result;
    result.reserve(static_cast<int>(m_currentNoteTagLocalIdToNameBimap.size()));

    for (auto it = m_currentNoteTagLocalIdToNameBimap.right.begin(),
              end = m_currentNoteTagLocalIdToNameBimap.right.end();
         it != end; ++it)
    {
        result << it->first;
        QNTRACE("widget::NoteTagsWidget", "Added tag name " << result.back());
    }

    return result;
}

} // namespace quentier
