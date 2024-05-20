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

#include "AddOrEditTagDialog.h"

#include "ui_AddOrEditTagDialog.h"

#include <lib/preferences/keys/Files.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QPushButton>
#include <QStringListModel>

#include <algorithm>
#include <iterator>
#include <string_view>

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gLastSelectedParentTagNameKey = "_LastSelectedParentTagName"sv;

} // namespace

AddOrEditTagDialog::AddOrEditTagDialog(
    TagModel * tagModel, QWidget * parent, QString editedTagLocalId) :
    QDialog(parent),
    m_ui(new Ui::AddOrEditTagDialog), m_tagModel(tagModel),
    m_editedTagLocalId(std::move(editedTagLocalId))
{
    m_ui->setupUi(this);
    m_ui->statusBar->setHidden(true);

    QStringList tagNames;
    int parentTagNameIndex = -1;
    const bool res = setupEditedTagItem(tagNames, parentTagNameIndex);
    if (!res && !m_tagModel.isNull()) {
        tagNames = m_tagModel->tagNames();
        tagNames.prepend(QLatin1String(""));
    }

    m_tagNamesModel = new QStringListModel(this);
    m_tagNamesModel->setStringList(tagNames);
    m_ui->parentTagNameComboBox->setModel(m_tagNamesModel);

    createConnections();

    if (res) {
        m_ui->parentTagNameComboBox->setCurrentIndex(parentTagNameIndex);
    }
    else if (!tagNames.isEmpty() && !m_tagModel.isNull()) {
        parentTagNameIndex = 0;
        m_ui->parentTagNameComboBox->setCurrentIndex(parentTagNameIndex);

        ApplicationSettings appSettings{
            m_tagModel->account(), preferences::keys::files::userInterface};

        appSettings.beginGroup(QStringLiteral("AddOrEditTagDialog"));

        const QString lastSelectedParentTagName =
            appSettings.value(gLastSelectedParentTagNameKey).toString();

        appSettings.endGroup();

        const auto it = std::lower_bound(
            tagNames.constBegin(), tagNames.constEnd(),
            lastSelectedParentTagName);

        if (it != tagNames.constEnd() && *it == lastSelectedParentTagName) {
            const int index =
                static_cast<int>(std::distance(tagNames.constBegin(), it));

            m_ui->parentTagNameComboBox->setCurrentIndex(index);
        }
    }

    m_ui->tagNameLineEdit->setFocus();
}

AddOrEditTagDialog::~AddOrEditTagDialog()
{
    delete m_ui;
}

void AddOrEditTagDialog::accept()
{
    QString tagName = m_ui->tagNameLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(tagName);

    const QString parentTagName = m_ui->parentTagNameComboBox->currentText();

    QNDEBUG(
        "dialog::AddOrEditTagDialog",
        "AddOrEditTagDialog::accept: tag name = "
            << tagName << ", parent tag name = " << parentTagName);

#define REPORT_ERROR(error)                                                    \
    ErrorString errorDescription(error);                                       \
    m_ui->statusBar->setText(errorDescription.localizedString());              \
    QNWARNING("dialog::AddOrEditTagDialog", errorDescription);                 \
    m_ui->statusBar->setHidden(false)

    if (Q_UNLIKELY(m_tagModel.isNull())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't accept new tag or edit the existing one: "
                       "tag model is gone"));
        return;
    }

    if (m_editedTagLocalId.isEmpty()) {
        QNDEBUG(
            "dialog::AddOrEditTagDialog",
            "Edited tag local id is empty, adding new tag to the model");

        ErrorString errorDescription;
        const auto index = m_tagModel->createTag(
            tagName, parentTagName,
            /* linked notebook guid = */ QString(), errorDescription);

        if (!index.isValid()) {
            m_ui->statusBar->setText(errorDescription.localizedString());
            QNWARNING("dialog::AddOrEditTagDialog", errorDescription);
            m_ui->statusBar->setHidden(false);
            return;
        }
    }
    else {
        QNDEBUG(
            "dialog::AddOrEditTagDialog",
            "Edited tag local id is not empty, editing "
                << "the existing tag within the model");

        const auto index = m_tagModel->indexForLocalId(m_editedTagLocalId);
        const auto * modelItem = m_tagModel->itemForIndex(index);
        if (Q_UNLIKELY(!modelItem)) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't edit tag: tag model item was not "
                           "found for local id"));
            return;
        }

        const auto * tagItem = modelItem->cast<TagItem>();
        if (Q_UNLIKELY(!tagItem)) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't edit tag: tag model item is not of a tag "
                           "type"));
            return;
        }

        // If needed, update the tag name
        const auto nameIndex = m_tagModel->index(
            index.row(), static_cast<int>(TagModel::Column::Name),
            index.parent());

        if (tagItem->name().toUpper() != tagName.toUpper()) {
            if (Q_UNLIKELY(!m_tagModel->setData(nameIndex, tagName))) {
                // Probably the new name collides with some existing tag's name
                const auto existingItemIndex = m_tagModel->indexForTagName(
                    tagName, tagItem->linkedNotebookGuid());

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

        const auto * parentItem = modelItem->parent();

        const auto * parentTagItem =
            (parentItem ? parentItem->cast<TagItem>() : nullptr);

        if (!parentItem ||
            (parentTagItem &&
             parentTagItem->nameUpper() != parentTagName.toUpper()))
        {
            const auto movedItemIndex =
                m_tagModel->moveToParent(nameIndex, parentTagName);

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
    QNDEBUG(
        "dialog::AddOrEditTagDialog",
        "AddOrEditTagDialog::onTagNameEdited: " << tagName);

    if (Q_UNLIKELY(m_currentTagName == tagName)) {
        QNTRACE(
            "dialog::AddOrEditTagDialog", "Current tag name hasn't changed");
        return;
    }

    if (Q_UNLIKELY(m_tagModel.isNull())) {
        QNTRACE("dialog::AddOrEditTagDialog", "Tag model is missing");
        return;
    }

    QString linkedNotebookGuid;
    if (!m_editedTagLocalId.isEmpty()) {
        const auto index = m_tagModel->indexForLocalId(m_editedTagLocalId);
        const auto * modelItem = m_tagModel->itemForIndex(index);
        if (Q_UNLIKELY(!modelItem)) {
            QNDEBUG(
                "dialog::AddOrEditTagDialog",
                "Found no tag model item for edited tag local id");

            m_ui->statusBar->setText(
                tr("Can't edit tag: the tag was not found "
                   "within the model by local id"));

            m_ui->statusBar->setHidden(false);
            m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            return;
        }

        const auto * tagItem = modelItem->cast<TagItem>();
        if (tagItem) {
            linkedNotebookGuid = tagItem->linkedNotebookGuid();
        }
    }

    const auto itemIndex =
        m_tagModel->indexForTagName(tagName, linkedNotebookGuid);

    if (itemIndex.isValid()) {
        m_ui->statusBar->setText(
            tr("The tag name must be unique in case insensitive manner"));

        m_ui->statusBar->setHidden(false);
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    m_ui->statusBar->clear();
    m_ui->statusBar->setHidden(true);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);

    QStringList tagNames;
    if (m_tagNamesModel) {
        tagNames = m_tagNamesModel->stringList();
    }

    bool changed = false;

    // 1) If previous tag name corresponds to a real tag and is missing within
    //    the tag names, need to insert it there
    const auto previousTagNameIndex =
        m_tagModel->indexForTagName(m_currentTagName, linkedNotebookGuid);

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

    if (!m_tagNamesModel && !tagNames.isEmpty()) {
        m_tagNamesModel = new QStringListModel(this);
        m_ui->parentTagNameComboBox->setModel(m_tagNamesModel);
    }

    if (m_tagNamesModel) {
        m_tagNamesModel->setStringList(tagNames);
    }
}

void AddOrEditTagDialog::onParentTagNameChanged(const QString & parentTagName)
{
    QNDEBUG(
        "dialog::AddOrEditTagDialog",
        "AddOrEditTagDialog::onParentTagNameChanged: " << parentTagName);

    if (Q_UNLIKELY(m_tagModel.isNull())) {
        QNDEBUG(
            "dialog::AddOrEditTagDialog", "No tag model is set, nothing to do");
        return;
    }

    ApplicationSettings appSettings{
        m_tagModel->account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("AddOrEditTagDialog"));
    appSettings.setValue(gLastSelectedParentTagNameKey, parentTagName);
    appSettings.endGroup();
}

void AddOrEditTagDialog::createConnections()
{
    QObject::connect(
        m_ui->tagNameLineEdit, &QLineEdit::textEdited, this,
        &AddOrEditTagDialog::onTagNameEdited);

    QObject::connect(
        m_ui->parentTagNameComboBox, SIGNAL(currentIndexChanged(QString)),
        this, SLOT(onParentTagNameChanged(QString)));
}

bool AddOrEditTagDialog::setupEditedTagItem(
    QStringList & tagNames, int & currentIndex)
{
    QNDEBUG(
        "dialog::AddOrEditTagDialog", "AddOrEditTagDialog::setupEditedTagItem");

    currentIndex = -1;

    if (m_editedTagLocalId.isEmpty()) {
        QNDEBUG("dialog::AddOrEditTagDialog", "Edited tag's local id is empty");
        return false;
    }

    if (Q_UNLIKELY(m_tagModel.isNull())) {
        QNDEBUG("dialog::AddOrEditTagDialog", "Tag model is null");
        return false;
    }

    const auto editedTagIndex = m_tagModel->indexForLocalId(m_editedTagLocalId);

    const auto * modelItem = m_tagModel->itemForIndex(editedTagIndex);
    if (Q_UNLIKELY(!modelItem)) {
        m_ui->statusBar->setText(
            tr("Can't find the edited tag within the model"));

        m_ui->statusBar->setHidden(false);
        return false;
    }

    const auto * tagItem = modelItem->cast<TagItem>();
    if (Q_UNLIKELY(!tagItem)) {
        m_ui->statusBar->setText(
            tr("The edited tag model item within the model "
               "is not of a tag type"));

        m_ui->statusBar->setHidden(false);
        return false;
    }

    if (Q_UNLIKELY(!tagItem)) {
        m_ui->statusBar->setText(
            tr("The edited tag model item within the model has no tag item "
               "even though it is of a tag type"));

        m_ui->statusBar->setHidden(false);
        return false;
    }

    m_ui->tagNameLineEdit->setText(tagItem->name());

    const QString & linkedNotebookGuid = tagItem->linkedNotebookGuid();
    tagNames = m_tagModel->tagNames(linkedNotebookGuid);

    // Inserting empty parent tag name to allow no parent in the combo box
    tagNames.prepend(QString());

    // 1) Remove the current tag name from the list of names available for
    // parenting
    auto it = std::lower_bound(
        tagNames.constBegin(), tagNames.constEnd(), tagItem->name());

    if (it != tagNames.constEnd() && tagItem->name() == *it) {
        int pos = static_cast<int>(std::distance(tagNames.constBegin(), it));
        tagNames.removeAt(pos);
    }

    if (tagNames.isEmpty()) {
        QNDEBUG(
            "dialog::AddOrEditTagDialog",
            "Tag names list has become empty after removing "
                << "the current tag's name");
        return true;
    }

    // 2) Remove all the children of this tag from the list of possible parents
    // for it
    const auto childCount = modelItem->childrenCount();
    for (int i = 0; i < childCount; ++i) {
        const auto * childItem = modelItem->childAtRow(i);
        if (Q_UNLIKELY(!childItem)) {
            QNWARNING(
                "dialog::AddOrEditTagDialog",
                "Found null child item at row " << i);
            continue;
        }

        const auto * childTagItem = childItem->cast<TagItem>();
        if (Q_UNLIKELY(!childTagItem)) {
            QNWARNING(
                "dialog::AddOrEditTagDialog",
                "Found tag model item without the actual "
                    << "tag item within the list of tag item's children");
            continue;
        }

        it = std::lower_bound(
            tagNames.constBegin(), tagNames.constEnd(), childTagItem->name());

        if ((it != tagNames.constEnd()) && (tagItem->name() == *it)) {
            const int pos =
                static_cast<int>(std::distance(tagNames.constBegin(), it));

            tagNames.removeAt(pos);
        }
    }

    // 3) If the current tag has a parent, set it's name as the current one
    const auto editedTagParentIndex = editedTagIndex.parent();
    const auto * parentItem = modelItem->parent();

    const auto * parentTagItem =
        (parentItem ? parentItem->cast<TagItem>() : nullptr);

    if (editedTagParentIndex.isValid() && parentTagItem) {
        const QString & parentTagName = parentTagItem->name();

        it = std::lower_bound(
            tagNames.constBegin(), tagNames.constEnd(), parentTagName);

        if (it != tagNames.constEnd() && parentTagName == *it) {
            currentIndex =
                static_cast<int>(std::distance(tagNames.constBegin(), it));
        }
    }

    return true;
}

} // namespace quentier
