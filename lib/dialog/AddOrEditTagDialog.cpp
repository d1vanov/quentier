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

#include "AddOrEditTagDialog.h"

#include "ui_AddOrEditTagDialog.h"

#include <lib/preferences/keys/Files.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QPushButton>
#include <QStringListModel>

#include <algorithm>
#include <iterator>

#define LAST_SELECTED_PARENT_TAG_NAME_KEY                                      \
    QStringLiteral("_LastSelectedParentTagName")

namespace quentier {

AddOrEditTagDialog::AddOrEditTagDialog(
    TagModel * pTagModel, QWidget * parent, QString editedTagLocalId) :
    QDialog(parent),
    m_pUi(new Ui::AddOrEditTagDialog), m_pTagModel(pTagModel),
    m_editedTagLocalId(std::move(editedTagLocalId))
{
    m_pUi->setupUi(this);
    m_pUi->statusBar->setHidden(true);

    QStringList tagNames;
    int parentTagNameIndex = -1;
    const bool res = setupEditedTagItem(tagNames, parentTagNameIndex);
    if (!res && !m_pTagModel.isNull()) {
        tagNames = m_pTagModel->tagNames();
        tagNames.prepend(QLatin1String(""));
    }

    m_pTagNamesModel = new QStringListModel(this);
    m_pTagNamesModel->setStringList(tagNames);
    m_pUi->parentTagNameComboBox->setModel(m_pTagNamesModel);

    createConnections();

    if (res) {
        m_pUi->parentTagNameComboBox->setCurrentIndex(parentTagNameIndex);
    }
    else if (!tagNames.isEmpty() && !m_pTagModel.isNull()) {
        parentTagNameIndex = 0;
        m_pUi->parentTagNameComboBox->setCurrentIndex(parentTagNameIndex);

        ApplicationSettings appSettings(
            m_pTagModel->account(), preferences::keys::files::userInterface);

        appSettings.beginGroup(QStringLiteral("AddOrEditTagDialog"));

        const auto lastSelectedParentTagName =
            appSettings.value(LAST_SELECTED_PARENT_TAG_NAME_KEY).toString();

        appSettings.endGroup();

        const auto it = std::lower_bound(
            tagNames.constBegin(), tagNames.constEnd(),
            lastSelectedParentTagName);

        if ((it != tagNames.constEnd()) && (*it == lastSelectedParentTagName)) {
            const int index =
                static_cast<int>(std::distance(tagNames.constBegin(), it));

            m_pUi->parentTagNameComboBox->setCurrentIndex(index);
        }
    }

    m_pUi->tagNameLineEdit->setFocus();
}

AddOrEditTagDialog::~AddOrEditTagDialog()
{
    delete m_pUi;
}

void AddOrEditTagDialog::accept()
{
    QString tagName = m_pUi->tagNameLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(tagName);

    const QString parentTagName = m_pUi->parentTagNameComboBox->currentText();

    QNDEBUG(
        "dialog",
        "AddOrEditTagDialog::accept: tag name = "
            << tagName << ", parent tag name = " << parentTagName);

#define REPORT_ERROR(error)                                                    \
    ErrorString errorDescription(error);                                       \
    m_pUi->statusBar->setText(errorDescription.localizedString());             \
    QNWARNING("dialog", errorDescription);                                     \
    m_pUi->statusBar->setHidden(false)

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't accept new tag or edit the existing one: "
                       "tag model is gone"));
        return;
    }

    if (m_editedTagLocalId.isEmpty()) {
        QNDEBUG(
            "dialog",
            "Edited tag local uid is empty, adding new tag to "
                << "the model");

        ErrorString errorDescription;

        const auto index = m_pTagModel->createTag(
            tagName, parentTagName,
            /* linked notebook guid = */ QString(), errorDescription);

        if (!index.isValid()) {
            m_pUi->statusBar->setText(errorDescription.localizedString());
            QNWARNING("dialog", errorDescription);
            m_pUi->statusBar->setHidden(false);
            return;
        }
    }
    else {
        QNDEBUG(
            "dialog",
            "Edited tag local uid is not empty, editing "
                << "the existing tag within the model");

        const auto index = m_pTagModel->indexForLocalId(m_editedTagLocalId);
        const auto * pModelItem = m_pTagModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem)) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't edit tag: tag model item was not "
                           "found for local uid"));
            return;
        }

        const auto * pTagItem = pModelItem->cast<TagItem>();
        if (Q_UNLIKELY(!pTagItem)) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't edit tag: tag model item is not of "
                           "a tag type"));
            return;
        }

        // If needed, update the tag name
        const auto nameIndex = m_pTagModel->index(
            index.row(), static_cast<int>(TagModel::Column::Name),
            index.parent());

        if (pTagItem->name().toUpper() != tagName.toUpper()) {
            if (Q_UNLIKELY(!m_pTagModel->setData(nameIndex, tagName))) {
                // Probably the new name collides with some existing tag's name
                auto existingItemIndex = m_pTagModel->indexForTagName(
                    tagName, pTagItem->linkedNotebookGuid());

                if (existingItemIndex.isValid() &&
                    ((existingItemIndex.row() != nameIndex.row()) ||
                     (existingItemIndex.parent() != nameIndex.parent())))
                {
                    // The new name collides with some existing tag and now with
                    // the currently edited one
                    REPORT_ERROR(
                        QT_TR_NOOP("The tag name must be unique in case "
                                   "insensitive manner"));
                }
                else {
                    // Don't really know what happened...
                    REPORT_ERROR(QT_TR_NOOP("Can't set this name for the tag"));
                }

                return;
            }
        }

        const auto * pParentItem = pModelItem->parent();

        const auto * pParentTagItem =
            (pParentItem ? pParentItem->cast<TagItem>() : nullptr);

        if (!pParentItem ||
            (pParentTagItem &&
             pParentTagItem->nameUpper() != parentTagName.toUpper()))
        {
            const auto movedItemIndex =
                m_pTagModel->moveToParent(nameIndex, parentTagName);

            if (Q_UNLIKELY(!movedItemIndex.isValid())) {
                REPORT_ERROR(QT_TR_NOOP("Can't set this parent for the tag"));
                return;
            }
        }
    }

    QDialog::accept();
}

void AddOrEditTagDialog::onTagNameEdited(const QString & tagName)
{
    QNDEBUG("dialog", "AddOrEditTagDialog::onTagNameEdited: " << tagName);

    if (Q_UNLIKELY(m_currentTagName == tagName)) {
        QNTRACE("dialog", "Current tag name hasn't changed");
        return;
    }

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        QNTRACE("dialog", "Tag model is missing");
        return;
    }

    QString linkedNotebookGuid;
    if (!m_editedTagLocalId.isEmpty()) {
        const auto index = m_pTagModel->indexForLocalId(m_editedTagLocalId);
        const auto * pModelItem = m_pTagModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem)) {
            QNDEBUG(
                "dialog",
                "Found no tag model item for edited tag local "
                    << "uid");

            m_pUi->statusBar->setText(
                tr("Can't edit tag: the tag was not found "
                   "within the model by local uid"));

            m_pUi->statusBar->setHidden(false);
            m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            return;
        }

        const auto * pTagItem = pModelItem->cast<TagItem>();
        if (pTagItem) {
            linkedNotebookGuid = pTagItem->linkedNotebookGuid();
        }
    }

    const auto itemIndex =
        m_pTagModel->indexForTagName(tagName, linkedNotebookGuid);

    if (itemIndex.isValid()) {
        m_pUi->statusBar->setText(
            tr("The tag name must be unique in case insensitive manner"));

        m_pUi->statusBar->setHidden(false);
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    m_pUi->statusBar->clear();
    m_pUi->statusBar->setHidden(true);
    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);

    QStringList tagNames;
    if (m_pTagNamesModel) {
        tagNames = m_pTagNamesModel->stringList();
    }

    bool changed = false;

    // 1) If previous tag name corresponds to a real tag and is missing within
    //    the tag names, need to insert it there
    const auto previousTagNameIndex =
        m_pTagModel->indexForTagName(m_currentTagName, linkedNotebookGuid);

    if (previousTagNameIndex.isValid()) {
        const auto it = std::lower_bound(
            tagNames.constBegin(), tagNames.constEnd(), m_currentTagName);

        if ((it != tagNames.constEnd()) && (m_currentTagName != *it)) {
            // Found the tag name "larger" than the previous tag name
            const auto pos = std::distance(tagNames.constBegin(), it);
            QStringList::iterator nonConstIt = tagNames.begin();
            std::advance(nonConstIt, pos);
            Q_UNUSED(tagNames.insert(nonConstIt, m_currentTagName))
            changed = true;
        }
        else if (it == tagNames.constEnd()) {
            tagNames.append(m_currentTagName);
            changed = true;
        }
    }

    // 2) If the new edited tag name is within the list of tag names,
    //    need to remove it from there
    const auto it =
        std::lower_bound(tagNames.constBegin(), tagNames.constEnd(), tagName);

    if ((it != tagNames.constEnd()) && (tagName == *it)) {
        const auto pos = std::distance(tagNames.constBegin(), it);
        QStringList::iterator nonConstIt = tagNames.begin();
        std::advance(nonConstIt, pos);
        Q_UNUSED(tagNames.erase(nonConstIt))
        changed = true;
    }

    m_currentTagName = tagName;

    if (!changed) {
        return;
    }

    if (!m_pTagNamesModel && !tagNames.isEmpty()) {
        m_pTagNamesModel = new QStringListModel(this);
        m_pUi->parentTagNameComboBox->setModel(m_pTagNamesModel);
    }

    if (m_pTagNamesModel) {
        m_pTagNamesModel->setStringList(tagNames);
    }
}

void AddOrEditTagDialog::onParentTagNameChanged(const QString & parentTagName)
{
    QNDEBUG(
        "dialog",
        "AddOrEditTagDialog::onParentTagNameChanged: " << parentTagName);

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        QNDEBUG("dialog", "No tag model is set, nothing to do");
        return;
    }

    ApplicationSettings appSettings(
        m_pTagModel->account(), preferences::keys::files::userInterface);

    appSettings.beginGroup(QStringLiteral("AddOrEditTagDialog"));
    appSettings.setValue(LAST_SELECTED_PARENT_TAG_NAME_KEY, parentTagName);
    appSettings.endGroup();
}

void AddOrEditTagDialog::createConnections()
{
    QObject::connect(
        m_pUi->tagNameLineEdit, &QLineEdit::textEdited, this,
        &AddOrEditTagDialog::onTagNameEdited);

    QObject::connect(
        m_pUi->parentTagNameComboBox, SIGNAL(currentIndexChanged(QString)),
        this, SLOT(onParentTagNameChanged(QString)));
}

bool AddOrEditTagDialog::setupEditedTagItem(
    QStringList & tagNames, int & currentIndex)
{
    QNDEBUG("dialog", "AddOrEditTagDialog::setupEditedTagItem");

    currentIndex = -1;

    if (m_editedTagLocalId.isEmpty()) {
        QNDEBUG("dialog", "Edited tag's local uid is empty");
        return false;
    }

    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        QNDEBUG("dialog", "Tag model is null");
        return false;
    }

    const auto editedTagIndex =
        m_pTagModel->indexForLocalId(m_editedTagLocalId);

    const auto * pModelItem = m_pTagModel->itemForIndex(editedTagIndex);
    if (Q_UNLIKELY(!pModelItem)) {
        m_pUi->statusBar->setText(
            tr("Can't find the edited tag within the model"));

        m_pUi->statusBar->setHidden(false);
        return false;
    }

    const auto * pTagItem = pModelItem->cast<TagItem>();
    if (Q_UNLIKELY(!pTagItem)) {
        m_pUi->statusBar->setText(
            tr("The edited tag model item within the model "
               "is not of a tag type"));

        m_pUi->statusBar->setHidden(false);
        return false;
    }

    if (Q_UNLIKELY(!pTagItem)) {
        m_pUi->statusBar->setText(
            tr("The edited tag model item within the model has no tag item "
               "even though it is of a tag type"));

        m_pUi->statusBar->setHidden(false);
        return false;
    }

    m_pUi->tagNameLineEdit->setText(pTagItem->name());

    const QString & linkedNotebookGuid = pTagItem->linkedNotebookGuid();
    tagNames = m_pTagModel->tagNames(linkedNotebookGuid);

    // Inserting empty parent tag name to allow no parent in the combo box
    tagNames.prepend(QString());

    // 1) Remove the current tag name from the list of names available for
    // parenting
    auto it = std::lower_bound(
        tagNames.constBegin(), tagNames.constEnd(), pTagItem->name());

    if ((it != tagNames.constEnd()) && (pTagItem->name() == *it)) {
        const int pos =
            static_cast<int>(std::distance(tagNames.constBegin(), it));
        tagNames.removeAt(pos);
    }

    if (tagNames.isEmpty()) {
        QNDEBUG(
            "dialog",
            "Tag names list has become empty after removing "
                << "the current tag's name");
        return true;
    }

    // 2) Remove all the children of this tag from the list of possible parents
    // for it
    const int childCount = pModelItem->childrenCount();
    for (int i = 0; i < childCount; ++i) {
        const auto * pChildItem = pModelItem->childAtRow(i);
        if (Q_UNLIKELY(!pChildItem)) {
            QNWARNING("dialog", "Found null child item at row " << i);
            continue;
        }

        const auto * pChildTagItem = pChildItem->cast<TagItem>();
        if (Q_UNLIKELY(!pChildTagItem)) {
            QNWARNING(
                "dialog",
                "Found tag model item without the actual "
                    << "tag item within the list of tag item's children");
            continue;
        }

        it = std::lower_bound(
            tagNames.constBegin(), tagNames.constEnd(), pChildTagItem->name());

        if ((it != tagNames.constEnd()) && (pTagItem->name() == *it)) {
            int pos =
                static_cast<int>(std::distance(tagNames.constBegin(), it));

            tagNames.removeAt(pos);
        }
    }

    // 3) If the current tag has a parent, set it's name as the current one
    const auto editedTagParentIndex = editedTagIndex.parent();
    const auto * pParentItem = pModelItem->parent();

    const auto * pParentTagItem =
        (pParentItem ? pParentItem->cast<TagItem>() : nullptr);

    if (editedTagParentIndex.isValid() && pParentTagItem) {
        const QString & parentTagName = pParentTagItem->name();

        it = std::lower_bound(
            tagNames.constBegin(), tagNames.constEnd(), parentTagName);

        if ((it != tagNames.constEnd()) && (parentTagName == *it)) {
            currentIndex =
                static_cast<int>(std::distance(tagNames.constBegin(), it));
        }
    }

    return true;
}

} // namespace quentier
