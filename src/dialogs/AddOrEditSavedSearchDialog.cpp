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

#include "AddOrEditSavedSearchDialog.h"
#include "ui_AddOrEditSavedSearchDialog.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/local_storage/NoteSearchQuery.h>
#include <QPushButton>

namespace quentier {

AddOrEditSavedSearchDialog::AddOrEditSavedSearchDialog(SavedSearchModel * pSavedSearchModel,
                                                       QWidget * parent,
                                                       const QString & editedSavedSearchLocalUid) :
    QDialog(parent),
    m_pUi(new Ui::AddOrEditSavedSearchDialog),
    m_pSavedSearchModel(pSavedSearchModel),
    m_pSearchQuery(new NoteSearchQuery),
    m_editedSavedSearchLocalUid(editedSavedSearchLocalUid),
    m_stringUtils()
{
    m_pUi->setupUi(this);
    m_pUi->statusBar->setHidden(true);
    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    setupEditedSavedSearchItem();

    createConnections();

    if (!m_editedSavedSearchLocalUid.isEmpty()) {
        m_pUi->searchQueryPlainTextEdit->setFocus();
    }
    else {
        m_pUi->savedSearchNameLineEdit->setFocus();
    }
}

AddOrEditSavedSearchDialog::~AddOrEditSavedSearchDialog()
{
    delete m_pUi;
}

void AddOrEditSavedSearchDialog::setQuery(const QString & query)
{
    QNDEBUG(QStringLiteral("AddOrEditSavedSearchDialog::setQuery: ") << query);
    m_pUi->searchQueryPlainTextEdit->setPlainText(query);
}

void AddOrEditSavedSearchDialog::accept()
{
    QString savedSearchName = m_pUi->savedSearchNameLineEdit->text().trimmed();
    m_stringUtils.removeNewlines(savedSearchName);

    QString savedSearchQuery = m_pSearchQuery->queryString();
    bool queryIsEmpty = m_pSearchQuery->isEmpty();

    QNDEBUG(QStringLiteral("AddOrEditSavedSearchDialog::accept: name = ") << savedSearchName
            << QStringLiteral(", query = ") << savedSearchQuery << QStringLiteral(", query is empty = ")
            << (queryIsEmpty ? QStringLiteral("true") : QStringLiteral("false")));

#define REPORT_ERROR(error) \
    m_pUi->statusBar->setText(tr(error)); \
    QNWARNING(error); \
    m_pUi->statusBar->setHidden(false)

    if (Q_UNLIKELY(m_pSavedSearchModel.isNull())) {
        REPORT_ERROR("Can't accept new saved search or edit the existing one: saved search model is gone");
        return;
    }

    if (m_editedSavedSearchLocalUid.isEmpty())
    {
        QNDEBUG(QStringLiteral("Edited saved search local uid is empty, adding new saved search to the model"));

        ErrorString errorDescription;
        QModelIndex index = m_pSavedSearchModel->createSavedSearch(savedSearchName, savedSearchQuery, errorDescription);
        if (!index.isValid()) {
            m_pUi->statusBar->setText(errorDescription.localizedString());
            QNWARNING(errorDescription);
            m_pUi->statusBar->setHidden(false);
            return;
        }
    }
    else
    {
        QNDEBUG(QStringLiteral("Edited saved search local uid is not empty, editing "
                               "the existing saved search within the model"));

        QModelIndex index = m_pSavedSearchModel->indexForLocalUid(m_editedSavedSearchLocalUid);
        const SavedSearchModelItem * pItem = m_pSavedSearchModel->itemForIndex(index);
        if (Q_UNLIKELY(!pItem)) {
            REPORT_ERROR("Can't edit saved search: saved search was not found in the model");
            return;
        }

        QModelIndex queryIndex = m_pSavedSearchModel->index(index.row(), SavedSearchModel::Columns::Query,
                                                            index.parent());
        if (pItem->m_query != savedSearchQuery)
        {
            bool res = m_pSavedSearchModel->setData(queryIndex, savedSearchQuery);
            if (Q_UNLIKELY(!res)) {
                REPORT_ERROR("Failed to set the saved search query to the model");
            }
        }

        // If needed, update the saved search name
        QModelIndex nameIndex = m_pSavedSearchModel->index(index.row(), SavedSearchModel::Columns::Name,
                                                           index.parent());
        if (pItem->nameUpper() != savedSearchName.toUpper())
        {
            bool res = m_pSavedSearchModel->setData(nameIndex, savedSearchName);
            if (Q_UNLIKELY(!res))
            {
                // Probably the new name collides with some existing saved search's name
                QModelIndex existingItemIndex = m_pSavedSearchModel->indexForSavedSearchName(savedSearchName);
                if (existingItemIndex.isValid() &&
                    ((existingItemIndex.row() != nameIndex.row()) ||
                     (existingItemIndex.parent() != nameIndex.parent())))
                {
                    // The new name collides wih some existing saved search and now with the currently edited one
                    REPORT_ERROR("The saved search name must be unique in case insensitive manner");
                }
                else
                {
                    // Don't really know what happened...
                    REPORT_ERROR("Can't set this name for the saved search");
                }
            }
        }
    }

    QDialog::accept();
}

void AddOrEditSavedSearchDialog::onSavedSearchNameEdited(const QString & savedSearchName)
{
    QNDEBUG(QStringLiteral("AddOrEditSavedSearchDialog::onSavedSearchNameEdited: ") << savedSearchName);

    if (Q_UNLIKELY(m_pSavedSearchModel.isNull())) {
        QNTRACE(QStringLiteral("No saved search model"));
        return;
    }

    QModelIndex index = m_pSavedSearchModel->indexForSavedSearchName(savedSearchName);
    if (index.isValid())
    {
        m_pUi->statusBar->setText(tr("The saved search name must be unique in case insensitive manner"));
        m_pUi->statusBar->setHidden(false);
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    }
    else
    {
        m_pUi->statusBar->clear();
        m_pUi->statusBar->setHidden(true);
        m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(false);
    }
}

void AddOrEditSavedSearchDialog::onSearchQueryEdited()
{
    QString searchQuery = m_pUi->searchQueryPlainTextEdit->toPlainText();

    QNDEBUG(QStringLiteral("AddOrEditSavedSearchDialog::onSearchQueryEdited: ") << searchQuery);

    ErrorString parseError;
    bool res = m_pSearchQuery->setQueryString(searchQuery, parseError);
    if (!res)
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't parse search query string"));
        error.additionalBases().append(parseError.base());
        error.additionalBases().append(parseError.additionalBases());
        error.details() = parseError.details();
        QNDEBUG(error);

        // NOTE: only show the parsing error to user if the query was good before the last edit
        if (m_pUi->buttonBox->button(QDialogButtonBox::Ok)->isEnabled()) {
            m_pUi->statusBar->setText(error.localizedString());
            m_pUi->statusBar->setHidden(false);
            m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        }

        m_pSearchQuery->clear();
        return;
    }

    m_pUi->statusBar->clear();
    m_pUi->statusBar->setHidden(true);
    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    QNTRACE(QStringLiteral("Successfully parsed the note search query: ") << *m_pSearchQuery);
}

void AddOrEditSavedSearchDialog::createConnections()
{
    QObject::connect(m_pUi->savedSearchNameLineEdit, QNSIGNAL(QLineEdit,textEdited,const QString&),
                     this, QNSLOT(AddOrEditSavedSearchDialog,onSavedSearchNameEdited,const QString&));
    QObject::connect(m_pUi->searchQueryPlainTextEdit, QNSIGNAL(QPlainTextEdit,textChanged),
                     this, QNSLOT(AddOrEditSavedSearchDialog,onSearchQueryEdited));
}

void AddOrEditSavedSearchDialog::setupEditedSavedSearchItem()
{
    QNDEBUG(QStringLiteral("AddOrEditSavedSearchDialog::setupEditedSavedSearchItem"));

    if (m_editedSavedSearchLocalUid.isEmpty()) {
        QNDEBUG(QStringLiteral("Edited saved search's local uid is empty"));
        return;
    }

    if (Q_UNLIKELY(m_pSavedSearchModel.isNull())) {
        QNDEBUG(QStringLiteral("Saved search model is null"));
        return;
    }

    QModelIndex editedSavedSearchIndex = m_pSavedSearchModel->indexForLocalUid(m_editedSavedSearchLocalUid);
    const SavedSearchModelItem * pItem = m_pSavedSearchModel->itemForIndex(editedSavedSearchIndex);
    if (Q_UNLIKELY(!pItem)) {
        m_pUi->statusBar->setText(tr("Can't find the edited saved search within the model"));
        m_pUi->statusBar->setHidden(false);
        return;
    }

    m_pUi->savedSearchNameLineEdit->setText(pItem->m_name);
    m_pUi->searchQueryPlainTextEdit->setPlainText(pItem->m_query);
}

} // namespace quentier
