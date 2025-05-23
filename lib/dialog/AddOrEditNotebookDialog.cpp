/*
 * Copyright 2017-2025 Dmitry Ivanov
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

#include "AddOrEditNotebookDialog.h"
#include "ui_AddOrEditNotebookDialog.h"

#include <lib/preferences/keys/Files.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QPushButton>
#include <QStringListModel>

#include <limits>
#include <string_view>

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gLastSelectedStack = "LastSelectedNotebookStack"sv;

} // namespace

AddOrEditNotebookDialog::AddOrEditNotebookDialog(
    NotebookModel * notebookModel, QWidget * parent,
    QString editedNotebookLocalId) :
    QDialog{parent},
    m_ui{new Ui::AddOrEditNotebookDialog}, m_notebookModel{notebookModel},
    m_editedNotebookLocalId{std::move(editedNotebookLocalId)}
{
    QNDEBUG(
        "dialog::AddOrEditNotebookDialog",
        "AddOrEditNotebookDialog: edited notebook local id = "
            << editedNotebookLocalId);

    m_ui->setupUi(this);
    m_ui->statusBar->setHidden(true);

    QStringList stacks;
    if (!m_notebookModel.isNull()) {
        if (m_editedNotebookLocalId.isEmpty()) {
            stacks = m_notebookModel->stacks(m_editedNotebookLocalId);
        }
        else {
            const auto editedNotebookIndex =
                m_notebookModel->indexForLocalId(m_editedNotebookLocalId);

            const auto * modelItem =
                m_notebookModel->itemForIndex(editedNotebookIndex);

            const auto * notebookItem = modelItem->cast<NotebookItem>();
            if (notebookItem) {
                stacks = m_notebookModel->stacks(
                    notebookItem->linkedNotebookGuid());
            }
        }
    }

    if (!stacks.isEmpty()) {
        m_pNotebookStacksModel = new QStringListModel(this);
        m_pNotebookStacksModel->setStringList(stacks);
        m_ui->notebookStackComboBox->setModel(m_pNotebookStacksModel);
    }

    createConnections();

    if (!m_editedNotebookLocalId.isEmpty() && !m_notebookModel.isNull()) {
        const auto editedNotebookIndex =
            m_notebookModel->indexForLocalId(m_editedNotebookLocalId);

        const auto * modelItem =
            m_notebookModel->itemForIndex(editedNotebookIndex);

        if (!modelItem) {
            m_ui->statusBar->setText(
                tr("Can't find the edited notebook within the model"));

            m_ui->statusBar->setHidden(false);
        }
        else {
            const auto * notebookItem = modelItem->cast<NotebookItem>();
            if (!notebookItem) {
                m_ui->statusBar->setText(
                    tr("Internal error: the edited item is not a notebook"));

                m_ui->statusBar->setHidden(false);
            }
            else {
                m_ui->notebookNameLineEdit->setText(notebookItem->name());
                const QString & stack = notebookItem->stack();
                if (!stack.isEmpty()) {
                    auto index = stacks.indexOf(stack);
                    if (index >= 0) {
                        Q_ASSERT(index <= std::numeric_limits<int>::max());
                        m_ui->notebookStackComboBox->setCurrentIndex(
                            static_cast<int>(index));
                    }
                }
            }
        }
    }
    else if (!stacks.isEmpty() && !m_notebookModel.isNull()) {
        utility::ApplicationSettings appSettings{
            m_notebookModel->account(),
            preferences::keys::files::userInterface};

        appSettings.beginGroup(QStringLiteral("AddOrEditNotebookDialog"));

        const QString lastSelectedStack =
            appSettings.value(gLastSelectedStack).toString();

        appSettings.endGroup();

        const auto index = stacks.indexOf(lastSelectedStack);
        if (index >= 0) {
            Q_ASSERT(index <= std::numeric_limits<int>::max());
            m_ui->notebookStackComboBox->setCurrentIndex(
                static_cast<int>(index));
        }
    }

    m_ui->notebookNameLineEdit->setFocus();
}

AddOrEditNotebookDialog::~AddOrEditNotebookDialog()
{
    delete m_ui;
}

void AddOrEditNotebookDialog::accept()
{
    QString notebookName = m_ui->notebookNameLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(notebookName);

    QString stack = m_ui->notebookStackComboBox->currentText().trimmed();
    m_stringUtils.removeNewlines(stack);

    QNDEBUG(
        "dialog::AddOrEditNotebookDialog",
        "AddOrEditNotebookDialog::accept: notebook name = "
            << notebookName << ", stack: " << stack);

#define REPORT_ERROR(error)                                                    \
    m_ui->statusBar->setText(tr(error));                                       \
    QNWARNING("dialog::AddOrEditNotebookDialog", error);                       \
    m_ui->statusBar->setHidden(false)

    if (Q_UNLIKELY(m_notebookModel.isNull())) {
        REPORT_ERROR(
            "Can't accept new notebook or edit existing one: "
            "notebook model is gone");
        return;
    }

    if (m_editedNotebookLocalId.isEmpty()) {
        QNDEBUG(
            "dialog::AddOrEditNotebookDialog",
            "Edited notebook local id is empty, adding new notebook to the "
            "model");

        ErrorString errorDescription;
        const auto index = m_notebookModel->createNotebook(
            notebookName, stack, errorDescription);

        if (!index.isValid()) {
            m_ui->statusBar->setText(errorDescription.localizedString());
            QNWARNING("dialog::AddOrEditNotebookDialog", errorDescription);
            m_ui->statusBar->setHidden(false);
            return;
        }
    }
    else {
        QNDEBUG(
            "dialog::AddOrEditNotebookDialog",
            "Edited notebook local id is not empty, editing "
                << "the existing notebook within the model");

        const auto index =
            m_notebookModel->indexForLocalId(m_editedNotebookLocalId);

        const auto * modelItem = m_notebookModel->itemForIndex(index);
        if (Q_UNLIKELY(!modelItem)) {
            REPORT_ERROR(
                "Can't edit notebook: notebook was not found in the model");
            return;
        }

        const auto * notebookItem = modelItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!notebookItem)) {
            REPORT_ERROR(
                "Can't edit notebook: the edited model item is not "
                "a notebook");
            return;
        }

        // If needed, update the notebook name
        const auto nameIndex = m_notebookModel->index(
            index.row(), static_cast<int>(NotebookModel::Column::Name),
            index.parent());

        if (notebookItem->name().toUpper() != notebookName.toUpper()) {
            if (Q_UNLIKELY(!m_notebookModel->setData(nameIndex, notebookName)))
            {
                // Probably new name collides with some existing notebook's name
                const auto existingItemIndex =
                    m_notebookModel->indexForNotebookName(
                        notebookName, notebookItem->linkedNotebookGuid());

                if (existingItemIndex.isValid() &&
                    ((existingItemIndex.row() != nameIndex.row()) ||
                     (existingItemIndex.parent() != nameIndex.parent())))
                {
                    // The new name collides with some existing notebook and not
                    // with the currently edited one
                    REPORT_ERROR(
                        "The notebook name must be unique in case "
                        "insensitive manner");
                }
                else {
                    // Don't really know what happened...
                    REPORT_ERROR("Can't set this name for the notebook");
                }

                return;
            }
        }

        if (notebookItem->stack() != stack) {
            const auto movedItemIndex =
                m_notebookModel->moveToStack(nameIndex, stack);

            if (Q_UNLIKELY(!movedItemIndex.isValid())) {
                REPORT_ERROR("Can't set this stack for the notebook");
                return;
            }
        }
    }

#undef REPORT_ERROR

    QDialog::accept();
}

void AddOrEditNotebookDialog::onNotebookNameEdited(const QString & notebookName)
{
    QNDEBUG(
        "dialog::AddOrEditNotebookDialog",
        "AddOrEditNotebookDialog::onNotebookNameEdited: " << notebookName);

    if (Q_UNLIKELY(m_notebookModel.isNull())) {
        QNDEBUG("dialog::AddOrEditNotebookDialog", "Notebook model is missing");
        return;
    }

    QString linkedNotebookGuid;
    if (!m_editedNotebookLocalId.isEmpty()) {
        const auto index =
            m_notebookModel->indexForLocalId(m_editedNotebookLocalId);

        const auto * modelItem = m_notebookModel->itemForIndex(index);
        if (Q_UNLIKELY(!modelItem)) {
            m_ui->statusBar->setText(
                tr("Can't edit notebook: notebook was "
                   "not found in the model"));

            m_ui->statusBar->setHidden(false);
            m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            return;
        }

        const auto * notebookItem = modelItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!notebookItem)) {
            m_ui->statusBar->setText(
                tr("Can't edit notebook: the edited model "
                   "item is not a notebook"));

            m_ui->statusBar->setHidden(false);
            m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            return;
        }

        if (Q_UNLIKELY(!notebookItem)) {
            m_ui->statusBar->setText(tr(
                "Internal error, can't edit notebook: the edited model item o"
                "has null pointer to the notebook item"));

            m_ui->statusBar->setHidden(false);
            m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            return;
        }

        linkedNotebookGuid = notebookItem->linkedNotebookGuid();
    }

    const auto itemIndex = m_notebookModel->indexForNotebookName(
        notebookName, linkedNotebookGuid);

    if (itemIndex.isValid()) {
        m_ui->statusBar->setText(
            tr("The notebook name must be unique in case insensitive manner"));

        m_ui->statusBar->setHidden(false);
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    m_ui->statusBar->clear();
    m_ui->statusBar->setHidden(true);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);
}

void AddOrEditNotebookDialog::onNotebookStackIndexChanged(int stackIndex)
{
    onNotebookStackChanged(m_ui->notebookStackComboBox->itemText(stackIndex));
}

void AddOrEditNotebookDialog::onNotebookStackChanged(const QString & stack)
{
    QNDEBUG(
        "dialog::AddOrEditNotebookDialog", "AddOrEditNotebookDialog::onNotebookStackChanged: " << stack);

    if (Q_UNLIKELY(m_notebookModel.isNull())) {
        QNDEBUG("dialog::AddOrEditNotebookDialog", "No notebook model is set, nothing to do");
        return;
    }

    utility::ApplicationSettings appSettings{
        m_notebookModel->account(), preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("AddOrEditNotebookDialog"));
    appSettings.setValue(gLastSelectedStack, stack);
    appSettings.endGroup();
}

void AddOrEditNotebookDialog::createConnections()
{
    QObject::connect(
        m_ui->notebookNameLineEdit, &QLineEdit::textEdited, this,
        &AddOrEditNotebookDialog::onNotebookNameEdited);

    QObject::connect(
        m_ui->notebookStackComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &AddOrEditNotebookDialog::onNotebookStackIndexChanged);

    QObject::connect(
        m_ui->notebookStackComboBox, &QComboBox::editTextChanged, this,
        &AddOrEditNotebookDialog::onNotebookStackChanged);
}

} // namespace quentier
