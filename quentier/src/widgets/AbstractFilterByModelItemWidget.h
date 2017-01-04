#ifndef QUENTIER_WIDGETS_ABSTRACT_FILTER_BY_MODEL_ITEM_WIDGET_H
#define QUENTIER_WIDGETS_ABSTRACT_FILTER_BY_MODEL_ITEM_WIDGET_H

#include <quentier/types/Account.h>
#include <QWidget>

// NOTE: Workaround a bug in Qt4 which may prevent building with some boost versions
#ifndef Q_MOC_RUN
#include <boost/bimap.hpp>
#endif

QT_FORWARD_DECLARE_CLASS(FlowLayout)

namespace quentier {

/**
 * @brief The AbstractFilterByModelItemWidget class is the base class for filter by model items widget;
 * it handles the boilerplate work of managing the widget's layout and requires its subclasses to implement
 * the communication with the local storage
 */
class AbstractFilterByModelItemWidget: public QWidget
{
    Q_OBJECT
public:
    explicit AbstractFilterByModelItemWidget(const QString & name, QWidget * parent = Q_NULLPTR);

    const Account & account() const { return m_account; }
    void setAccount(const Account & account);

    QStringList itemsInFilter() const;

Q_SIGNALS:
    /**
     * @brief addedItemToFilter signal is emitted when the item is added to the filter
     * @param itemName - the name of the item added to the filted
     */
    void addedItemToFilter(const QString & itemName);

    /**
     * @brief itemRemovedFromFilter signal is emitted when the item is removed from the filter
     * @param itemName - the name of the item removed from the filter
     */
    void itemRemovedFromFilter(const QString & itemName);

    /**
     * @brief cleared signal is emitted when all items are removed from the filter
     */
    void cleared();

    /**
     * @brief updated signal is emitted when the set of items in the filter changes
     * so that it needs to be re-requested and re-processed
     */
    void updated();

public Q_SLOTS:
    void addItemToFilter(const QString & localUid, const QString & itemName);
    void clear();

    // The subclass should call these methods in relevant circumstances
    void onItemUpdatedInLocalStorage(const QString & localUid, const QString & name);
    void onItemRemovedFromLocalStorage(const QString & localUid);
    void onItemFoundInLocalStorage(const QString & localUid, const QString & name);
    void onItemNotFoundInLocalStorage(const QString & localUid);

private Q_SLOTS:
    void onItemRemovedFromList(QString name);

protected:
    // Interface for subclasses
    virtual void findItemInLocalStorage(const QString & localUid) = 0;

private:
    void persistFilteredItems();
    void restoreFilteredItems();

private:
    QString                     m_name;
    FlowLayout *                m_pLayout;
    Account                     m_account;

    typedef  boost::bimap<QString, QString>  ItemLocalUidToNameBimap;
    ItemLocalUidToNameBimap     m_filteredItemsLocalUidToNameBimap;

    QStringList                 m_localUidsPendingFindInLocalStorage;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_ABSTRACT_FILTER_BY_MODEL_ITEM_WIDGET_H
