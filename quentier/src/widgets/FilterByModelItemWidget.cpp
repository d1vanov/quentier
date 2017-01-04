#include "FilterByModelItemWidget.h"
#include "FlowLayout.h"
#include "ListItemWidget.h"
#include "../models/ItemModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QModelIndex>

namespace quentier {

FilterByModelItemWidget::FilterByModelItemWidget(QWidget * parent) :
    QWidget(parent),
    m_pLayout(new FlowLayout(this)),
    m_account(),
    m_filteredItemsLocalUidToNameBimap()
{}

void FilterByModelItemWidget::setAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::setAccount: ") << account);

    if (m_account == account) {
        QNDEBUG(QStringLiteral("Already set this account"));
        return;
    }

    m_account = account;
}

QStringList FilterByModelItemWidget::itemsInFilter() const
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

void FilterByModelItemWidget::addItemToFilter(const QString & localUid, const QString & itemName)
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::addItemToFilter: local uid = ")
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
    m_pLayout->addWidget(pNewListItemWidget);
}

void FilterByModelItemWidget::onItemUpdatedInLocalStorage(const QString & localUid, const QString & name)
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::onItemUpdatedInLocalStorage: local uid = ")
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

void FilterByModelItemWidget::onItemRemovedFromLocalStorage(const QString & localUid)
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::onItemRemovedFromLocalStorage: local uid = ")
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

        if (pItemWidget->name() == itemName) {
            pItem = m_pLayout->takeAt(i);
            delete pItem->widget();
            delete pItem;
            break;
        }
    }
}

void FilterByModelItemWidget::clear()
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::clear"));

    // TODO: implement
}

} // namespace quentier
