#include "TagsFilterWidget.h"
#include "FlowLayout.h"
#include "ListItemWidget.h"
#include "../models/TagModel.h"
#include <quentier/logging/QuentierLogger.h>
#include <QModelIndex>

namespace quentier {

TagsFilterWidget::TagsFilterWidget(QWidget * parent) :
    QWidget(parent),
    m_pLayout(new FlowLayout(this)),
    m_account(),
    m_pTagModel(Q_NULLPTR)
{}

void TagsFilterWidget::setAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("TagsFilterWidget::setAccount: ") << account);

    if (m_account == account) {
        QNDEBUG(QStringLiteral("Already set this account"));
        return;
    }

    m_account = account;

    // TODO: disconnect from the previous model, if any
    m_pTagModel = Q_NULLPTR;
}

void TagsFilterWidget::setTagModel(TagModel * pTagModel)
{
    QNDEBUG(QStringLiteral("TagsFilterWidget::setTagModel"));

    if (m_pTagModel.data() == pTagModel) {
        QNDEBUG(QStringLiteral("Already set this tag model"));
        return;
    }

    // TODO: disconnect from the previous model, if any

    m_pTagModel = pTagModel;
    if (Q_UNLIKELY(m_pTagModel.isNull())) {
        QNDEBUG(QStringLiteral("Null tag model was set to the tags filter widget"));
        return;
    }

    // TODO: create the connections with the new model
    // TODO: restore the last saved filter for the current account
}

QStringList TagsFilterWidget::tagNames() const
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

        ListItemWidget * pTagWidget = qobject_cast<ListItemWidget*>(pItem->widget());
        if (!pTagWidget) {
            continue;
        }

        result << pTagWidget->name();
    }

    return result;
}

void TagsFilterWidget::onModelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
                                         )
#else
                                         , const QVector<int> & roles)
#endif
{
    QNDEBUG(QStringLiteral("TagsFilterWidget::onModelDataChanged"));

    Q_UNUSED(topLeft)
    Q_UNUSED(bottomRight)

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    Q_UNUSED(roles)
#endif

    // TODO: examine the existing tags within the filter, remove those that no longer exist in the model
}

void TagsFilterWidget::onModelRowsRemoved(const QModelIndex & parent, int first, int last)
{
    QNDEBUG(QStringLiteral("TagsFilterWidget::onModelRowsRemoved"));

    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)

    // TODO: examine the existing tags within the filter, remove those that no longer exist in the model
}

void TagsFilterWidget::onModelReset()
{
    QNDEBUG(QStringLiteral("TagsFilterWidget::onModelReset"));
}

} // namespace quentier
