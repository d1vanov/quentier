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

#ifndef QUENTIER_LIB_WIDGET_FILTER_BY_SEARCH_STRING_WIDGET_H
#define QUENTIER_LIB_WIDGET_FILTER_BY_SEARCH_STRING_WIDGET_H

#include <QString>
#include <QWidget>

namespace Ui {
class FilterBySearchStringWidget;
}

/**
 * @brief The FilterBySearchStringWidget class is a widget controlling
 * the line edit containing the search string. That search string can either
 * come from user as a regular search query or it can come from the saved search
 * as its saved query. In the latter case the widget overrides the previously
 * displayed search query (if any) with the query from the saved search. Then
 * the original query can be restored.
 */
class FilterBySearchStringWidget final: public QWidget
{
    Q_OBJECT
public:
    explicit FilterBySearchStringWidget(QWidget * parent = nullptr);
    virtual ~FilterBySearchStringWidget() override;

    QString displayedSearchQuery() const;

    const QString & originalSearchQuery() const
    {
        return m_searchQuery;
    }

    const QString & savedSearchQuery() const
    {
        return m_savedSearchQuery;
    }

    const QString & savedSearchLocalUid() const
    {
        return m_savedSearchLocalUid;
    }

    bool isSavedSearchQuery() const
    {
        return !m_savedSearchLocalUid.isEmpty();
    }

    void setSavedSearch(const QString & localUid, const QString & searchQuery);

    void clearSavedSearch();

Q_SIGNALS:
    void searchQueryChanged(QString query);
    void savedSearchQueryChanged(QString savedSearchLocalUid, QString query);

private Q_SLOTS:
    void onLineEditTextEdited(const QString & text);
    void onLineEditEditingFinished();

private:
    void createConnections();
    void updateDisplayedSearchQuery();
    void notifyQueryChanged();

private:
    Ui::FilterBySearchStringWidget * m_pUi;

    QString     m_searchQuery;

    QString     m_savedSearchQuery;
    QString     m_savedSearchLocalUid;
};

#endif // QUENTIER_LIB_WIDGET_FILTER_BY_SEARCH_STRING_WIDGET_H
