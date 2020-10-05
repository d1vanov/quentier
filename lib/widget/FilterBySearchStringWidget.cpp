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

FilterBySearchStringWidget::FilterBySearchStringWidget(QWidget *parent) :
    QWidget(parent),
    m_pUi(new Ui::FilterBySearchStringWidget)
{
    m_pUi->setupUi(this);
}

FilterBySearchStringWidget::~FilterBySearchStringWidget()
{
    delete m_pUi;
}

QString FilterBySearchStringWidget::displayedSearchQuery() const
{
    return m_pUi->lineEdit->text();
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

void FilterBySearchStringWidget::updateDisplayedSearchQuery()
{
    QNDEBUG(
        "widget:filter_search_string",
        "FilterBySearchStringWidget::updateDisplayedSearchQuery");

    // TODO: implement
}
