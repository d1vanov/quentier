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

#include "AddOrEditSavedSearchDialog.h"
#include "ui_AddOrEditSavedSearchDialog.h"

#include <lib/model/saved_search/SavedSearchModel.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <quentier/logging/QuentierLogger.h>

#include <QPushButton>

namespace quentier {

AddOrEditSavedSearchDialog::AddOrEditSavedSearchDialog(
    SavedSearchModel * savedSearchModel, QWidget * parent,
    const QString & editedSavedSearchLocalId) :
    QDialog{parent},
    m_ui{new Ui::AddOrEditSavedSearchDialog},
    m_savedSearchModel{savedSearchModel},
    m_searchQuery{new local_storage::NoteSearchQuery},
    m_editedSavedSearchLocalId{editedSavedSearchLocalId}
{
    m_ui->setupUi(this);
    m_ui->statusBar->setHidden(true);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    setupEditedSavedSearchItem();

    createConnections();

    if (!m_editedSavedSearchLocalId.isEmpty()) {
        m_ui->searchQueryPlainTextEdit->setFocus();
    }
    else {
        m_ui->savedSearchNameLineEdit->setFocus();
    }
}

AddOrEditSavedSearchDialog::~AddOrEditSavedSearchDialog()
{
    delete m_ui;
}

QString AddOrEditSavedSearchDialog::query() const
{
    return m_ui->searchQueryPlainTextEdit->toPlainText();
}

void AddOrEditSavedSearchDialog::setQuery(const QString & query)
{
    QNDEBUG(
        "dialog::AddOrEditSavedSearchDialog",
        "AddOrEditSavedSearchDialog::setQuery: " << query);

    m_ui->searchQueryPlainTextEdit->setPlainText(query);
}

void AddOrEditSavedSearchDialog::accept()
{
    QString savedSearchName = m_ui->savedSearchNameLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(savedSearchName);

    QString savedSearchQuery = m_searchQuery->queryString();
    const bool queryIsEmpty = m_searchQuery->isEmpty();

    QNDEBUG(
        "dialog::AddOrEditSavedSearchDialog",
        "AddOrEditSavedSearchDialog::accept: name = "
            << savedSearchName << ", query = " << savedSearchQuery
            << ", query is empty = " << (queryIsEmpty ? "true" : "false"));

#define REPORT_ERROR(error)                                                    \
    m_ui->statusBar->setText(tr(error));                                       \
    QNWARNING("dialog::AddOrEditSavedSearchDialog", error);                    \
    m_ui->statusBar->setHidden(false)

    if (Q_UNLIKELY(m_savedSearchModel.isNull())) {
        REPORT_ERROR(
            QT_TR_NOOP("Can't accept new saved search or edit "
                       "the existing one: saved search model is gone"));
        return;
    }

    if (m_editedSavedSearchLocalId.isEmpty()) {
        QNDEBUG(
            "dialog::AddOrEditSavedSearchDialog",
            "Edited saved search local id is empty, adding "
                << "new saved search to the model");

        ErrorString errorDescription;
        const auto index = m_savedSearchModel->createSavedSearch(
            savedSearchName, savedSearchQuery, errorDescription);

        if (!index.isValid()) {
            m_ui->statusBar->setText(errorDescription.localizedString());
            QNWARNING("dialog::AddOrEditSavedSearchDialog", errorDescription);
            m_ui->statusBar->setHidden(false);
            return;
        }
    }
    else {
        QNDEBUG(
            "dialog::AddOrEditSavedSearchDialog",
            "Edited saved search local id is not empty, "
                << "editing the existing saved search within the model");

        const auto index =
            m_savedSearchModel->indexForLocalId(m_editedSavedSearchLocalId);

        const auto * item = m_savedSearchModel->itemForIndex(index);
        if (Q_UNLIKELY(!item)) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't edit saved search: saved search was "
                           "not found in the model"));
            return;
        }

        const auto * savedSearchItem = item->cast<SavedSearchItem>();
        if (Q_UNLIKELY(!savedSearchItem)) {
            REPORT_ERROR(
                QT_TR_NOOP("Can't edit saved search: can't cast model item to "
                           "saved search one"));
            return;
        }

        const auto queryIndex = m_savedSearchModel->index(
            index.row(), static_cast<int>(SavedSearchModel::Column::Query),
            index.parent());

        if (savedSearchItem->query() != savedSearchQuery) {
            if (Q_UNLIKELY(
                    !m_savedSearchModel->setData(queryIndex, savedSearchQuery)))
            {
                REPORT_ERROR(
                    QT_TR_NOOP("Failed to set the saved search query "
                               "to the model"));
            }
        }

        // If needed, update the saved search name
        const auto nameIndex = m_savedSearchModel->index(
            index.row(), static_cast<int>(SavedSearchModel::Column::Name),
            index.parent());

        if (savedSearchItem->nameUpper() != savedSearchName.toUpper()) {
            if (Q_UNLIKELY(
                    !m_savedSearchModel->setData(nameIndex, savedSearchName)))
            {
                // Probably the new name collides with some existing
                // saved search's name
                const auto existingItemIndex =
                    m_savedSearchModel->indexForSavedSearchName(
                        savedSearchName);

                if (existingItemIndex.isValid() &&
                    ((existingItemIndex.row() != nameIndex.row()) ||
                     (existingItemIndex.parent() != nameIndex.parent())))
                {
                    // The new name collides with some existing saved search and
                    // now with the currently edited one
                    REPORT_ERROR(
                        QT_TR_NOOP("The saved search name must be "
                                   "unique in case insensitive manner"));
                }
                else {
                    // Don't really know what happened...
                    REPORT_ERROR(
                        QT_TR_NOOP("Can't set this name for the saved "
                                   "search"));
                }
            }
        }
    }

    QDialog::accept();
}

void AddOrEditSavedSearchDialog::onSavedSearchNameEdited(
    const QString & savedSearchName)
{
    QNDEBUG(
        "dialog::AddOrEditSavedSearchDialog",
        "AddOrEditSavedSearchDialog::onSavedSearchNameEdited: "
            << savedSearchName);

    if (Q_UNLIKELY(m_savedSearchModel.isNull())) {
        QNTRACE("dialog::AddOrEditSavedSearchDialog", "No saved search model");
        return;
    }

    const auto index =
        m_savedSearchModel->indexForSavedSearchName(savedSearchName);

    if (index.isValid()) {
        m_ui->statusBar->setText(
            tr("The saved search name must be unique in "
               "case insensitive manner"));

        m_ui->statusBar->setHidden(false);
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
    else {
        m_ui->statusBar->clear();
        m_ui->statusBar->setHidden(true);
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);
    }
}

void AddOrEditSavedSearchDialog::onSearchQueryEdited()
{
    const QString searchQuery = m_ui->searchQueryPlainTextEdit->toPlainText();

    QNDEBUG(
        "dialog::AddOrEditSavedSearchDialog",
        "AddOrEditSavedSearchDialog::onSearchQueryEdited: " << searchQuery);

    ErrorString parseError;
    if (!m_searchQuery->setQueryString(searchQuery, parseError)) {
        ErrorString error{QT_TR_NOOP("Search query string is invalid")};
        error.appendBase(parseError.base());
        error.appendBase(parseError.additionalBases());
        error.details() = parseError.details();
        QNDEBUG("dialog::AddOrEditSavedSearchDialog", error);

        // NOTE: only show the parsing error to user if the query was good
        // before the last edit
        if (m_ui->buttonBox->button(QDialogButtonBox::Ok)->isEnabled()) {
            m_ui->statusBar->setText(error.localizedString());
            m_ui->statusBar->setHidden(false);
            m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        }

        m_searchQuery->clear();
        return;
    }

    m_ui->statusBar->clear();
    m_ui->statusBar->setHidden(true);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    QNTRACE(
        "dialog::AddOrEditSavedSearchDialog",
        "Successfully parsed the note search query: " << *m_searchQuery);
}

void AddOrEditSavedSearchDialog::createConnections()
{
    QObject::connect(
        m_ui->savedSearchNameLineEdit, &QLineEdit::textEdited, this,
        &AddOrEditSavedSearchDialog::onSavedSearchNameEdited);

    QObject::connect(
        m_ui->searchQueryPlainTextEdit, &QPlainTextEdit::textChanged, this,
        &AddOrEditSavedSearchDialog::onSearchQueryEdited);
}

void AddOrEditSavedSearchDialog::setupEditedSavedSearchItem()
{
    QNDEBUG(
        "dialog::AddOrEditSavedSearchDialog",
        "AddOrEditSavedSearchDialog::setupEditedSavedSearchItem");

    if (m_editedSavedSearchLocalId.isEmpty()) {
        QNDEBUG(
            "dialog::AddOrEditSavedSearchDialog",
            "Edited saved search's local id is empty");
        return;
    }

    if (Q_UNLIKELY(m_savedSearchModel.isNull())) {
        QNDEBUG(
            "dialog::AddOrEditSavedSearchDialog", "Saved search model is null");
        return;
    }

    const auto editedSavedSearchIndex =
        m_savedSearchModel->indexForLocalId(m_editedSavedSearchLocalId);

    const auto * item =
        m_savedSearchModel->itemForIndex(editedSavedSearchIndex);

    const auto * savedSearchItem =
        (item ? item->cast<SavedSearchItem>() : nullptr);

    if (Q_UNLIKELY(!savedSearchItem)) {
        m_ui->statusBar->setText(
            tr("Can't find the edited saved search within the model"));
        m_ui->statusBar->setHidden(false);
        return;
    }

    m_ui->savedSearchNameLineEdit->setText(savedSearchItem->name());
    m_ui->searchQueryPlainTextEdit->setPlainText(savedSearchItem->query());
}

} // namespace quentier
