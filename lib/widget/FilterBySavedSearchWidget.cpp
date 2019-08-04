/*
 * Copyright 2017-2019 Dmitry Ivanov
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

#include "FilterBySavedSearchWidget.h"

#include <lib/model/SavedSearchModel.h>
#include <lib/preferences/SettingsNames.h>

#include <quentier/utility/ApplicationSettings.h>
#include <quentier/logging/QuentierLogger.h>

#include <QModelIndex>

#include <algorithm>

#define LAST_FILTERED_SAVED_SEARCH_KEY                                         \
    QStringLiteral("LastSelectedSavedSearchLocalUid")                          \
// LAST_FILTERED_SAVED_SEARCH_KEY

namespace quentier {

FilterBySavedSearchWidget::FilterBySavedSearchWidget(QWidget * parent) :
    QComboBox(parent),
    m_account(),
    m_pSavedSearchModel(),
    m_availableSavedSearchesModel(),
    m_currentSavedSearchName(),
    m_currentSavedSearchLocalUid(),
    m_settingCurrentIndex(false),
    m_isReady(false)
{
    setModel(&m_availableSavedSearchesModel);

    QObject::connect(this, SIGNAL(currentIndexChanged(QString)),
                     this, SLOT(onCurrentSavedSearchChanged(QString)));
}

void FilterBySavedSearchWidget::switchAccount(
    const Account & account, SavedSearchModel * pSavedSearchModel)
{
    QNDEBUG("FilterBySavedSearchWidget::switchAccount: " << account);

    if (!m_pSavedSearchModel.isNull() &&
        (m_pSavedSearchModel.data() != pSavedSearchModel))
    {
        QObject::disconnect(m_pSavedSearchModel.data(),
                            QNSIGNAL(SavedSearchModel,rowsInserted,
                                     const QModelIndex&,int,int),
                            this,
                            QNSLOT(FilterBySavedSearchWidget,onModelRowsInserted,
                                   const QModelIndex&,int,int));
        QObject::disconnect(m_pSavedSearchModel.data(),
                            QNSIGNAL(SavedSearchModel,rowsRemoved,
                                     const QModelIndex&,int,int),
                            this,
                            QNSLOT(FilterBySavedSearchWidget,onModelRowsRemoved,
                                   const QModelIndex&,int,int));
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
        QObject::disconnect(m_pSavedSearchModel.data(),
                            SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                            this,
                            SLOT(onModelDataChanged(QModelIndex,QModelIndex)));
#else
        QObject::disconnect(m_pSavedSearchModel.data(),
                            &SavedSearchModel::dataChanged,
                            this,
                            &FilterBySavedSearchWidget::onModelDataChanged);
#endif
    }

    m_pSavedSearchModel = pSavedSearchModel;

    if (!m_pSavedSearchModel.isNull() &&
        !m_pSavedSearchModel->allSavedSearchesListed())
    {
        QObject::connect(m_pSavedSearchModel.data(),
                         QNSIGNAL(SavedSearchModel,notifyAllSavedSearchesListed),
                         this,
                         QNSLOT(FilterBySavedSearchWidget,onAllSavedSearchesListed));
    }
    else
    {
        connectoToSavedSearchModel();
    }

    if (m_account == account) {
        QNDEBUG("Already set this account");
        return;
    }

    persistSelectedSavedSearch();

    m_account = account;

    m_currentSavedSearchName.resize(0);
    m_currentSavedSearchLocalUid.resize(0);

    if (Q_UNLIKELY(m_pSavedSearchModel.isNull())) {
        QNTRACE("The new model is null");
        m_availableSavedSearchesModel.setStringList(QStringList());
        return;
    }

    m_isReady = false;

    if (m_pSavedSearchModel->allSavedSearchesListed()) {
        restoreSelectedSavedSearch();
        updateSavedSearchesInComboBox();
        m_isReady = true;
        Q_EMIT ready();
    }
}

const SavedSearchModel * FilterBySavedSearchWidget::savedSearchModel() const
{
    if (m_pSavedSearchModel.isNull()) {
        return Q_NULLPTR;
    }

    return m_pSavedSearchModel.data();
}

bool FilterBySavedSearchWidget::isReady() const
{
    return m_isReady;
}

QString FilterBySavedSearchWidget::filteredSavedSearchLocalUid() const
{
    if (isReady()) {
        return m_currentSavedSearchLocalUid;
    }

    if (m_account.isEmpty()) {
        return QString();
    }

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("SavedSearchFilter"));
    QString savedSearchLocalUid =
        appSettings.value(LAST_FILTERED_SAVED_SEARCH_KEY).toString();
    appSettings.endGroup();

    return savedSearchLocalUid;
}

void FilterBySavedSearchWidget::onAllSavedSearchesListed()
{
    QNDEBUG("FilterBySavedSearchWidget::onAllSavedSearchesListed");

    QObject::disconnect(m_pSavedSearchModel.data(),
                        QNSIGNAL(SavedSearchModel,notifyAllSavedSearchesListed),
                        this,
                        QNSLOT(FilterBySavedSearchWidget,onAllSavedSearchesListed));
    connectoToSavedSearchModel();
    restoreSelectedSavedSearch();
    updateSavedSearchesInComboBox();
    m_isReady = true;
    Q_EMIT ready();
}

void FilterBySavedSearchWidget::onModelRowsInserted(
    const QModelIndex & parent, int start, int end)
{
    QNDEBUG("FilterBySavedSearchWidget::onModelRowsInserted");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    updateSavedSearchesInComboBox();
}

void FilterBySavedSearchWidget::onModelRowsRemoved(
    const QModelIndex & parent, int start, int end)
{
    QNDEBUG("FilterBySavedSearchWidget::onModelRowsRemoved");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    updateSavedSearchesInComboBox();
}

void FilterBySavedSearchWidget::onModelDataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    )
#else
    , const QVector<int> & roles)
#endif
{
    QNDEBUG("FilterBySavedSearchWidget::onModelDataChanged");

    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    Q_UNUSED(roles)
#endif

    updateSavedSearchesInComboBox();
}

void FilterBySavedSearchWidget::persistSelectedSavedSearch()
{
    QNDEBUG("FilterBySavedSearchWidget::persistSelectedSavedSearch: account = "
            << m_account);

    if (m_account.isEmpty()) {
        QNDEBUG("The account is empty, nothing to persist");
        return;
    }

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("SavedSearchFilter"));
    appSettings.setValue(
        LAST_FILTERED_SAVED_SEARCH_KEY, m_currentSavedSearchLocalUid);
    appSettings.endGroup();

    QNTRACE("Successfully persisted the last selected saved search in the filter: "
            << m_currentSavedSearchLocalUid);
}

void FilterBySavedSearchWidget::restoreSelectedSavedSearch()
{
    QNDEBUG("FilterBySavedSearchWidget::restoreSelectedSavedSearch");

    if (m_account.isEmpty()) {
        QNDEBUG("The account is empty, nothing to restore");
        return;
    }

    if (Q_UNLIKELY(m_pSavedSearchModel.isNull())) {
        QNDEBUG("The saved search model is null, can't restore anything");
        return;
    }

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(QStringLiteral("SavedSearchFilter"));
    m_currentSavedSearchLocalUid =
        appSettings.value(LAST_FILTERED_SAVED_SEARCH_KEY).toString();
    appSettings.endGroup();

    if (m_currentSavedSearchLocalUid.isEmpty()) {
        QNDEBUG("No last selected saved search local uid, nothing to restore");
        return;
    }
}

void FilterBySavedSearchWidget::onCurrentSavedSearchChanged(
    const QString & savedSearchName)
{
    QNDEBUG("FilterBySavedSearchWidget::onCurrentSavedSearchChanged: "
            << savedSearchName);

    if (m_settingCurrentIndex) {
        QNTRACE("This time the current index was set programmatically, won't "
                "do anything");
        return;
    }

    if (Q_UNLIKELY(m_pSavedSearchModel.isNull())) {
        QNDEBUG("The saved search model is null, won't do anything");
        return;
    }

    if (savedSearchName.isEmpty())
    {
        m_currentSavedSearchName.resize(0);
        m_currentSavedSearchLocalUid.resize(0);
    }
    else
    {
        m_currentSavedSearchLocalUid = m_pSavedSearchModel->localUidForItemName(
            savedSearchName, /* linked notebook guid = */ QString());

        if (m_currentSavedSearchLocalUid.isEmpty()) {
            QNWARNING("Wasn't able to find the saved search local uid for "
                      << "chosen saved search name: " << savedSearchName);
            m_currentSavedSearchName.resize(0);
        }
        else {
            m_currentSavedSearchName = savedSearchName;
        }
    }

    persistSelectedSavedSearch();
}

void FilterBySavedSearchWidget::updateSavedSearchesInComboBox()
{
    QNDEBUG("FilterBySavedSearchWidget::updateSavedSearchesInComboBox");

    if (Q_UNLIKELY(m_pSavedSearchModel.isNull())) {
        QNDEBUG("The saved search model is null");
        m_availableSavedSearchesModel.setStringList(QStringList());
        m_currentSavedSearchLocalUid.resize(0);
        m_currentSavedSearchName.resize(0);
        return;
    }

    m_currentSavedSearchName = m_pSavedSearchModel->itemNameForLocalUid(
        m_currentSavedSearchLocalUid);

    if (Q_UNLIKELY(m_currentSavedSearchName.isEmpty())) {
        QNDEBUG("Wasn't able to find the saved search name for restored saved "
                "search local uid");
        m_currentSavedSearchLocalUid.resize(0);
        m_currentSavedSearchName.resize(0);
    }

    QStringList savedSearchNames = m_pSavedSearchModel->savedSearchNames();
    savedSearchNames.prepend(QString());

    int index = 0;
    auto it = std::lower_bound(
        savedSearchNames.constBegin(), savedSearchNames.constEnd(),
        m_currentSavedSearchName);
    if ((it != savedSearchNames.constEnd()) && (*it == m_currentSavedSearchName)) {
        index = static_cast<int>(std::distance(savedSearchNames.constBegin(), it));
    }

    m_settingCurrentIndex = true;
    m_availableSavedSearchesModel.setStringList(savedSearchNames);
    setCurrentIndex(index);
    m_settingCurrentIndex = false;
}

void FilterBySavedSearchWidget::connectoToSavedSearchModel()
{
    QNDEBUG("FilterBySavedSearchWidget::connectoToSavedSearchModel");

    QObject::connect(m_pSavedSearchModel.data(),
                     QNSIGNAL(SavedSearchModel,rowsInserted,
                              const QModelIndex&,int,int),
                     this,
                     QNSLOT(FilterBySavedSearchWidget,onModelRowsInserted,
                            const QModelIndex&,int,int));
    QObject::connect(m_pSavedSearchModel.data(),
                     QNSIGNAL(SavedSearchModel,rowsRemoved,
                              const QModelIndex&,int,int),
                     this,
                     QNSLOT(FilterBySavedSearchWidget,onModelRowsRemoved,
                            const QModelIndex&,int,int));

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QObject::connect(m_pSavedSearchModel.data(),
                     SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                     this,
                     SLOT(onModelDataChanged(QModelIndex,QModelIndex)));
#else
    QObject::connect(m_pSavedSearchModel.data(),
                     &SavedSearchModel::dataChanged,
                     this,
                     &FilterBySavedSearchWidget::onModelDataChanged);
#endif
}

} // namespace quentier
