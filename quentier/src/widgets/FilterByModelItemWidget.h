#ifndef QUENTIER_WIDGETS_FILTER_BY_MODEL_ITEM_WIDGET_H
#define QUENTIER_WIDGETS_FILTER_BY_MODEL_ITEM_WIDGET_H

#include <quentier/types/Account.h>
#include <QWidget>
#include <QPointer>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/bimap.hpp>
#endif

QT_FORWARD_DECLARE_CLASS(QModelIndex)
QT_FORWARD_DECLARE_CLASS(FlowLayout)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ItemModel)

class FilterByModelItemWidget: public QWidget
{
    Q_OBJECT
public:
    explicit FilterByModelItemWidget(QWidget * parent = Q_NULLPTR);

    const Account & account() const { return m_account; }
    void setAccount(const Account & account);

    /**
     * NOTE: it is intended to set the account before setting the model
     */
    void setModel(ItemModel * pItemModel);

    /**
     * @return the list of names of items present within the filter
     */
    QStringList itemsInFilter() const;

Q_SIGNALS:
    void addedItemToFilter(const QString & itemName);
    void itemRemovedFromFilter(const QString & itemName);

public Q_SLOTS:
    void addItemToFilter(const QString & itemName);

private Q_SLOTS:
    // Slots to track the model changes
    void onModelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight
#if QT_VERSION < 0x050000
                            );
#else
                            , const QVector<int> & roles = QVector<int>());
#endif

    void onModelRowsRemoved(const QModelIndex & parent, int first, int last);
    void onModelReset();

private:
    FlowLayout *                m_pLayout;
    Account                     m_account;
    QPointer<ItemModel>         m_pModel;

    typedef  boost::bimap<QString, QString>  ItemLocalUidToNameBimap;
    ItemLocalUidToNameBimap     m_filteredItemsLocalUidToNameBimap;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_FILTER_BY_MODEL_ITEM_WIDGET_H
