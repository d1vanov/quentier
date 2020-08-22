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

#include "AbstractFilterByModelItemWidget.h"

#include "FlowLayout.h"
#include "ListItemWidget.h"
#include "NewListItemLineEdit.h"

#include <lib/model/ItemModel.h>
#include <lib/preferences/SettingsNames.h>

#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>

#include <QModelIndex>

#define LAST_FILTERED_ITEMS_KEY QStringLiteral("LastFilteredItems")

#define AFTRACE(message) QNTRACE("widget", "[" << m_name << "] " << message)

#define AFDEBUG(message) QNDEBUG("widget", "[" << m_name << "] " << message)

#define AFINFO(message) QNINFO("widget", "[" << m_name << "] " << message)

#define AFWARNING(message) QNWARNING("widget", "[" << m_name << "] " << message)

#define AFERROR(message) QNERROR("widget", "[" << m_name << "] " << message)

namespace quentier {

AbstractFilterByModelItemWidget::AbstractFilterByModelItemWidget(
    const QString & name, QWidget * parent) :
    QWidget(parent),
    m_name(name), m_pLayout(new FlowLayout(this))
{}

void AbstractFilterByModelItemWidget::switchAccount(
    const Account & account, ItemModel * pItemModel)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::switchAccount: " << account.name());

    if (!m_pItemModel.isNull() && (m_pItemModel.data() != pItemModel)) {
        QObject::disconnect(
            m_pItemModel.data(), &ItemModel::notifyAllItemsListed, this,
            &AbstractFilterByModelItemWidget::onModelReady);
    }

    m_pItemModel = pItemModel;
    m_isReady = m_pItemModel->allItemsListed();

    if (!m_pItemModel.isNull() && !m_isReady) {
        QObject::connect(
            m_pItemModel.data(), &ItemModel::notifyAllItemsListed, this,
            &AbstractFilterByModelItemWidget::onModelReady);
    }

    if (m_account == account) {
        AFDEBUG("Already set this account");
        return;
    }

    persistFilteredItems();

    m_account = account;

    if (Q_UNLIKELY(m_pItemModel.isNull())) {
        AFTRACE("The new model is null");
        clearLayout();
        return;
    }

    if (m_pItemModel->allItemsListed()) {
        restoreFilteredItems();
        m_isReady = true;
        Q_EMIT ready();
        return;
    }
}

const ItemModel * AbstractFilterByModelItemWidget::model() const
{
    if (m_pItemModel.isNull()) {
        return nullptr;
    }

    return m_pItemModel.data();
}

QStringList AbstractFilterByModelItemWidget::itemsInFilter() const
{
    QStringList result;

    int numItems = m_pLayout->count();
    result.reserve(numItems);
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pItemWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        QString itemName = pItemWidget->name().trimmed();
        if (itemName.isEmpty()) {
            continue;
        }

        result << itemName;
    }

    return result;
}

QStringList AbstractFilterByModelItemWidget::localUidsOfItemsInFilter() const
{
    AFTRACE("AbstractFilterByModelItemWidget::localUidsOfItemsInFilter");

    QStringList result;

    if (isReady()) {
        AFTRACE("Ready, collecting result from list item widgets")

        int numItems = m_pLayout->count();
        result.reserve(numItems);
        for (int i = 0; i < numItems; ++i) {
            auto * pItem = m_pLayout->itemAt(i);
            if (Q_UNLIKELY(!pItem)) {
                continue;
            }

            auto * pItemWidget =
                qobject_cast<ListItemWidget *>(pItem->widget());
            if (!pItemWidget) {
                continue;
            }

            QString itemLocalUid = pItemWidget->localUid();
            if (itemLocalUid.isEmpty()) {
                continue;
            }

            result << itemLocalUid;
        }
    }
    else {
        AFTRACE("Not ready, reading the result from persistent settings");

        if (m_account.isEmpty()) {
            AFTRACE("Account is empty");
            return result;
        }

        ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
        appSettings.beginGroup(m_name + QStringLiteral("Filter"));
        result = appSettings.value(LAST_FILTERED_ITEMS_KEY).toStringList();
        appSettings.endGroup();
    }

    return result;
}

bool AbstractFilterByModelItemWidget::isReady() const
{
    return m_isReady;
}

void AbstractFilterByModelItemWidget::addItemToFilter(
    const QString & localUid, const QString & itemName,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::addItemToFilter: local uid = "
        << localUid << ", name = " << itemName
        << ", linked notebook guid = " << linkedNotebookGuid
        << ", linked notebook username = " << linkedNotebookUsername);

    int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pItemWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        QString itemLocalUid = pItemWidget->localUid();
        if (itemLocalUid != localUid) {
            continue;
        }

        QString name = pItemWidget->name();
        if (name == itemName) {
            AFDEBUG(
                "Item is already present in the filter and has the same "
                << "name, nothing to do");
            return;
        }

        pItemWidget->setName(itemName);
        pItemWidget->setLinkedNotebookGuid(linkedNotebookGuid);
        pItemWidget->setLinkedNotebookUsername(linkedNotebookUsername);
        return;
    }

    auto * pItemWidget = new ListItemWidget(
        itemName, localUid, linkedNotebookGuid, linkedNotebookUsername, this);

    QObject::connect(
        pItemWidget, &ListItemWidget::itemRemovedFromList, this,
        &AbstractFilterByModelItemWidget::onItemRemovedFromList);

    auto * pNewItemLineEdit = findNewItemWidget();
    if (pNewItemLineEdit) {
        m_pLayout->removeWidget(pNewItemLineEdit);
        pNewItemLineEdit->hide();
        pNewItemLineEdit->deleteLater();
        pNewItemLineEdit = nullptr;
    }

    m_pLayout->addWidget(pItemWidget);
    addNewItemWidget();

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::clear()
{
    AFDEBUG("AbstractFilterByModelItemWidget::clear");

    bool wasEmpty = (m_pLayout->count() <= 0);
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

    if (m_pItemModel.isNull()) {
        AFTRACE("The item model is null");
        return;
    }

    m_isReady = false;

    if (m_pItemModel->allItemsListed()) {
        restoreFilteredItems();
        m_isReady = true;
        Q_EMIT ready();
        return;
    }

    QObject::connect(
        m_pItemModel.data(), &ItemModel::notifyAllItemsListed, this,
        &AbstractFilterByModelItemWidget::onModelReady);
}

void AbstractFilterByModelItemWidget::onItemUpdatedInLocalStorage(
    const QString & localUid, const QString & name)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::onItemUpdatedInLocalStorage: "
        << "local uid = " << localUid << ", name = " << name);

    int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pItemWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        QString itemLocalUid = pItemWidget->localUid();
        if (itemLocalUid != localUid) {
            continue;
        }

        QString itemName = pItemWidget->name();
        if (itemName == name) {
            AFDEBUG("Filtered item's name hasn't changed");
            return;
        }

        pItemWidget->setName(name);
        AFDEBUG("Changed filtered item's name to " << name);
        return;
    }

    AFDEBUG("Item is not within filter");
}

void AbstractFilterByModelItemWidget::onItemRemovedFromLocalStorage(
    const QString & localUid)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::onItemRemovedFromLocalStorage: "
        << "local uid = " << localUid);

    int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pItemWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        if (pItemWidget->localUid() != localUid) {
            continue;
        }

        m_pLayout->removeWidget(pItemWidget);
        pItemWidget->hide();
        pItemWidget->deleteLater();
        break;
    }

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::onNewItemAdded()
{
    AFDEBUG("AbstractFilterByModelItemWidget::onNewItemAdded");

    auto * pNewItemLineEdit = qobject_cast<NewListItemLineEdit *>(sender());
    if (Q_UNLIKELY(!pNewItemLineEdit)) {
        ErrorString error(
            QT_TR_NOOP("Internal error: can't process the addition of a new "
                       "item to the filter: can't cast the signal sender to "
                       "NewListLineEdit"));

        AFWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    QString newItemName = pNewItemLineEdit->text();
    AFTRACE("New item name: " << newItemName);

    if (newItemName.isEmpty()) {
        return;
    }

    pNewItemLineEdit->clear();

    if (Q_UNLIKELY(m_account.isEmpty())) {
        AFDEBUG("Current account is empty, won't do anything");
        return;
    }

    if (Q_UNLIKELY(m_pItemModel.isNull())) {
        AFDEBUG("Current item model is null, won't do anything");
        return;
    }

    QString newItemLinkedNotebookUsername;
    if (newItemName.contains(QStringLiteral("\\"))) {
        auto nameParts = newItemName.split(QStringLiteral("\\"));
        if (nameParts.size() == 2) {
            newItemName = nameParts[0].trimmed();
            newItemLinkedNotebookUsername = nameParts[1].trimmed().remove(0, 1);
        }
    }

    auto linkedNotebooksInfo = m_pItemModel->linkedNotebooksInfo();
    QString newItemLinkedNotebookGuid;
    for (const auto & linkedNotebookInfo: qAsConst(linkedNotebooksInfo)) {
        if (linkedNotebookInfo.m_username == newItemLinkedNotebookUsername) {
            newItemLinkedNotebookGuid = linkedNotebookInfo.m_guid;
            break;
        }
    }

    QString localUid = m_pItemModel->localUidForItemName(
        newItemName, newItemLinkedNotebookGuid);

    if (localUid.isEmpty()) {
        ErrorString error(
            QT_TR_NOOP("Can't process the addition of a new item "
                       "to the filter: can't find the item's local uid"));

        AFWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    NewListItemLineEdit::ItemInfo newReservedItem;
    newReservedItem.m_name = newItemName;
    newReservedItem.m_linkedNotebookGuid = newItemLinkedNotebookGuid;
    newReservedItem.m_linkedNotebookUsername = newItemLinkedNotebookUsername;
    pNewItemLineEdit->addReservedItem(std::move(newReservedItem));

    m_pLayout->removeWidget(pNewItemLineEdit);

    auto * pItemWidget = new ListItemWidget(
        newItemName, localUid, newItemLinkedNotebookGuid,
        newItemLinkedNotebookUsername, this);

    QObject::connect(
        pItemWidget, &ListItemWidget::itemRemovedFromList, this,
        &AbstractFilterByModelItemWidget::onItemRemovedFromList);

    m_pLayout->addWidget(pItemWidget);

    m_pLayout->addWidget(pNewItemLineEdit);
    if (!pNewItemLineEdit->hasFocus()) {
        pNewItemLineEdit->setFocus();
    }

    AFTRACE(
        "Successfully added the new item to filter: local uid = "
        << localUid << ", name = " << newItemName
        << ", linked notebook guid = " << newItemLinkedNotebookGuid
        << ", linked notebook username = " << newItemLinkedNotebookUsername);

    Q_EMIT addedItemToFilter(
        localUid, newItemName, newItemLinkedNotebookGuid,
        newItemLinkedNotebookUsername);

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::onItemRemovedFromList(
    QString localUid, QString name, QString linkedNotebookGuid,
    QString linkedNotebookUsername)
{
    AFDEBUG(
        "AbstractFilterByModelItemWidget::onItemRemovedFromList: local "
        << "uid = " << localUid << " name = " << name << ", linked notebook "
        << "guid = " << linkedNotebookGuid
        << ", linked notebook username = " << linkedNotebookUsername);

    int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pItemWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        if (pItemWidget->localUid() != localUid) {
            continue;
        }

        m_pLayout->removeWidget(pItemWidget);
        pItemWidget->hide();
        pItemWidget->deleteLater();
        break;
    }

    Q_EMIT itemRemovedFromFilter(
        localUid, name, linkedNotebookGuid, linkedNotebookUsername);

    persistFilteredItems();

    auto * pNewItemLineEdit = findNewItemWidget();
    if (pNewItemLineEdit) {
        NewListItemLineEdit::ItemInfo removedItemInfo;
        removedItemInfo.m_name = name;
        removedItemInfo.m_linkedNotebookGuid = linkedNotebookGuid;
        removedItemInfo.m_linkedNotebookUsername = linkedNotebookUsername;

        pNewItemLineEdit->removeReservedItem(removedItemInfo);
    }
}

void AbstractFilterByModelItemWidget::onModelReady()
{
    AFDEBUG("AbstractFilterByModelItemWidget::onModelReady");

    QObject::disconnect(
        m_pItemModel.data(), &ItemModel::notifyAllItemsListed, this,
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

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(m_name + QStringLiteral("Filter"));

    QStringList filteredItemsLocalUids;

    int numItems = m_pLayout->count();
    filteredItemsLocalUids.reserve(numItems);

    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pItemWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        filteredItemsLocalUids << pItemWidget->localUid();
    }

    appSettings.setValue(LAST_FILTERED_ITEMS_KEY, filteredItemsLocalUids);
    appSettings.endGroup();

    AFDEBUG(
        "Successfully persisted the local uids of filtered items: "
        << filteredItemsLocalUids.join(QStringLiteral(", ")));
}

void AbstractFilterByModelItemWidget::restoreFilteredItems()
{
    AFDEBUG("AbstractFilterByModelItemWidget::restoreFilteredItems");

    if (m_account.isEmpty()) {
        AFDEBUG("The account is empty, nothing to restore");
        return;
    }

    if (Q_UNLIKELY(m_pItemModel.isNull())) {
        AFDEBUG("The item model is null, can't restore anything");
        return;
    }

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(m_name + QStringLiteral("Filter"));

    QStringList itemLocalUids =
        appSettings.value(LAST_FILTERED_ITEMS_KEY).toStringList();

    appSettings.endGroup();

    if (itemLocalUids.isEmpty()) {
        AFDEBUG(
            "The previously persisted list of item local uids within "
            << "the filter is empty");
        clear();
        return;
    }

    clearLayout();

    for (const auto & itemLocalUid: qAsConst(itemLocalUids)) {
        auto itemInfo = m_pItemModel->itemInfoForLocalUid(itemLocalUid);
        if (itemInfo.m_localUid.isEmpty()) {
            AFTRACE("Found no item name for local uid " << itemLocalUid);
            continue;
        }

        auto * pItemWidget = new ListItemWidget(
            itemInfo.m_name, itemLocalUid, itemInfo.m_linkedNotebookGuid,
            itemInfo.m_linkedNotebookUsername, this);

        QObject::connect(
            pItemWidget, &ListItemWidget::itemRemovedFromList, this,
            &AbstractFilterByModelItemWidget::onItemRemovedFromList);

        m_pLayout->addWidget(pItemWidget);
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

    if (m_pItemModel.isNull()) {
        AFDEBUG("The model is null");
        return;
    }

    QVector<NewListItemLineEdit::ItemInfo> reservedItems;

    int numItems = m_pLayout->count();
    reservedItems.reserve(numItems);

    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pItemWidget = qobject_cast<ListItemWidget *>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        NewListItemLineEdit::ItemInfo reservedItem;
        reservedItem.m_name = pItemWidget->name();
        reservedItem.m_linkedNotebookGuid = pItemWidget->linkedNotebookGuid();

        reservedItem.m_linkedNotebookUsername =
            pItemWidget->linkedNotebookUsername();

        reservedItems.push_back(std::move(reservedItem));
    }

    auto * pNewItemLineEdit = new NewListItemLineEdit(
        m_pItemModel.data(), std::move(reservedItems), this);

    QObject::connect(
        pNewItemLineEdit, &NewListItemLineEdit::returnPressed, this,
        &AbstractFilterByModelItemWidget::onNewItemAdded);

    m_pLayout->addWidget(pNewItemLineEdit);
}

void AbstractFilterByModelItemWidget::clearLayout()
{
    AFDEBUG("AbstractFilterByModelItemWidget::clearLayout");

    while (m_pLayout->count() > 0) {
        auto * pItem = m_pLayout->itemAt(0);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pWidget = pItem->widget();
        m_pLayout->removeWidget(pWidget);
        pWidget->hide();
        pWidget->deleteLater();
    }
}

NewListItemLineEdit * AbstractFilterByModelItemWidget::findNewItemWidget()
{
    const int numItems = m_pLayout->count();
    for (int i = 0; i < numItems; ++i) {
        auto * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        auto * pNewItemWidget =
            qobject_cast<NewListItemLineEdit *>(pItem->widget());

        if (!pNewItemWidget) {
            continue;
        }

        return pNewItemWidget;
    }

    return nullptr;
}

} // namespace quentier
