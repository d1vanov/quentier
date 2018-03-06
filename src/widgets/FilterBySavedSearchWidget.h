/*
 * Copyright 2017 Dmitry Ivanov
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

#ifndef QUENTIER_WIDGETS_FILTER_BY_SAVED_SEARCH_WIDGET_H
#define QUENTIER_WIDGETS_FILTER_BY_SAVED_SEARCH_WIDGET_H

#include <quentier/types/Account.h>
#include <QComboBox>
#include <QPointer>
#include <QStringListModel>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(SavedSearchModel)

class FilterBySavedSearchWidget: public QComboBox
{
    Q_OBJECT
public:
    explicit FilterBySavedSearchWidget(QWidget * parent = Q_NULLPTR);

    void switchAccount(const Account & account, SavedSearchModel * pSavedSearchModel);

    const SavedSearchModel * savedSearchModel() const;

    bool isReady() const;

Q_SIGNALS:
    void ready();

private Q_SLOTS:
    void onAllSavedSearchesListed();

    void onModelRowsInserted(const QModelIndex & parent, int start, int end);
    void onModelRowsRemoved(const QModelIndex & parent, int start, int end);

    void onModelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                            );
#else
                            , const QVector<int> & roles = QVector<int>());
#endif

    void persistSelectedSavedSearch();
    void restoreSelectedSavedSearch();

    void onCurrentSavedSearchChanged(const QString & savedSearchName);

private:
    void updateSavedSearchesInComboBox();
    void connectoToSavedSearchModel();

private:
    Account                     m_account;
    QPointer<SavedSearchModel>  m_pSavedSearchModel;
    QStringListModel            m_availableSavedSearchesModel;

    QString                     m_currentSavedSearchName;
    QString                     m_currentSavedSearchLocalUid;

    bool                        m_settingCurrentIndex;

    bool                        m_isReady;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_FILTER_BY_SAVED_SEARCH_WIDGET_H
