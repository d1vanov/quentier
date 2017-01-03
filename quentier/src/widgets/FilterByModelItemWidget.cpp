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
    m_pModel(Q_NULLPTR),
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

    // TODO: disconnect from the previous model, if any
    m_pModel = Q_NULLPTR;
}

void FilterByModelItemWidget::setModel(ItemModel * pModel)
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::setModel"));

    if (m_pModel.data() == pModel) {
        QNDEBUG(QStringLiteral("Already set this model"));
        return;
    }

    // TODO: disconnect from the previous model, if any

    m_pModel = pModel;
    if (Q_UNLIKELY(m_pModel.isNull())) {
        QNDEBUG(QStringLiteral("Null model was set to the filter by model items widget"));
        return;
    }

    // TODO: create the connections with the new model
    // TODO: restore the last saved filter for the current account
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

void FilterByModelItemWidget::addItemToFilter(const QString & itemName)
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::addItemToFilter: ") << itemName);

    // TODO: implement
}

void FilterByModelItemWidget::onModelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
                                                 )
#else
                                                 , const QVector<int> & roles)
#endif
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::onModelDataChanged"));

    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    Q_UNUSED(roles)
#endif

    // TODO: examine the existing items within the filter, rename those which names have changed
    // (use local uid to name bimap to determine that), remove those that no longer exist in the model
}

void FilterByModelItemWidget::onModelRowsRemoved(const QModelIndex & parent, int first, int last)
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::onModelRowsRemoved"));

    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    // TODO: examine the existing tags within the filter, remove those that no longer exist in the model
}

void FilterByModelItemWidget::onModelReset()
{
    QNDEBUG(QStringLiteral("FilterByModelItemWidget::onModelReset"));

    // TODO: clear the items within the filter
}

} // namespace quentier
