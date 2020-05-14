/*
 * Copyright 2017-2020 Dmitry Ivanov
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

#include <lib/preferences/SettingsNames.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QPushButton>
#include <QStringListModel>

#define LAST_SELECTED_STACK QStringLiteral("LastSelectedNotebookStack")

namespace quentier {

AddOrEditNotebookDialog::AddOrEditNotebookDialog(
        NotebookModel * pNotebookModel, QWidget * parent,
        const QString & editedNotebookLocalUid) :
    QDialog(parent),
    m_pUi(new Ui::AddOrEditNotebookDialog),
    m_pNotebookModel(pNotebookModel),
    m_editedNotebookLocalUid(editedNotebookLocalUid)
{
    QNDEBUG("AddOrEditNotebookDialog: edited notebook local uid = "
        << editedNotebookLocalUid);

    m_pUi->setupUi(this);
    m_pUi->statusBar->setHidden(true);

    QStringList stacks;
    if (!m_pNotebookModel.isNull())
    {
        if (m_editedNotebookLocalUid.isEmpty())
        {
            stacks = m_pNotebookModel->stacks(m_editedNotebookLocalUid);
        }
        else
        {
            auto editedNotebookIndex = m_pNotebookModel->indexForLocalUid(
                m_editedNotebookLocalUid);

            const auto * pModelItem = m_pNotebookModel->itemForIndex(
                editedNotebookIndex);

            const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
            if (pNotebookItem) {
                stacks = m_pNotebookModel->stacks(
                    pNotebookItem->linkedNotebookGuid());
            }
        }
    }

    if (!stacks.isEmpty()) {
        m_pNotebookStacksModel = new QStringListModel(this);
        m_pNotebookStacksModel->setStringList(stacks);
        m_pUi->notebookStackComboBox->setModel(m_pNotebookStacksModel);
    }

    createConnections();

    if (!m_editedNotebookLocalUid.isEmpty() && !m_pNotebookModel.isNull())
    {
        auto editedNotebookIndex = m_pNotebookModel->indexForLocalUid(
            m_editedNotebookLocalUid);

        const auto * pModelItem = m_pNotebookModel->itemForIndex(
            editedNotebookIndex);

        if (!pModelItem)
        {
            m_pUi->statusBar->setText(
                tr("Can't find the edited notebook within the model"));

            m_pUi->statusBar->setHidden(false);
        }
        else
        {
            const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
            if (!pNotebookItem)
            {
                m_pUi->statusBar->setText(
                    tr("Internal error: the edited item is not a notebook"));

                m_pUi->statusBar->setHidden(false);
            }
            else
            {
                m_pUi->notebookNameLineEdit->setText(pNotebookItem->name());
                const QString & stack = pNotebookItem->stack();
                if (!stack.isEmpty())
                {
                    int index = stacks.indexOf(stack);
                    if (index >= 0) {
                        m_pUi->notebookStackComboBox->setCurrentIndex(index);
                    }
                }
            }
        }
    }
    else if (!stacks.isEmpty() && !m_pNotebookModel.isNull())
    {
        ApplicationSettings appSettings(
            m_pNotebookModel->account(),
            QUENTIER_UI_SETTINGS);

        appSettings.beginGroup(QStringLiteral("AddOrEditNotebookDialog"));

        QString lastSelectedStack = appSettings.value(
            LAST_SELECTED_STACK).toString();

        appSettings.endGroup();

        int index = stacks.indexOf(lastSelectedStack);
        if (index >= 0) {
            m_pUi->notebookStackComboBox->setCurrentIndex(index);
        }
    }

    m_pUi->notebookNameLineEdit->setFocus();
}

AddOrEditNotebookDialog::~AddOrEditNotebookDialog()
{
    delete m_pUi;
}

void AddOrEditNotebookDialog::accept()
{
    QString notebookName = m_pUi->notebookNameLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(notebookName);

    QString stack = m_pUi->notebookStackComboBox->currentText().trimmed();
    m_stringUtils.removeNewlines(stack);

    QNDEBUG("AddOrEditNotebookDialog::accept: notebook name = "
        << notebookName << ", stack: " << stack);

#define REPORT_ERROR(error)                                                    \
    m_pUi->statusBar->setText(tr(error));                                      \
    QNWARNING(error);                                                          \
    m_pUi->statusBar->setHidden(false)                                         \
// REPORT_ERROR

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        REPORT_ERROR("Can't accept new notebook or edit existing one: "
                     "notebook model is gone");
        return;
    }

    if (m_editedNotebookLocalUid.isEmpty())
    {
        QNDEBUG("Edited notebook local uid is empty, adding new notebook "
            << "to the model");

        ErrorString errorDescription;
        QModelIndex index = m_pNotebookModel->createNotebook(
            notebookName,
            stack,
            errorDescription);

        if (!index.isValid()) {
            m_pUi->statusBar->setText(errorDescription.localizedString());
            QNWARNING(errorDescription);
            m_pUi->statusBar->setHidden(false);
            return;
        }
    }
    else
    {
        QNDEBUG("Edited notebook local uid is not empty, editing "
            << "the existing notebook within the model");

        auto index = m_pNotebookModel->indexForLocalUid(
            m_editedNotebookLocalUid);

        const auto * pModelItem = m_pNotebookModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem)) {
            REPORT_ERROR("Can't edit notebook: notebook was not found "
                         "in the model");
            return;
        }

        const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!pNotebookItem)) {
            REPORT_ERROR("Can't edit notebook: the edited model item is not "
                         "a notebook");
            return;
        }

        // If needed, update the notebook name
        QModelIndex nameIndex = m_pNotebookModel->index(
            index.row(),
            static_cast<int>(NotebookModel::Column::Name),
            index.parent());

        if (pNotebookItem->name().toUpper() != notebookName.toUpper())
        {
            bool res = m_pNotebookModel->setData(nameIndex, notebookName);
            if (Q_UNLIKELY(!res))
            {
                // Probably new name collides with some existing notebook's name
                auto existingItemIndex = m_pNotebookModel->indexForNotebookName(
                    notebookName,
                    pNotebookItem->linkedNotebookGuid());

                if (existingItemIndex.isValid() &&
                    ((existingItemIndex.row() != nameIndex.row()) ||
                     (existingItemIndex.parent() != nameIndex.parent())))
                {
                    // The new name collides with some existing notebook and not
                    // with the currently edited one
                    REPORT_ERROR("The notebook name must be unique in case "
                                 "insensitive manner");
                }
                else
                {
                    // Don't really know what happened...
                    REPORT_ERROR("Can't set this name for the notebook");
                }

                return;
            }
        }

        if (pNotebookItem->stack() != stack)
        {
            auto movedItemIndex = m_pNotebookModel->moveToStack(
                nameIndex,
                stack);

            if (Q_UNLIKELY(!movedItemIndex.isValid())) {
                REPORT_ERROR("Can't set this stack for the notebook");
                return;
            }
        }
    }

    QDialog::accept();
}

#undef REPORT_ERROR

void AddOrEditNotebookDialog::onNotebookNameEdited(const QString & notebookName)
{
    QNDEBUG("AddOrEditNotebookDialog::onNotebookNameEdited: "
        << notebookName);

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        QNDEBUG("Notebook model is missing");
        return;
    }

    QString linkedNotebookGuid;
    if (!m_editedNotebookLocalUid.isEmpty())
    {
        auto index = m_pNotebookModel->indexForLocalUid(
            m_editedNotebookLocalUid);

        const auto * pModelItem = m_pNotebookModel->itemForIndex(index);
        if (Q_UNLIKELY(!pModelItem))
        {
            m_pUi->statusBar->setText(
                tr("Can't edit notebook: notebook was "
                   "not found in the model"));

            m_pUi->statusBar->setHidden(false);
            m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            return;
        }

        const auto * pNotebookItem = pModelItem->cast<NotebookItem>();
        if (Q_UNLIKELY(!pNotebookItem))
        {
            m_pUi->statusBar->setText(
                tr("Can't edit notebook: the edited model "
                   "item is not a notebook"));

            m_pUi->statusBar->setHidden(false);
            m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            return;
        }

        if (Q_UNLIKELY(!pNotebookItem))
        {
            m_pUi->statusBar->setText(
                tr("Internal error, can't edit notebook: the edited model item o"
                   "has null pointer to the notebook item"));

            m_pUi->statusBar->setHidden(false);
            m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
            return;
        }

        linkedNotebookGuid = pNotebookItem->linkedNotebookGuid();
    }

    auto itemIndex = m_pNotebookModel->indexForNotebookName(
        notebookName,
        linkedNotebookGuid);

    if (itemIndex.isValid())
    {
        m_pUi->statusBar->setText(
            tr("The notebook name must be unique in case "
               "insensitive manner"));

        m_pUi->statusBar->setHidden(false);
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        return;
    }

    m_pUi->statusBar->clear();
    m_pUi->statusBar->setHidden(true);
    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);
}

void AddOrEditNotebookDialog::onNotebookStackChanged(const QString & stack)
{
    QNDEBUG("AddOrEditNotebookDialog::onNotebookStackChanged: " << stack);

    if (Q_UNLIKELY(m_pNotebookModel.isNull())) {
        QNDEBUG("No notebook model is set, nothing to do");
        return;
    }

    ApplicationSettings appSettings(
        m_pNotebookModel->account(),
        QUENTIER_UI_SETTINGS);

    appSettings.beginGroup(QStringLiteral("AddOrEditNotebookDialog"));
    appSettings.setValue(LAST_SELECTED_STACK, stack);
    appSettings.endGroup();
}

void AddOrEditNotebookDialog::createConnections()
{
    QObject::connect(
        m_pUi->notebookNameLineEdit,
        &QLineEdit::textEdited,
        this,
        &AddOrEditNotebookDialog::onNotebookNameEdited);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->notebookStackComboBox,
        qOverload<const QString&>(&QComboBox::currentIndexChanged),
        this,
        &AddOrEditNotebookDialog::onNotebookStackChanged);
        SLOT(onNotebookStackChanged(QString));
#else
    QObject::connect(
        m_pUi->notebookStackComboBox,
        SIGNAL(currentIndexChanged(QString)),
        this,
        SLOT(onNotebookStackChanged(QString)));
#endif

    QObject::connect(
        m_pUi->notebookStackComboBox,
        &QComboBox::editTextChanged,
        this,
        &AddOrEditNotebookDialog::onNotebookStackChanged);
}

} // namespace quentier
