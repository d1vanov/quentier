#ifndef QUENTIER_WIDGETS_FILTER_BY_MODEL_ITEM_WIDGET_H
#define QUENTIER_WIDGETS_FILTER_BY_MODEL_ITEM_WIDGET_H

#include <quentier/types/Account.h>
#include <QWidget>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/bimap.hpp>
#endif

QT_FORWARD_DECLARE_CLASS(FlowLayout)

namespace quentier {

class FilterByModelItemWidget: public QWidget
{
    Q_OBJECT
public:
    explicit FilterByModelItemWidget(QWidget * parent = Q_NULLPTR);

    const Account & account() const { return m_account; }
    void setAccount(const Account & account);

    QStringList itemsInFilter() const;

Q_SIGNALS:
    void addedItemToFilter(const QString & itemName);
    void itemRemovedFromFilter(const QString & itemName);

public Q_SLOTS:
    void addItemToFilter(const QString & localUid, const QString & itemName);
    void clear();

    void onItemUpdatedInLocalStorage(const QString & localUid, const QString & name);
    void onItemRemovedFromLocalStorage(const QString & localUid);

private:
    FlowLayout *                m_pLayout;
    Account                     m_account;

    typedef  boost::bimap<QString, QString>  ItemLocalUidToNameBimap;
    ItemLocalUidToNameBimap     m_filteredItemsLocalUidToNameBimap;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_FILTER_BY_MODEL_ITEM_WIDGET_H
