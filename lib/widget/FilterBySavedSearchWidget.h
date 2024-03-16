/*
 * Copyright 2017-2024 Dmitry Ivanov
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

#pragma once

#include <quentier/types/Account.h>

#include <QComboBox>
#include <QList>
#include <QPointer>
#include <QStringListModel>

class QModelIndex;

namespace quentier {

class SavedSearchModel;

class FilterBySavedSearchWidget : public QComboBox
{
    Q_OBJECT
public:
    explicit FilterBySavedSearchWidget(QWidget * parent = nullptr);

    ~FilterBySavedSearchWidget() override;

    void switchAccount(Account account, SavedSearchModel * savedSearchModel);

    [[nodiscard]] SavedSearchModel * savedSearchModel();
    [[nodiscard]] const SavedSearchModel * savedSearchModel() const;

    [[nodiscard]] bool isReady() const noexcept;

    [[nodiscard]] QString filteredSavedSearchLocalId() const;
    void setCurrentSavedSearchLocalId(const QString & savedSearchLocalId);

Q_SIGNALS:
    void ready();
    void currentSavedSearchNameChanged(const QString & savedSearchName);

private Q_SLOTS:
    void onAllSavedSearchesListed();

    void onModelRowsInserted(const QModelIndex & parent, int start, int end);
    void onModelRowsRemoved(const QModelIndex & parent, int start, int end);

    void onModelDataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = {});

    void persistSelectedSavedSearch();
    void restoreSelectedSavedSearch();

    void onCurrentSavedSearchChanged(int index);

private:
    void updateSavedSearchesInComboBox();
    void connectoToSavedSearchModel();

    void connectToCurrentIndexChangedSignal();
    void disconnectFromCurrentIndexChangedSignal();

private:
    Account m_account;
    QPointer<SavedSearchModel> m_savedSearchModel;
    QStringListModel m_availableSavedSearchesModel;

    QString m_currentSavedSearchName;
    QString m_currentSavedSearchLocalId;

    bool m_isReady = false;
};

} // namespace quentier
