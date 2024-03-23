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

#include "AbstractFilterByModelItemWidget.h"

#include "FlowLayout.h"
#include "ListItemWidget.h"
#include "NewListItemLineEdit.h"

#include <lib/model/common/AbstractItemModel.h>
#include <lib/preferences/keys/Files.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QModelIndex>

#include <string_view>
#include <utility>

#define AFTRACE(message)                                                       \
    QNTRACE(                                                                   \
        "widget::AbstractFilterByModelItemWidget",                             \
        "[" << m_name << "] " << message)

#define AFDEBUG(message)                                                       \
    QNDEBUG(                                                                   \
        "widget::AbstractFilterByModelItemWidget",                             \
        "[" << m_name << "] " << message)

#define AFINFO(message)                                                        \
    QNINFO(                                                                    \
        "widget::AbstractFilterByModelItemWidget",                             \
        "[" << m_name << "] " << message)

#define AFWARNING(message)                                                     \
    QNWARNING(                                                                 \
        "widget::AbstractFilterByModelItemWidget",                             \
        "[" << m_name << "] " << message)

#define AFERROR(message)                                                       \
    QNERROR(                                                                   \
        "widget::AbstractFilterByModelItemWidget",                             \
        "[" << m_name << "] " << message)

namespace quentier {

using namespace std::string_view_literals;

namespace {

constexpr auto gLastFilteredItemsKey = "LastFilteredItems"sv;

} // namespace

AbstractFilterByModelItemWidget::AbstractFilterByModelItemWidget(
    QString name, QWidget * parent) :
    QWidget{parent},
    m_name{std::move(name)}, m_layout{new FlowLayout{this}}
{}

void AbstractFilterByModelItemWidget::switchAccount(
    const Account & account, AbstractItemModel * itemModel)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::switchAccount: " << account.name());

    if (!m_itemModel.isNull() && m_itemModel.data() != itemModel) {
        QObject::disconnect(
            m_itemModel.data(), &AbstractItemModel::notifyAllItemsListed, this,
            &AbstractFilterByModelItemWidget::onModelReady);
    }

    m_itemModel = itemModel;
    m_isReady = m_itemModel->allItemsListed();

    if (!m_itemModel.isNull() && !m_isReady) {
        QObject::connect(
            m_itemModel.data(), &AbstractItemModel::notifyAllItemsListed, this,
            &AbstractFilterByModelItemWidget::onModelReady);
    }

    if (m_account == account) {
        AFDEBUG("Already set this account");
        return;
    }

    persistFilteredItems();

    m_account = account;

    if (Q_UNLIKELY(m_itemModel.isNull())) {
        AFTRACE("The new model is null");
        clearLayout();
        return;
    }

    if (m_itemModel->allItemsListed()) {
        restoreFilteredItems();
        m_isReady = true;
        Q_EMIT ready();
        return;
    }
}

const AbstractItemModel * AbstractFilterByModelItemWidget::model() const
{
    if (m_itemModel.isNull()) {
        return nullptr;
    }

    return m_itemModel.data();
}

QStringList AbstractFilterByModelItemWidget::itemsInFilter() const
{
    QStringList result;

    const int numItems = m_layout->count();
    result.reserve(numItems);
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * itemWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (!itemWidget) {
            continue;
        }

        const QString itemName = itemWidget->name().trimmed();
        if (itemName.isEmpty()) {
            continue;
        }

        result << itemName;
    }

    return result;
}

QStringList AbstractFilterByModelItemWidget::localIdsOfItemsInFilter() const
{
    AFTRACE("AbstractFilterByModelItemWidget::localIdsOfItemsInFilter");

    QStringList result;

    if (isReady()) {
        AFTRACE("Ready, collecting result from list item widgets")

        const int numItems = m_layout->count();
        result.reserve(numItems);
        for (int i = 0; i < numItems; ++i) {
            auto * item = m_layout->itemAt(i);
            if (Q_UNLIKELY(!item)) {
                continue;
            }

            auto * itemWidget =
                qobject_cast<ListItemWidget *>(item->widget());
            if (!itemWidget) {
                continue;
            }

            const QString itemLocalId = itemWidget->localId();
            if (itemLocalId.isEmpty()) {
                continue;
            }

            result << itemLocalId;
        }
    }
    else {
        AFTRACE("Not ready, reading the result from persistent settings");

        if (m_account.isEmpty()) {
            AFTRACE("Account is empty");
            return result;
        }

        ApplicationSettings appSettings{
            m_account, preferences::keys::files::userInterface};

        appSettings.beginGroup(m_name + QStringLiteral("Filter"));
        result = appSettings.value(gLastFilteredItemsKey).toStringList();
        appSettings.endGroup();
    }

    return result;
}

bool AbstractFilterByModelItemWidget::isReady() const noexcept
{
    return m_isReady;
}

void AbstractFilterByModelItemWidget::addItemToFilter(
    const QString & localId, const QString & itemName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::addItemToFilter: local id = "
        << localId << ", name = " << itemName
        << ", linked notebook guid = " << linkedNotebookGuid
        << ", linked notebook username = " << linkedNotebookUsername);

    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * itemWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (!itemWidget) {
            continue;
        }

        const QString itemLocalId = itemWidget->localId();
        if (itemLocalId != localId) {
            continue;
        }

        const QString name = itemWidget->name();
        if (name == itemName) {
            AFDEBUG(
                "Item is already present in the filter and has the same "
                << "name, nothing to do");
            return;
        }

        itemWidget->setName(itemName);
        itemWidget->setLinkedNotebookGuid(linkedNotebookGuid);
        itemWidget->setLinkedNotebookUsername(linkedNotebookUsername);
        return;
    }

    auto * itemWidget = new ListItemWidget(
        itemName, localId, linkedNotebookGuid, linkedNotebookUsername, this);

    QObject::connect(
        itemWidget, &ListItemWidget::itemRemovedFromList, this,
        &AbstractFilterByModelItemWidget::onItemRemovedFromList);

    auto * newItemLineEdit = findNewItemWidget();
    if (newItemLineEdit) {
        m_layout->removeWidget(newItemLineEdit);
        newItemLineEdit->hide();
        newItemLineEdit->deleteLater();
        newItemLineEdit = nullptr;
    }

    m_layout->addWidget(itemWidget);
    addNewItemWidget();

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::clear()
{
    AFDEBUG("AbstractFilterByModelItemWidget::clear");

    const bool wasEmpty = (m_layout->count() <= 0);
    AFTRACE("Was empty: " << (wasEmpty ? "true" : "false"));

    clearLayout();
    addNewItemWidget();
    persistFilteredItems();

    if (!wasEmpty) {
        Q_EMIT cleared();
    }
}

void AbstractFilterByModelItemWidget::update()
{
    AFDEBUG("AbstractFilterByModelItemWidget::update");

    clear();

    if (Q_UNLIKELY(m_account.isEmpty())) {
        AFDEBUG("Current account is empty, won't do anything");
        return;
    }

    if (m_itemModel.isNull()) {
        AFTRACE("The item model is null");
        return;
    }

    m_isReady = false;

    if (m_itemModel->allItemsListed()) {
        restoreFilteredItems();
        m_isReady = true;
        Q_EMIT ready();
        return;
    }

    QObject::connect(
        m_itemModel.data(), &AbstractItemModel::notifyAllItemsListed, this,
        &AbstractFilterByModelItemWidget::onModelReady);
}

void AbstractFilterByModelItemWidget::onItemUpdatedInLocalStorage(
    const QString & localId, const QString & name)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::onItemUpdatedInLocalStorage: "
        << "local id = " << localId << ", name = " << name);

    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * itemWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (!itemWidget) {
            continue;
        }

        const QString itemLocalId = itemWidget->localId();
        if (itemLocalId != localId) {
            continue;
        }

        const QString itemName = itemWidget->name();
        if (itemName == name) {
            AFDEBUG("Filtered item's name hasn't changed");
            return;
        }

        itemWidget->setName(name);
        AFDEBUG("Changed filtered item's name to " << name);
        return;
    }

    AFDEBUG("Item is not within filter");
}

void AbstractFilterByModelItemWidget::onItemRemovedFromLocalStorage(
    const QString & localId)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::onItemRemovedFromLocalStorage: "
        << "local id = " << localId);

    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * itemWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (!itemWidget) {
            continue;
        }

        if (itemWidget->localId() != localId) {
            continue;
        }

        m_layout->removeWidget(itemWidget);
        itemWidget->hide();
        itemWidget->deleteLater();
        break;
    }

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::onNewItemAdded()
{
    AFDEBUG("AbstractFilterByModelItemWidget::onNewItemAdded");

    auto * newItemLineEdit = qobject_cast<NewListItemLineEdit *>(sender());
    if (Q_UNLIKELY(!newItemLineEdit)) {
        ErrorString error{
            QT_TR_NOOP("Internal error: can't process the addition of a new "
                       "item to the filter: can't cast the signal sender to "
                       "NewListLineEdit")};

        AFWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    QString newItemName = newItemLineEdit->text();
    AFTRACE("New item name: " << newItemName);

    if (newItemName.isEmpty()) {
        return;
    }

    newItemLineEdit->clear();

    if (Q_UNLIKELY(m_account.isEmpty())) {
        AFDEBUG("Current account is empty, won't do anything");
        return;
    }

    if (Q_UNLIKELY(m_itemModel.isNull())) {
        AFDEBUG("Current item model is null, won't do anything");
        return;
    }

    QString newItemLinkedNotebookUsername;
    if (newItemName.contains(QStringLiteral("\\"))) {
        const auto nameParts = newItemName.split(QStringLiteral("\\"));
        if (nameParts.size() == 2) {
            newItemName = nameParts[0].trimmed();
            newItemLinkedNotebookUsername = nameParts[1].trimmed().remove(0, 1);
        }
    }

    const auto linkedNotebooksInfo = m_itemModel->linkedNotebooksInfo();
    QString newItemLinkedNotebookGuid;
    for (const auto & linkedNotebookInfo: std::as_const(linkedNotebooksInfo)) {
        if (linkedNotebookInfo.m_username == newItemLinkedNotebookUsername) {
            newItemLinkedNotebookGuid = linkedNotebookInfo.m_guid;
            break;
        }
    }

    const QString localId = m_itemModel->localIdForItemName(
        newItemName, newItemLinkedNotebookGuid);

    if (localId.isEmpty()) {
        ErrorString error{
            QT_TR_NOOP("Can't process the addition of a new item "
                       "to the filter: can't find the item's local id")};

        AFWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    NewListItemLineEdit::ItemInfo newReservedItem;
    newReservedItem.m_name = newItemName;
    newReservedItem.m_linkedNotebookGuid = newItemLinkedNotebookGuid;
    newReservedItem.m_linkedNotebookUsername = newItemLinkedNotebookUsername;
    newItemLineEdit->addReservedItem(std::move(newReservedItem));

    m_layout->removeWidget(newItemLineEdit);

    auto * itemWidget = new ListItemWidget(
        newItemName, localId, newItemLinkedNotebookGuid,
        newItemLinkedNotebookUsername, this);

    QObject::connect(
        itemWidget, &ListItemWidget::itemRemovedFromList, this,
        &AbstractFilterByModelItemWidget::onItemRemovedFromList);

    m_layout->addWidget(itemWidget);

    m_layout->addWidget(newItemLineEdit);
    if (!newItemLineEdit->hasFocus()) {
        newItemLineEdit->setFocus();
    }

    AFTRACE(
        "Successfully added the new item to filter: local id = "
        << localId << ", name = " << newItemName
        << ", linked notebook guid = " << newItemLinkedNotebookGuid
        << ", linked notebook username = " << newItemLinkedNotebookUsername);

    Q_EMIT addedItemToFilter(
        localId, newItemName, newItemLinkedNotebookGuid,
        newItemLinkedNotebookUsername);

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::onItemRemovedFromList(
    QString localId, QString name, QString linkedNotebookGuid,
    QString linkedNotebookUsername)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::onItemRemovedFromList: local id = "
        << localId << " name = " << name << ", linked notebook "
        << "guid = " << linkedNotebookGuid
        << ", linked notebook username = " << linkedNotebookUsername);

    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * itemWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (!itemWidget) {
            continue;
        }

        if (itemWidget->localId() != localId) {
            continue;
        }

        m_layout->removeWidget(itemWidget);
        itemWidget->hide();
        itemWidget->deleteLater();
        break;
    }

    Q_EMIT itemRemovedFromFilter(
        localId, name, linkedNotebookGuid, linkedNotebookUsername);

    persistFilteredItems();

    auto * newItemLineEdit = findNewItemWidget();
    if (newItemLineEdit) {
        NewListItemLineEdit::ItemInfo removedItemInfo;
        removedItemInfo.m_name = name;
        removedItemInfo.m_linkedNotebookGuid = linkedNotebookGuid;
        removedItemInfo.m_linkedNotebookUsername = linkedNotebookUsername;

        newItemLineEdit->removeReservedItem(removedItemInfo);
    }
}

void AbstractFilterByModelItemWidget::onModelReady()
{
    AFDEBUG("AbstractFilterByModelItemWidget::onModelReady");

    QObject::disconnect(
        m_itemModel.data(), &AbstractItemModel::notifyAllItemsListed, this,
        &AbstractFilterByModelItemWidget::onModelReady);

    restoreFilteredItems();
    m_isReady = true;
    Q_EMIT ready();
}

void AbstractFilterByModelItemWidget::persistFilteredItems()
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::persistFilteredItems: account = "
        << m_account.name());

    if (m_account.isEmpty()) {
        AFDEBUG("The account is empty, nothing to persist");
        return;
    }

    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(m_name + QStringLiteral("Filter"));

    QStringList filteredItemsLocalIds;

    const int numItems = m_layout->count();
    filteredItemsLocalIds.reserve(numItems);

    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * itemWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (!itemWidget) {
            continue;
        }

        filteredItemsLocalIds << itemWidget->localId();
    }

    appSettings.setValue(gLastFilteredItemsKey, filteredItemsLocalIds);
    appSettings.endGroup();

    AFDEBUG(
        "Successfully persisted the local ids of filtered items: "
        << filteredItemsLocalIds.join(QStringLiteral(", ")));
}

void AbstractFilterByModelItemWidget::restoreFilteredItems()
{
    AFDEBUG("AbstractFilterByModelItemWidget::restoreFilteredItems");

    if (m_account.isEmpty()) {
        AFDEBUG("The account is empty, nothing to restore");
        return;
    }

    if (Q_UNLIKELY(m_itemModel.isNull())) {
        AFDEBUG("The item model is null, can't restore anything");
        return;
    }

    ApplicationSettings appSettings{
        m_account, preferences::keys::files::userInterface};

    appSettings.beginGroup(m_name + QStringLiteral("Filter"));

    const QStringList itemLocalIds =
        appSettings.value(gLastFilteredItemsKey).toStringList();

    appSettings.endGroup();

    if (itemLocalIds.isEmpty()) {
        AFDEBUG(
            "The previously persisted list of item local ids within "
            << "the filter is empty");
        clear();
        return;
    }

    clearLayout();

    for (const auto & itemLocalId: std::as_const(itemLocalIds)) {
        auto itemInfo = m_itemModel->itemInfoForLocalId(itemLocalId);
        if (itemInfo.m_localId.isEmpty()) {
            AFTRACE("Found no item name for local id " << itemLocalId);
            continue;
        }

        auto * itemWidget = new ListItemWidget(
            itemInfo.m_name, itemLocalId, itemInfo.m_linkedNotebookGuid,
            itemInfo.m_linkedNotebookUsername, this);

        QObject::connect(
            itemWidget, &ListItemWidget::itemRemovedFromList, this,
            &AbstractFilterByModelItemWidget::onItemRemovedFromList);

        m_layout->addWidget(itemWidget);
    }

    addNewItemWidget();
    AFTRACE("Updated the list of items within the filter");
}

void AbstractFilterByModelItemWidget::addNewItemWidget()
{
    AFDEBUG("AbstractFilterByModelItemWidget::addNewItemWidget");

    if (m_account.isEmpty()) {
        AFDEBUG("The account is empty");
        return;
    }

    if (m_itemModel.isNull()) {
        AFDEBUG("The model is null");
        return;
    }

    QList<NewListItemLineEdit::ItemInfo> reservedItems;

    const int numItems = m_layout->count();
    reservedItems.reserve(numItems);

    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * itemWidget = qobject_cast<ListItemWidget *>(item->widget());
        if (!itemWidget) {
            continue;
        }

        NewListItemLineEdit::ItemInfo reservedItem;
        reservedItem.m_name = itemWidget->name();
        reservedItem.m_linkedNotebookGuid = itemWidget->linkedNotebookGuid();

        reservedItem.m_linkedNotebookUsername =
            itemWidget->linkedNotebookUsername();

        reservedItems.push_back(std::move(reservedItem));
    }

    auto * newItemLineEdit = new NewListItemLineEdit(
        m_itemModel.data(), std::move(reservedItems), this);

    QObject::connect(
        newItemLineEdit, &NewListItemLineEdit::returnPressed, this,
        &AbstractFilterByModelItemWidget::onNewItemAdded);

    m_layout->addWidget(newItemLineEdit);
}

void AbstractFilterByModelItemWidget::clearLayout()
{
    AFDEBUG("AbstractFilterByModelItemWidget::clearLayout");

    while (m_layout->count() > 0) {
        auto * item = m_layout->itemAt(0);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * widget = item->widget();
        m_layout->removeWidget(widget);
        widget->hide();
        widget->deleteLater();
    }
}

NewListItemLineEdit * AbstractFilterByModelItemWidget::findNewItemWidget()
{
    const int numItems = m_layout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * item = m_layout->itemAt(i);
        if (Q_UNLIKELY(!item)) {
            continue;
        }

        auto * newItemWidget =
            qobject_cast<NewListItemLineEdit *>(item->widget());

        if (!newItemWidget) {
            continue;
        }

        return newItemWidget;
    }

    return nullptr;
}

} // namespace quentier
