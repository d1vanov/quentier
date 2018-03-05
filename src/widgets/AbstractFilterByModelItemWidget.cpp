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

#include "AbstractFilterByModelItemWidget.h"
#include "FlowLayout.h"
#include "ListItemWidget.h"
#include "NewListItemLineEdit.h"
#include "../SettingsNames.h"
#include "../models/ItemModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QModelIndex>

#define LAST_FILTERED_ITEMS_LOCAL_UIDS_KEY QStringLiteral("LastFilteredItems")

namespace quentier {

AbstractFilterByModelItemWidget::AbstractFilterByModelItemWidget(const QString & name, QWidget * parent) :
    QWidget(parent),
    m_name(name),
    m_pLayout(new FlowLayout(this)),
    m_account(),
    m_pItemModel(),
    m_filteredItemsLocalUidToNameBimap()
{}

void AbstractFilterByModelItemWidget::switchAccount(const Account & account, ItemModel * pItemModel)
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::switchAccount: ") << account);

    if (!m_pItemModel.isNull() && (m_pItemModel.data() != pItemModel)) {
        QObject::disconnect(m_pItemModel.data(), QNSIGNAL(ItemModel,notifyAllItemsListed),
                            this, QNSLOT(AbstractFilterByModelItemWidget,onModelReady));
    }

    m_pItemModel = pItemModel;
    if (!m_pItemModel.isNull() && !m_pItemModel->allItemsListed()) {
        QObject::connect(m_pItemModel.data(), QNSIGNAL(ItemModel,notifyAllItemsListed),
                         this, QNSLOT(AbstractFilterByModelItemWidget,onModelReady));
    }

    if (m_account == account) {
        QNDEBUG(QStringLiteral("Already set this account"));
        return;
    }

    persistFilteredItems();

    m_account = account;

    if (Q_UNLIKELY(m_pItemModel.isNull())) {
        QNTRACE(QStringLiteral("The new model is null"));
        m_filteredItemsLocalUidToNameBimap.clear();
        clearLayout();
        return;
    }

    if (m_pItemModel->allItemsListed()) {
        restoreFilteredItems();
        Q_EMIT updated();
        return;
    }
}

const ItemModel * AbstractFilterByModelItemWidget::model() const
{
    if (m_pItemModel.isNull()) {
        return Q_NULLPTR;
    }

    return m_pItemModel.data();
}

QStringList AbstractFilterByModelItemWidget::itemsInFilter() const
{
    QStringList result;

    int numItems = m_pLayout->count();
    result.reserve(numItems);
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        ListItemWidget * pItemWidget = qobject_cast<ListItemWidget*>(pItem->widget());
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

void AbstractFilterByModelItemWidget::addItemToFilter(const QString & localUid, const QString & itemName)
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::addItemToFilter: local uid = ")
            << localUid << QStringLiteral(", name = ") << itemName);

    auto it = m_filteredItemsLocalUidToNameBimap.left.find(localUid);
    if (it != m_filteredItemsLocalUidToNameBimap.left.end()) {
        QNDEBUG(QStringLiteral("Item is already within filter"));
        // Just in case ensure the name would match
        onItemUpdatedInLocalStorage(localUid, itemName);
        return;
    }

    Q_UNUSED(m_filteredItemsLocalUidToNameBimap.insert(ItemLocalUidToNameBimap::value_type(localUid, itemName)))

    ListItemWidget * pItemWidget = new ListItemWidget(itemName, this);
    QObject::connect(pItemWidget, QNSIGNAL(ListItemWidget,itemRemovedFromList,QString),
                     this, QNSLOT(AbstractFilterByModelItemWidget,onItemRemovedFromList,QString));

    NewListItemLineEdit * pNewItemLineEdit = findNewItemWidget();
    if (pNewItemLineEdit) {
        m_pLayout->removeWidget(pNewItemLineEdit);
        pNewItemLineEdit->hide();
        pNewItemLineEdit->deleteLater();
        pNewItemLineEdit = Q_NULLPTR;
    }

    m_pLayout->addWidget(pItemWidget);
    addNewItemWidget();

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::clear()
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::clear"));

    bool wasEmpty = m_filteredItemsLocalUidToNameBimap.empty();
    QNTRACE(QStringLiteral("Was empty: ") << (wasEmpty ? QStringLiteral("true") : QStringLiteral("false")));

    m_filteredItemsLocalUidToNameBimap.clear();

    clearLayout();
    addNewItemWidget();
    persistFilteredItems();

    if (!wasEmpty) {
        Q_EMIT cleared();
    }
}

void AbstractFilterByModelItemWidget::update()
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::update"));

    clear();

    if (Q_UNLIKELY(m_account.isEmpty())) {
        QNDEBUG(QStringLiteral("Current account is empty, won't do anything"));
        return;
    }

    if (m_pItemModel.isNull()) {
        QNTRACE(QStringLiteral("The item model is null"));
        return;
    }

    if (m_pItemModel->allItemsListed()) {
        restoreFilteredItems();
        return;
    }

    QObject::connect(m_pItemModel.data(), QNSIGNAL(ItemModel,notifyAllItemsListed),
                     this, QNSLOT(AbstractFilterByModelItemWidget,onModelReady));
}

void AbstractFilterByModelItemWidget::onItemUpdatedInLocalStorage(const QString & localUid, const QString & name)
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::onItemUpdatedInLocalStorage: local uid = ")
            << localUid << QStringLiteral(", name = ") << name);

    auto it = m_filteredItemsLocalUidToNameBimap.left.find(localUid);
    if (it == m_filteredItemsLocalUidToNameBimap.left.end()) {
        QNDEBUG(QStringLiteral("Item is not within filter"));
        return;
    }

    if (it->second == name) {
        QNDEBUG(QStringLiteral("Filtered item's name hasn't changed"));
        return;
    }

    QString previousName = it->second;
    Q_UNUSED(m_filteredItemsLocalUidToNameBimap.left.erase(it))
    Q_UNUSED(m_filteredItemsLocalUidToNameBimap.insert(ItemLocalUidToNameBimap::value_type(localUid, name)))

    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        ListItemWidget * pItemWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        if (pItemWidget->name() != previousName) {
            continue;
        }

        pItemWidget->setName(name);
        break;
    }
}

void AbstractFilterByModelItemWidget::onItemRemovedFromLocalStorage(const QString & localUid)
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::onItemRemovedFromLocalStorage: local uid = ")
            << localUid);

    auto it = m_filteredItemsLocalUidToNameBimap.left.find(localUid);
    if (it == m_filteredItemsLocalUidToNameBimap.left.end()) {
        QNDEBUG(QStringLiteral("Item is not within filter"));
        return;
    }

    QString itemName = it->second;
    Q_UNUSED(m_filteredItemsLocalUidToNameBimap.left.erase(it))

    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        ListItemWidget * pItemWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        if (pItemWidget->name() != itemName) {
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
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::onNewItemAdded"));

    NewListItemLineEdit * pNewItemLineEdit = qobject_cast<NewListItemLineEdit*>(sender());
    if (Q_UNLIKELY(!pNewItemLineEdit)) {
        ErrorString error(QT_TR_NOOP("Internal error: can't process the addition of a new item to the filter: "
                                     "can't cast the signal sender to NewListLineEdit"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    QString newItemName = pNewItemLineEdit->text();
    QNTRACE(QStringLiteral("New item name: ") << newItemName);

    if (newItemName.isEmpty()) {
        return;
    }

    pNewItemLineEdit->clear();

    if (Q_UNLIKELY(m_account.isEmpty())) {
        QNDEBUG(QStringLiteral("Current account is empty, won't do anything"));
        return;
    }

    if (Q_UNLIKELY(m_pItemModel.isNull())) {
        QNDEBUG(QStringLiteral("Current item model is null, won't do anything"));
        return;
    }

    QString localUid = m_pItemModel->localUidForItemName(newItemName, /* linked notebook guid = */ QString());
    if (localUid.isEmpty()) {
        ErrorString error(QT_TR_NOOP("Can't process the addition of a new item "
                                     "to the filter: can't find the item's local uid"));
        QNWARNING(error);
        Q_EMIT notifyError(error);
        return;
    }

    auto nit = m_filteredItemsLocalUidToNameBimap.right.find(newItemName);
    if (nit != m_filteredItemsLocalUidToNameBimap.right.end()) {
        QNDEBUG(QStringLiteral("Such item already exists within the filter, skipping"));
        return;
    }

    Q_UNUSED(m_filteredItemsLocalUidToNameBimap.insert(ItemLocalUidToNameBimap::value_type(localUid, newItemName)))

    QStringList filteredItemNames = pNewItemLineEdit->reservedItemNames();
    filteredItemNames << newItemName;
    pNewItemLineEdit->updateReservedItemNames(filteredItemNames);

    m_pLayout->removeWidget(pNewItemLineEdit);

    ListItemWidget * pItemWidget = new ListItemWidget(newItemName, this);
    QObject::connect(pItemWidget, QNSIGNAL(ListItemWidget,itemRemovedFromList,QString),
                     this, QNSLOT(AbstractFilterByModelItemWidget,onItemRemovedFromList,QString));
    m_pLayout->addWidget(pItemWidget);

    m_pLayout->addWidget(pNewItemLineEdit);
    if (!pNewItemLineEdit->hasFocus()) {
        pNewItemLineEdit->setFocus();
    }

    QNTRACE(QStringLiteral("Successfully added the new item to filter: ") << newItemName);
    Q_EMIT addedItemToFilter(newItemName);

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::onItemRemovedFromList(QString name)
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::onItemRemovedFromList: name = ") << name);

    auto it = m_filteredItemsLocalUidToNameBimap.right.find(name);
    if (it == m_filteredItemsLocalUidToNameBimap.right.end()) {
        QNWARNING(QStringLiteral("Internal error: can't remove item from filter: no item with such name was found"));
        return;
    }

    Q_UNUSED(m_filteredItemsLocalUidToNameBimap.right.erase(it))

    int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        ListItemWidget * pItemWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (!pItemWidget) {
            continue;
        }

        if (pItemWidget->name() != name) {
            continue;
        }

        m_pLayout->removeWidget(pItemWidget);
        pItemWidget->hide();
        pItemWidget->deleteLater();
        break;
    }

    QNTRACE(QStringLiteral("Removed item from filter: ") << name);
    Q_EMIT itemRemovedFromFilter(name);

    persistFilteredItems();

    NewListItemLineEdit * pNewItemLineEdit = findNewItemWidget();
    if (pNewItemLineEdit)
    {
        QStringList filteredItemNames = pNewItemLineEdit->reservedItemNames();
        if (filteredItemNames.removeOne(name)) {
            pNewItemLineEdit->updateReservedItemNames(filteredItemNames);
        }
    }
}

void AbstractFilterByModelItemWidget::onModelReady()
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::onModelReady"));

    QObject::disconnect(m_pItemModel.data(), QNSIGNAL(ItemModel,notifyAllItemsListed),
                        this, QNSLOT(AbstractFilterByModelItemWidget,onModelReady));
    restoreFilteredItems();
    Q_EMIT updated();
}

void AbstractFilterByModelItemWidget::persistFilteredItems()
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::persistFilteredItems: account = ")
            << m_account);

    if (m_account.isEmpty()) {
        QNDEBUG(QStringLiteral("The account is empty, nothing to persist"));
        return;
    }

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(m_name + QStringLiteral("Filter"));

    QStringList filteredItemsLocalUids;
    filteredItemsLocalUids.reserve(static_cast<int>(m_filteredItemsLocalUidToNameBimap.size()));
    for(auto it = m_filteredItemsLocalUidToNameBimap.left.begin(), end = m_filteredItemsLocalUidToNameBimap.left.end(); it != end; ++it) {
        const QString & localUid = it->first;
        filteredItemsLocalUids << localUid;
    }

    appSettings.setValue(LAST_FILTERED_ITEMS_LOCAL_UIDS_KEY, filteredItemsLocalUids);
    appSettings.endGroup();

    QNDEBUG(QStringLiteral("Successfully persisted the local uids of filtered items: ")
            << filteredItemsLocalUids.join(QStringLiteral(", ")));
}

void AbstractFilterByModelItemWidget::restoreFilteredItems()
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::restoreFilteredItems"));

    if (m_account.isEmpty()) {
        QNDEBUG(QStringLiteral("The account is empty, nothing to restore"));
        return;
    }

    if (Q_UNLIKELY(m_pItemModel.isNull())) {
        QNDEBUG(QStringLiteral("The item model is null, can't restore anything"));
        return;
    }

    ApplicationSettings appSettings(m_account, QUENTIER_UI_SETTINGS);
    appSettings.beginGroup(m_name + QStringLiteral("Filter"));
    QStringList itemLocalUids = appSettings.value(LAST_FILTERED_ITEMS_LOCAL_UIDS_KEY).toStringList();
    appSettings.endGroup();

    if (itemLocalUids.isEmpty()) {
        QNDEBUG(QStringLiteral("The previously persisted list of item local uids within the filter is empty"));
        clear();
        return;
    }

    m_filteredItemsLocalUidToNameBimap.clear();
    clearLayout();

    for(auto it = itemLocalUids.constBegin(), end = itemLocalUids.constEnd(); it != end; ++it)
    {
        QString itemName = m_pItemModel->itemNameForLocalUid(*it);
        if (itemName.isEmpty()) {
            QNTRACE(QStringLiteral("Found no item name for local uid ") << *it);
            continue;
        }

        Q_UNUSED(m_filteredItemsLocalUidToNameBimap.insert(ItemLocalUidToNameBimap::value_type(*it, itemName)))

        ListItemWidget * pItemWidget = new ListItemWidget(itemName, this);
        QObject::connect(pItemWidget, QNSIGNAL(ListItemWidget,itemRemovedFromList,QString),
                         this, QNSLOT(AbstractFilterByModelItemWidget,onItemRemovedFromList,QString));
        m_pLayout->addWidget(pItemWidget);
    }

    addNewItemWidget();
    QNTRACE(QStringLiteral("Updated the list of items within the filter"));
}

void AbstractFilterByModelItemWidget::addNewItemWidget()
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::addNewItemWidget"));

    if (m_account.isEmpty()) {
        QNDEBUG(QStringLiteral("The account is empty"));
        return;
    }

    if (m_pItemModel.isNull()) {
        QNDEBUG(QStringLiteral("The model is null"));
        return;
    }

    QStringList existingNames;
    existingNames.reserve(static_cast<int>(m_filteredItemsLocalUidToNameBimap.size()));
    for(auto it = m_filteredItemsLocalUidToNameBimap.left.begin(),
        end = m_filteredItemsLocalUidToNameBimap.left.end(); it != end; ++it)
    {
        existingNames << it->second;
    }

    NewListItemLineEdit * pNewItemLineEdit = new NewListItemLineEdit(m_pItemModel.data(), existingNames,
                                                                     /* fake linked notebook guid = */ QStringLiteral(""),
                                                                     this);
    QObject::connect(pNewItemLineEdit, QNSIGNAL(NewListItemLineEdit,returnPressed),
                     this, QNSLOT(AbstractFilterByModelItemWidget,onNewItemAdded));
    m_pLayout->addWidget(pNewItemLineEdit);
}

void AbstractFilterByModelItemWidget::clearLayout()
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::clearLayout"));

    while(m_pLayout->count() > 0)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(0);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        QWidget * pWidget = pItem->widget();
        m_pLayout->removeWidget(pWidget);
        pWidget->hide();
        pWidget->deleteLater();
    }
}

NewListItemLineEdit * AbstractFilterByModelItemWidget::findNewItemWidget()
{
    const int numItems = m_pLayout->count();
    for(int i = 0; i < numItems; ++i)
    {
        QLayoutItem * pItem = m_pLayout->itemAt(i);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        NewListItemLineEdit * pNewItemWidget = qobject_cast<NewListItemLineEdit*>(pItem->widget());
        if (!pNewItemWidget) {
            continue;
        }

        return pNewItemWidget;
    }

    return Q_NULLPTR;
}

} // namespace quentier
