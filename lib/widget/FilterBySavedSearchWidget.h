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

#ifndef QUENTIER_LIB_WIDGET_FILTER_BY_SAVED_SEARCH_WIDGET_H
#define QUENTIER_LIB_WIDGET_FILTER_BY_SAVED_SEARCH_WIDGET_H

#include <quentier/types/Account.h>

#include <QComboBox>
#include <QPointer>
#include <QStringListModel>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SavedSearchModel)

class FilterBySavedSearchWidget : public QComboBox
{
    Q_OBJECT
public:
    explicit FilterBySavedSearchWidget(QWidget * parent = nullptr);

    virtual ~FilterBySavedSearchWidget() override;

    void switchAccount(
        const Account & account, SavedSearchModel * pSavedSearchModel);

    SavedSearchModel * savedSearchModel();
    const SavedSearchModel * savedSearchModel() const;

    bool isReady() const;

    QString filteredSavedSearchLocalUid() const;

    void setCurrentSavedSearchLocalUid(const QString & savedSearchLocalUid);

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
    QPointer<SavedSearchModel> m_pSavedSearchModel;
    QStringListModel m_availableSavedSearchesModel;

    QString m_currentSavedSearchName;
    QString m_currentSavedSearchLocalUid;

    bool m_isReady = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_FILTER_BY_SAVED_SEARCH_WIDGET_H
