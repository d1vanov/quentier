/*
 * Copyright 2020 Dmitry Ivanov
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

#include "FilterBySearchStringWidget.h"
#include "ui_FilterBySearchStringWidget.h"

#include <quentier/logging/QuentierLogger.h>

namespace quentier {

FilterBySearchStringWidget::FilterBySearchStringWidget(QWidget * parent) :
    QWidget(parent), m_pUi(new Ui::FilterBySearchStringWidget)
{
    m_pUi->setupUi(this);
    m_pUi->saveSearchButton->setEnabled(false);
    createConnections();
}

FilterBySearchStringWidget::~FilterBySearchStringWidget()
{
    delete m_pUi;
}

QString FilterBySearchStringWidget::displayedSearchQuery() const
{
    return m_pUi->lineEdit->text();
}

void FilterBySearchStringWidget::setSearchQuery(const QString & searchQuery)
{
    QNDEBUG(
        "widget:filter_search_string",
        "FilterBySearchStringWidget::setSearchQuery: " << searchQuery);

    if (!m_savedSearchLocalUid.isEmpty()) {
        m_searchQuery = searchQuery;
        return;
    }

    if (m_searchQuery == searchQuery) {
        return;
    }

    m_searchQuery = searchQuery;
    m_pUi->lineEdit->setText(m_searchQuery);
    m_pUi->saveSearchButton->setEnabled(!m_searchQuery.isEmpty());
}

void FilterBySearchStringWidget::setSavedSearch(
    const QString & localUid, const QString & searchQuery)
{
    QNDEBUG(
        "widget:filter_search_string",
        "FilterBySearchStringWidget::setSavedSearch: local uid = "
            << localUid << ", query = " << searchQuery);

    if (m_savedSearchLocalUid == localUid && m_savedSearchQuery == searchQuery)
    {
        QNDEBUG("widget:filter_search_string", "Same saved search, same query");
        return;
    }

    m_savedSearchLocalUid = localUid;
    m_savedSearchQuery = searchQuery;
    updateDisplayedSearchQuery();
}

void FilterBySearchStringWidget::clearSavedSearch()
{
    QNDEBUG(
        "widget:filter_search_string",
        "FilterBySearchStringWidget::clearSavedSearch");

    if (m_savedSearchLocalUid.isEmpty() && m_savedSearchQuery.isEmpty()) {
        QNDEBUG("widget:filter_search_string", "Already cleared");
        return;
    }

    m_savedSearchQuery.clear();
    m_savedSearchLocalUid.clear();
    updateDisplayedSearchQuery();
}

void FilterBySearchStringWidget::onLineEditTextEdited(const QString & text)
{
    QNDEBUG(
        "widget:filter_search_string",
        "FilterBySearchStringWidget::onLineEditTextEdited: " << text);

    QString & searchQuery =
        (m_savedSearchLocalUid.isEmpty() ? m_searchQuery : m_savedSearchQuery);

    bool wasEmpty = searchQuery.isEmpty();
    searchQuery = text;
    bool isEmpty = searchQuery.isEmpty();

    m_pUi->saveSearchButton->setEnabled(!isEmpty);

    if (!wasEmpty && isEmpty) {
        notifyQueryChanged();
    }
}

void FilterBySearchStringWidget::onLineEditEditingFinished()
{
    const QString displayedQuery = m_pUi->lineEdit->text();

    QNDEBUG(
        "widget:filter_search_string",
        "FilterBySearchStringWidget::onLineEditEditingFinished: "
            << displayedQuery);

    const QString & searchQuery =
        (m_savedSearchLocalUid.isEmpty() ? m_searchQuery : m_savedSearchQuery);

    const bool displayedQueryIsEmpty = displayedQuery.isEmpty();

    if (searchQuery.isEmpty() && displayedQueryIsEmpty) {
        // This situation should have already been processed
        return;
    }

    m_pUi->saveSearchButton->setEnabled(!displayedQueryIsEmpty);
    notifyQueryChanged();
}

void FilterBySearchStringWidget::onSaveButtonPressed()
{
    QNDEBUG(
        "widget:filter_search_string",
        "FilterBySearchStringWidget::onSaveButtonPressed");

    if (!m_savedSearchLocalUid.isEmpty()) {
        Q_EMIT savedSearchQueryChanged(
            m_savedSearchLocalUid, m_savedSearchQuery);
        return;
    }

    Q_EMIT searchSavingRequested(m_searchQuery);
}

void FilterBySearchStringWidget::createConnections()
{
    QObject::connect(
        m_pUi->lineEdit, &QLineEdit::editingFinished, this,
        &FilterBySearchStringWidget::onLineEditEditingFinished);

    QObject::connect(
        m_pUi->lineEdit, &QLineEdit::textEdited, this,
        &FilterBySearchStringWidget::onLineEditTextEdited);

    QObject::connect(
        m_pUi->saveSearchButton, &QPushButton::clicked, this,
        &FilterBySearchStringWidget::onSaveButtonPressed);
}

void FilterBySearchStringWidget::updateDisplayedSearchQuery()
{
    QNDEBUG(
        "widget:filter_search_string",
        "FilterBySearchStringWidget::updateDisplayedSearchQuery");

    if (!m_savedSearchLocalUid.isEmpty()) {
        QNTRACE(
            "widget:filter_search_string",
            "Setting saved search "
                << m_savedSearchLocalUid
                << "query to line edit: " << m_savedSearchQuery);

        m_pUi->lineEdit->setText(m_savedSearchQuery);
    }
    else {
        QNTRACE(
            "widget:filter_search_string",
            "Setting search string to line edit: " << m_searchQuery);

        m_pUi->lineEdit->setText(m_searchQuery);
    }

    m_pUi->saveSearchButton->setEnabled(!m_pUi->lineEdit->text().isEmpty());
}

void FilterBySearchStringWidget::notifyQueryChanged()
{
    QNDEBUG(
        "widget:filter_search_string",
        "FilterBySearchStringWidget::notifyQueryChanged");

    if (m_savedSearchLocalUid.isEmpty()) {
        Q_EMIT searchQueryChanged(m_searchQuery);
    }
    else {
        Q_EMIT savedSearchQueryChanged(
            m_savedSearchLocalUid, m_savedSearchQuery);
    }
}

} // namespace quentier
