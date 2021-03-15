/*
 * Copyright 2020-2021 Dmitry Ivanov
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

namespace quentier {

/**
 * @brief The FilterBySearchStringWidget class is a widget controlling
 * the line edit containing the search string. That search string can either
 * come from user as a regular search query or it can come from the saved search
 * as its saved query. In the latter case the widget overrides the previously
 * displayed search query (if any) with the query from the saved search. Then
 * the original query can be restored.
 */
class FilterBySearchStringWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit FilterBySearchStringWidget(QWidget * parent = nullptr);
    ~FilterBySearchStringWidget() override;

    /**
     * @return search query displayed by the widget at the moment: either the
     *         one entered manually by user or the one from the saved search
     */
    [[nodiscard]] QString displayedSearchQuery() const;

    /**
     * @return search query corresponding to the manual search string entered
     *         by user
     */
    [[nodiscard]] const QString & searchQuery() const noexcept
    {
        return m_searchQuery;
    }

    /**
     * @return search query from the saved search set to the widget (if any)
     */
    [[nodiscard]] const QString & savedSearchQuery() const noexcept
    {
        return m_savedSearchQuery;
    }

    /**
     * @return local id of the saved search which query the widget displays
     *         at the moment; empty if the widget doesn't display saved search
     *         query at the moment
     */
    [[nodiscard]] const QString & savedSearchLocalId() const noexcept
    {
        return m_savedSearchLocalId;
    }

    [[nodiscard]] bool displaysSavedSearchQuery() const noexcept
    {
        return !m_savedSearchLocalId.isEmpty();
    }

    void setSearchQuery(const QString & searchQuery);
    void setSavedSearch(const QString & localId, const QString & searchQuery);

    void clearSavedSearch();

Q_SIGNALS:
    void searchQueryChanged(QString query);
    void searchSavingRequested(QString query);
    void savedSearchQueryChanged(QString savedSearchLocalId, QString query);

private Q_SLOTS:
    void onLineEditTextEdited(const QString & text);
    void onLineEditEditingFinished();
    void onSaveButtonPressed();

private:
    void createConnections();
    void updateDisplayedSearchQuery();
    void notifyQueryChanged();

private:
    Ui::FilterBySearchStringWidget * m_pUi;

    QString m_searchQuery;

    QString m_savedSearchQuery;
    QString m_savedSearchLocalId;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_FILTER_BY_SEARCH_STRING_WIDGET_H
