#include "AbstractFilterByModelItemWidget.h"
#include "FlowLayout.h"
#include "ListItemWidget.h"
#include "../AccountToKey.h"
#include "../models/ItemModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QModelIndex>

#define LAST_FILTERED_ITEMS_LOCAL_UIDS_KEY QStringLiteral("_LastFilteredItems")

namespace quentier {

AbstractFilterByModelItemWidget::AbstractFilterByModelItemWidget(const QString & name, QWidget * parent) :
    QWidget(parent),
    m_name(name),
    m_pLayout(new FlowLayout(this)),
    m_account(),
    m_filteredItemsLocalUidToNameBimap(),
    m_localUidsPendingFindInLocalStorage()
{}

void AbstractFilterByModelItemWidget::setAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::setAccount: ") << account);

    if (m_account == account) {
        QNDEBUG(QStringLiteral("Already set this account"));
        return;
    }

    persistFilteredItems();

    m_account = account;
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

        result << pItemWidget->name();
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

    ListItemWidget * pNewListItemWidget = new ListItemWidget(itemName, this);
    QObject::connect(pNewListItemWidget, QNSIGNAL(ListItemWidget,itemRemovedFromList,QString),
                     this, QNSLOT(AbstractFilterByModelItemWidget,onItemRemovedFromList,QString));
    m_pLayout->addWidget(pNewListItemWidget);

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::clear()
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::clear"));

    m_filteredItemsLocalUidToNameBimap.clear();

    while(m_pLayout->count() > 0)
    {
        QLayoutItem * pItem = m_pLayout->takeAt(0);
        if (Q_UNLIKELY(!pItem)) {
            continue;
        }

        delete pItem->widget();
        delete pItem;
    }

    persistFilteredItems();
    emit cleared();
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

    int index = m_localUidsPendingFindInLocalStorage.indexOf(localUid);
    if (index >= 0) {
        m_localUidsPendingFindInLocalStorage.removeAt(index);
    }

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

        if (pItemWidget->name() == itemName) {
            pItem = m_pLayout->takeAt(i);
            delete pItem->widget();
            delete pItem;
            break;
        }
    }

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::onItemFoundInLocalStorage(const QString & localUid, const QString & name)
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::onItemFoundInLocalStorage: local uid = ")
            << localUid << QStringLiteral(", name = ") << name);

    int index = m_localUidsPendingFindInLocalStorage.indexOf(localUid);
    if (Q_UNLIKELY(index < 0)) {
        return;
    }

    m_localUidsPendingFindInLocalStorage.removeAt(index);

    Q_UNUSED(m_filteredItemsLocalUidToNameBimap.insert(ItemLocalUidToNameBimap::value_type(localUid, name)))

    ListItemWidget * pItemWidget = new ListItemWidget(name, this);
    QObject::connect(pItemWidget, QNSIGNAL(ListItemWidget,itemRemovedFromList,QString),
                     this, QNSLOT(AbstractFilterByModelItemWidget,onItemRemovedFromList,QString));
    m_pLayout->addWidget(pItemWidget);

    // NOTE: don't emit addedItemToFilter here, instead only emit updated if that was the last item to find
    if (m_localUidsPendingFindInLocalStorage.isEmpty()) {
        QNTRACE(QStringLiteral("Updated the list of items within the filter"));
        emit updated();
    }
}

void AbstractFilterByModelItemWidget::onItemNotFoundInLocalStorage(const QString & localUid)
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::onItemNotFoundInLocalStorage: local uid = ")
            << localUid);

    int index = m_localUidsPendingFindInLocalStorage.indexOf(localUid);
    if (Q_UNLIKELY(index < 0)) {
        return;
    }

    m_localUidsPendingFindInLocalStorage.removeAt(index);

    if (m_localUidsPendingFindInLocalStorage.isEmpty()) {
        QNTRACE(QStringLiteral("Updated the list of items within the filter"));
        emit updated();
    }
}

void AbstractFilterByModelItemWidget::onItemRemovedFromList(QString name)
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::onItemRemovedFromList: name = ") << name);

    auto it = m_filteredItemsLocalUidToNameBimap.right.find(name);
    if (it == m_filteredItemsLocalUidToNameBimap.right.end()) {
        QNWARNING(QStringLiteral("Internal error: can't remove item from filter: no item with such name was found"));
        return;
    }

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

        if (pItemWidget->name() == name) {
            pItem = m_pLayout->takeAt(i);
            delete pItem->widget();
            delete pItem;
            break;
        }
    }

    persistFilteredItems();
}

void AbstractFilterByModelItemWidget::persistFilteredItems()
{
    QNDEBUG(QStringLiteral("AbstractFilterByModelItemWidget::persistFilteredItems: account = ")
            << m_account);

    if (m_account.isEmpty()) {
        QNDEBUG(QStringLiteral("The account is empty, nothing to persist"));
        return;
    }

    QString accountKey = accountToKey(m_account);
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        QNWARNING(QStringLiteral("Internal error: can't convert the account to string key for persisting the filter settings: filter ")
                  << m_name);
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/") + m_name + QStringLiteral("Filter"));

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

    QString accountKey = accountToKey(m_account);
    if (Q_UNLIKELY(accountKey.isEmpty())) {
        QNWARNING(QStringLiteral("Internal error: can't convert the account to string key for restoring the persisted filter settings: filter ")
                  << m_name);
        return;
    }

    ApplicationSettings appSettings;
    appSettings.beginGroup(accountKey + QStringLiteral("/") + m_name + QStringLiteral("Filter"));
    QStringList itemLocalUids = appSettings.value(LAST_FILTERED_ITEMS_LOCAL_UIDS_KEY).toStringList();
    appSettings.endGroup();

    if (itemLocalUids.isEmpty()) {
        QNDEBUG(QStringLiteral("The previously persisted list of item local uids within the filter is empty"));
        clear();
        return;
    }

    m_localUidsPendingFindInLocalStorage.clear();
    m_localUidsPendingFindInLocalStorage.reserve(itemLocalUids.size());
    for(auto it = itemLocalUids.constBegin(), end = itemLocalUids.constEnd(); it != end; ++it) {
        m_localUidsPendingFindInLocalStorage << *it;
        findItemInLocalStorage(*it);
    }
}

} // namespace quentier
