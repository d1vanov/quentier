/*
 * Copyright 2020-2024 Dmitry Ivanov
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
    QWidget{parent}, m_ui{new Ui::FilterBySearchStringWidget}
{
    m_ui->setupUi(this);
    m_ui->saveSearchButton->setEnabled(false);
    createConnections();
}

FilterBySearchStringWidget::~FilterBySearchStringWidget()
{
    delete m_ui;
}

QString FilterBySearchStringWidget::displayedSearchQuery() const
{
    return m_ui->lineEdit->text();
}

void FilterBySearchStringWidget::setSearchQuery(QString searchQuery)
{
    QNDEBUG(
        "widget::FilterBySearchStringWidget",
        "FilterBySearchStringWidget::setSearchQuery: " << searchQuery);

    if (!m_savedSearchLocalId.isEmpty()) {
        m_searchQuery = std::move(searchQuery);
        return;
    }

    if (m_searchQuery == searchQuery) {
        return;
    }

    m_searchQuery = std::move(searchQuery);
    m_ui->lineEdit->setText(m_searchQuery);
    m_ui->saveSearchButton->setEnabled(!m_searchQuery.isEmpty());
}

void FilterBySearchStringWidget::setSavedSearch(
    const QString & localId, const QString & searchQuery)
{
    QNDEBUG(
        "widget::FilterBySearchStringWidget",
        "FilterBySearchStringWidget::setSavedSearch: local id = "
            << localId << ", query = " << searchQuery);

    if (m_savedSearchLocalId == localId && m_savedSearchQuery == searchQuery) {
        QNDEBUG(
            "widget::FilterBySearchStringWidget",
            "Same saved search, same query");
        return;
    }

    m_savedSearchLocalId = localId;
    m_savedSearchQuery = searchQuery;
    updateDisplayedSearchQuery();
}

void FilterBySearchStringWidget::clearSavedSearch()
{
    QNDEBUG(
        "widget::FilterBySearchStringWidget",
        "FilterBySearchStringWidget::clearSavedSearch");

    if (m_savedSearchLocalId.isEmpty() && m_savedSearchQuery.isEmpty()) {
        QNDEBUG("widget::FilterBySearchStringWidget", "Already cleared");
        return;
    }

    m_savedSearchQuery.clear();
    m_savedSearchLocalId.clear();
    updateDisplayedSearchQuery();
}

void FilterBySearchStringWidget::onLineEditTextEdited(QString text)
{
    QNDEBUG(
        "widget::FilterBySearchStringWidget",
        "FilterBySearchStringWidget::onLineEditTextEdited: " << text);

    QString & searchQuery =
        (m_savedSearchLocalId.isEmpty() ? m_searchQuery : m_savedSearchQuery);

    const bool wasEmpty = searchQuery.isEmpty();
    searchQuery = std::move(text);
    const bool isEmpty = searchQuery.isEmpty();

    m_ui->saveSearchButton->setEnabled(!isEmpty);

    if (!wasEmpty && isEmpty) {
        notifyQueryChanged();
    }
}

void FilterBySearchStringWidget::onLineEditEditingFinished()
{
    const QString displayedQuery = m_ui->lineEdit->text();

    QNDEBUG(
        "widget::FilterBySearchStringWidget",
        "FilterBySearchStringWidget::onLineEditEditingFinished: "
            << displayedQuery);

    const QString & searchQuery =
        (m_savedSearchLocalId.isEmpty() ? m_searchQuery : m_savedSearchQuery);

    const bool displayedQueryIsEmpty = displayedQuery.isEmpty();

    if (searchQuery.isEmpty() && displayedQueryIsEmpty) {
        // This situation should have already been processed
        return;
    }

    m_ui->saveSearchButton->setEnabled(!displayedQueryIsEmpty);
    notifyQueryChanged();
}

void FilterBySearchStringWidget::onSaveButtonPressed()
{
    QNDEBUG(
        "widget::FilterBySearchStringWidget",
        "FilterBySearchStringWidget::onSaveButtonPressed");

    if (!m_savedSearchLocalId.isEmpty()) {
        Q_EMIT savedSearchQueryChanged(
            m_savedSearchLocalId, m_savedSearchQuery);
        return;
    }

    Q_EMIT searchSavingRequested(m_searchQuery);
}

void FilterBySearchStringWidget::createConnections()
{
    QObject::connect(
        m_ui->lineEdit, &QLineEdit::editingFinished, this,
        &FilterBySearchStringWidget::onLineEditEditingFinished);

    QObject::connect(
        m_ui->lineEdit, &QLineEdit::textEdited, this,
        &FilterBySearchStringWidget::onLineEditTextEdited);

    QObject::connect(
        m_ui->saveSearchButton, &QPushButton::clicked, this,
        &FilterBySearchStringWidget::onSaveButtonPressed);
}

void FilterBySearchStringWidget::updateDisplayedSearchQuery()
{
    QNDEBUG(
        "widget::FilterBySearchStringWidget",
        "FilterBySearchStringWidget::updateDisplayedSearchQuery");

    if (!m_savedSearchLocalId.isEmpty()) {
        QNTRACE(
            "widget::FilterBySearchStringWidget",
            "Setting saved search "
                << m_savedSearchLocalId
                << "query to line edit: " << m_savedSearchQuery);

        m_ui->lineEdit->setText(m_savedSearchQuery);
    }
    else {
        QNTRACE(
            "widget::FilterBySearchStringWidget",
            "Setting search string to line edit: " << m_searchQuery);

        m_ui->lineEdit->setText(m_searchQuery);
    }

    m_ui->saveSearchButton->setEnabled(!m_ui->lineEdit->text().isEmpty());
}

void FilterBySearchStringWidget::notifyQueryChanged()
{
    QNDEBUG(
        "widget::FilterBySearchStringWidget",
        "FilterBySearchStringWidget::notifyQueryChanged");

    if (m_savedSearchLocalId.isEmpty()) {
        Q_EMIT searchQueryChanged(m_searchQuery);
    }
    else {
        Q_EMIT savedSearchQueryChanged(
            m_savedSearchLocalId, m_savedSearchQuery);
    }
}

} // namespace quentier
