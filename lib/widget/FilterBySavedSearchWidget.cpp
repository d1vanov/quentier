/*
 * Copyright 2017-2025 Dmitry Ivanov
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

#include <lib/model/saved_search/SavedSearchModel.h>
#include <lib/preferences/keys/Files.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QModelIndex>

#include <algorithm>
#include <string_view>

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gLastFilteredSavedSearchKey =
    "LastSelectedSavedSearchLocalUid"sv;

} // namespace

FilterBySavedSearchWidget::FilterBySavedSearchWidget(QWidget * parent) :
    QComboBox(parent)
{
    setModel(&m_availableSavedSearchesModel);
    connectToCurrentIndexChangedSignal();
}

FilterBySavedSearchWidget::~FilterBySavedSearchWidget() = default;

void FilterBySavedSearchWidget::switchAccount(
    Account account, SavedSearchModel * savedSearchModel)
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::switchAccount: " << account);

    if (!m_savedSearchModel.isNull() &&
        m_savedSearchModel.data() != savedSearchModel)
    {
        QObject::disconnect(
            m_savedSearchModel.data(), &SavedSearchModel::rowsInserted, this,
            &FilterBySavedSearchWidget::onModelRowsInserted);

        QObject::disconnect(
            m_savedSearchModel.data(), &SavedSearchModel::rowsRemoved, this,
            &FilterBySavedSearchWidget::onModelRowsRemoved);

        QObject::disconnect(
            m_savedSearchModel.data(), &SavedSearchModel::dataChanged, this,
            &FilterBySavedSearchWidget::onModelDataChanged);
    }

    m_savedSearchModel = savedSearchModel;

    if (!m_savedSearchModel.isNull() &&
        !m_savedSearchModel->allSavedSearchesListed())
    {
        QObject::connect(
            m_savedSearchModel.data(),
            &SavedSearchModel::notifyAllSavedSearchesListed, this,
            &FilterBySavedSearchWidget::onAllSavedSearchesListed);
    }
    else {
        connectoToSavedSearchModel();
    }

    if (m_account == account) {
        QNDEBUG(
            "widget::FilterBySavedSearchWidget", "Already set this account");
        return;
    }

    persistSelectedSavedSearch();

    m_account = std::move(account);

    m_currentSavedSearchName.resize(0);
    m_currentSavedSearchLocalId.resize(0);

    if (Q_UNLIKELY(m_savedSearchModel.isNull())) {
        QNTRACE("widget::FilterBySavedSearchWidget", "The new model is null");
        m_availableSavedSearchesModel.setStringList({});
        return;
    }

    m_isReady = false;

    if (m_savedSearchModel->allSavedSearchesListed()) {
        restoreSelectedSavedSearch();
        updateSavedSearchesInComboBox();
        m_isReady = true;
        Q_EMIT ready();
    }
}

SavedSearchModel * FilterBySavedSearchWidget::savedSearchModel()
{
    if (m_savedSearchModel.isNull()) {
        return nullptr;
    }

    return m_savedSearchModel.data();
}

const SavedSearchModel * FilterBySavedSearchWidget::savedSearchModel() const
{
    if (m_savedSearchModel.isNull()) {
        return nullptr;
    }

    return m_savedSearchModel.data();
}

bool FilterBySavedSearchWidget::isReady() const noexcept
{
    return m_isReady;
}

QString FilterBySavedSearchWidget::filteredSavedSearchLocalId() const
{
    if (isReady()) {
        return m_currentSavedSearchLocalId;
    }

    if (m_account.isEmpty()) {
        return {};
    }

    utility::ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("SavedSearchFilter"));

    QString savedSearchLocalId =
        appSettings.value(gLastFilteredSavedSearchKey).toString();

    appSettings.endGroup();

    return savedSearchLocalId;
}

void FilterBySavedSearchWidget::setCurrentSavedSearchLocalId(
    const QString & savedSearchLocalId)
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::setCurrentSavedSearchLocalId: "
            << savedSearchLocalId);

    if (m_currentSavedSearchLocalId == savedSearchLocalId) {
        QNDEBUG(
            "widget::FilterBySavedSearchWidget",
            "Current saved search local id hasn't changed");
        return;
    }

    m_currentSavedSearchLocalId = savedSearchLocalId;
    persistSelectedSavedSearch();

    if (!isReady()) {
        QNDEBUG(
            "widget::FilterBySavedSearchWidget",
            "Filter by saved search is not ready");
        return;
    }

    updateSavedSearchesInComboBox();
}

void FilterBySavedSearchWidget::onAllSavedSearchesListed()
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::onAllSavedSearchesListed");

    QObject::disconnect(
        m_savedSearchModel.data(),
        &SavedSearchModel::notifyAllSavedSearchesListed, this,
        &FilterBySavedSearchWidget::onAllSavedSearchesListed);

    connectoToSavedSearchModel();
    restoreSelectedSavedSearch();
    updateSavedSearchesInComboBox();

    m_isReady = true;
    Q_EMIT ready();
}

void FilterBySavedSearchWidget::onModelRowsInserted(
    const QModelIndex & parent, const int start, const int end)
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::onModelRowsInserted");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    updateSavedSearchesInComboBox();
}

void FilterBySavedSearchWidget::onModelRowsRemoved(
    const QModelIndex & parent, const int start, const int end)
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::onModelRowsRemoved");

    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)

    updateSavedSearchesInComboBox();
}

void FilterBySavedSearchWidget::onModelDataChanged(
    const QModelIndex & topLeft, const QModelIndex & bottomRight,
    const QVector<int> & roles)
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::onModelDataChanged");

    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)
    Q_UNUSED(roles)

    updateSavedSearchesInComboBox();
}

void FilterBySavedSearchWidget::persistSelectedSavedSearch()
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::persistSelectedSavedSearch: account = "
            << m_account);

    if (m_account.isEmpty()) {
        QNDEBUG(
            "widget::FilterBySavedSearchWidget",
            "The account is empty, nothing to persist");
        return;
    }

    utility::ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("SavedSearchFilter"));

    if (m_currentSavedSearchLocalId.isEmpty()) {
        appSettings.remove(gLastFilteredSavedSearchKey);
    }
    else {
        appSettings.setValue(
            gLastFilteredSavedSearchKey, m_currentSavedSearchLocalId);
    }

    appSettings.endGroup();

    QNTRACE(
        "widget::FilterBySavedSearchWidget",
        "Successfully persisted the last selected saved search in the filter: "
            << m_currentSavedSearchLocalId);
}

void FilterBySavedSearchWidget::restoreSelectedSavedSearch()
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::restoreSelectedSavedSearch");

    if (m_account.isEmpty()) {
        QNDEBUG(
            "widget::FilterBySavedSearchWidget",
            "The account is empty, nothing to restore");
        return;
    }

    if (Q_UNLIKELY(m_savedSearchModel.isNull())) {
        QNDEBUG(
            "widget::FilterBySavedSearchWidget",
            "The saved search model is null, can't restore anything");
        return;
    }

    utility::ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(QStringLiteral("SavedSearchFilter"));

    m_currentSavedSearchLocalId =
        appSettings.value(gLastFilteredSavedSearchKey).toString();

    appSettings.endGroup();

    if (m_currentSavedSearchLocalId.isEmpty()) {
        QNDEBUG(
            "widget::FilterBySavedSearchWidget",
            "No last selected saved search local id, nothing to restore");
        return;
    }
}

void FilterBySavedSearchWidget::onCurrentSavedSearchChanged(int index)
{
    QString savedSearchName = itemText(index);

    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::onCurrentSavedSearchChanged: "
            << savedSearchName << ", index: " << index);

    Q_EMIT currentSavedSearchNameChanged(savedSearchName);

    if (Q_UNLIKELY(m_savedSearchModel.isNull())) {
        QNDEBUG(
            "widget::FilterBySavedSearchWidget",
            "The saved search model is null, "
                << "won't do anything");
        return;
    }

    if (savedSearchName.isEmpty()) {
        m_currentSavedSearchName.resize(0);
        m_currentSavedSearchLocalId.resize(0);
    }
    else {
        m_currentSavedSearchLocalId = m_savedSearchModel->localIdForItemName(
            savedSearchName,
            /* linked notebook guid = */ {});

        if (m_currentSavedSearchLocalId.isEmpty()) {
            QNWARNING(
                "widget::FilterBySavedSearchWidget",
                "Wasn't able to find the saved search local id for chosen "
                    << "saved search name: " << savedSearchName);
            m_currentSavedSearchName.resize(0);
        }
        else {
            m_currentSavedSearchName = std::move(savedSearchName);
        }
    }

    persistSelectedSavedSearch();
}

void FilterBySavedSearchWidget::updateSavedSearchesInComboBox()
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::updateSavedSearchesInComboBox");

    if (Q_UNLIKELY(m_savedSearchModel.isNull())) {
        QNDEBUG(
            "widget::FilterBySavedSearchWidget",
            "The saved search model is null");
        m_availableSavedSearchesModel.setStringList(QStringList{});
        m_currentSavedSearchLocalId.resize(0);
        m_currentSavedSearchName.resize(0);
        return;
    }

    if (!m_currentSavedSearchLocalId.isEmpty()) {
        m_currentSavedSearchName =
            m_savedSearchModel->itemNameForLocalId(m_currentSavedSearchLocalId);

        if (Q_UNLIKELY(m_currentSavedSearchName.isEmpty())) {
            QNDEBUG(
                "widget::FilterBySavedSearchWidget",
                "Wasn't able to find the saved search name for restored saved "
                "search local id");
            m_currentSavedSearchLocalId.resize(0);
            m_currentSavedSearchName.resize(0);
        }
    }
    else {
        m_currentSavedSearchName.resize(0);
    }

    auto savedSearchNames = m_savedSearchModel->savedSearchNames();

    int index = -1;
    const auto it = std::lower_bound(
        savedSearchNames.constBegin(), savedSearchNames.constEnd(),
        m_currentSavedSearchName, [](const QString & lhs, const QString & rhs) {
            return lhs.toUpper() < rhs.toUpper();
        });

    if (it != savedSearchNames.constEnd() && *it == m_currentSavedSearchName) {
        index =
            static_cast<int>(std::distance(savedSearchNames.constBegin(), it));
    }

    savedSearchNames.prepend(QString{});
    if (index >= 0) {
        ++index;
    }
    else {
        index = 0;
    }

    disconnectFromCurrentIndexChangedSignal();
    m_availableSavedSearchesModel.setStringList(savedSearchNames);
    connectToCurrentIndexChangedSignal();

    setCurrentIndex(index);
}

void FilterBySavedSearchWidget::connectoToSavedSearchModel()
{
    QNDEBUG(
        "widget::FilterBySavedSearchWidget",
        "FilterBySavedSearchWidget::connectoToSavedSearchModel");

    QObject::connect(
        m_savedSearchModel.data(), &SavedSearchModel::rowsInserted, this,
        &FilterBySavedSearchWidget::onModelRowsInserted);

    QObject::connect(
        m_savedSearchModel.data(), &SavedSearchModel::rowsRemoved, this,
        &FilterBySavedSearchWidget::onModelRowsRemoved);

    QObject::connect(
        m_savedSearchModel.data(), &SavedSearchModel::dataChanged, this,
        &FilterBySavedSearchWidget::onModelDataChanged);
}

void FilterBySavedSearchWidget::connectToCurrentIndexChangedSignal()
{
    QObject::connect(
        this, qOverload<int>(&FilterBySavedSearchWidget::currentIndexChanged),
        this, &FilterBySavedSearchWidget::onCurrentSavedSearchChanged);
}

void FilterBySavedSearchWidget::disconnectFromCurrentIndexChangedSignal()
{
    QObject::disconnect(
        this, qOverload<int>(&FilterBySavedSearchWidget::currentIndexChanged),
        this, &FilterBySavedSearchWidget::onCurrentSavedSearchChanged);
}

} // namespace quentier
