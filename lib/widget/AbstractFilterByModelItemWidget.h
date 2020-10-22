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

#ifndef QUENTIER_LIB_WIDGET_ABSTRACT_FILTER_BY_MODEL_ITEM_WIDGET_H
#define QUENTIER_LIB_WIDGET_ABSTRACT_FILTER_BY_MODEL_ITEM_WIDGET_H

#include <quentier/types/Account.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/SuppressWarnings.h>

#include <QPointer>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(FlowLayout)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(AbstractItemModel)
QT_FORWARD_DECLARE_CLASS(NewListItemLineEdit)

/**
 * @brief The AbstractFilterByModelItemWidget class is the base class for filter
 * by model items widget; it handles the boilerplate work of managing the
 * widget's layout and requires its subclasses to implement communication
 * with local storage
 */
class AbstractFilterByModelItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AbstractFilterByModelItemWidget(
        const QString & name, QWidget * parent = nullptr);

    const Account & account() const
    {
        return m_account;
    }

    void switchAccount(const Account & account, AbstractItemModel * pItemModel);

    const AbstractItemModel * model() const;

    QStringList itemsInFilter() const;

    QStringList localUidsOfItemsInFilter() const;

    /**
     * @return true if the filter widget has been fully initialized after the
     * most recent account switching, false otherwise
     */
    bool isReady() const;

Q_SIGNALS:
    void notifyError(ErrorString error);

    /**
     * @brief addedItemToFilter signal is emitted when the item is added to
     * the filter
     *
     * @param itemLocalUid              The local uid of the item added to
     *                                  filter
     * @param itemName                  The name of the item added to filter
     * @param linkedNotebookGuid        The linked notebook guid of the item
     *                                  added to filter
     * @param linkedNotebookUsername    The linked notebook username
     *                                  corresponding to the item added to
     *                                  filter
     */
    void addedItemToFilter(
        const QString & itemLocalUid, const QString & itemName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    /**
     * @brief itemRemovedFromFilter signal is emitted when the item is removed
     * from the filter
     *
     * @param itemLocalUid              The local uid of the item removed from
     *                                  filter
     * @param itemName                  The name of the item removed from filter
     * @param linkedNotebookGuid        The linked notebook guid of the item
     *                                  removed from filter
     * @param linkedNotebookUsername    The linked notebook username
     *                                  corresponding to the item removed from
     *                                  filter
     */
    void itemRemovedFromFilter(
        const QString & itemLocalUid, const QString & itemName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    /**
     * @brief cleared signal is emitted when all items are removed from the
     * filter
     */
    void cleared();

    /**
     * @brief updated signal is emitted when the set of items in the filter
     * changes so that it needs to be re-requested and re-processed
     */
    void updated();

    /**
     * @brief ready signal is emitted when the filter widget's initialization
     * is complete
     */
    void ready();

public Q_SLOTS:
    void addItemToFilter(
        const QString & localUid, const QString & itemName,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername);

    void clear();

    /**
     * @brief update - this slot should be called in case it's necessary to
     * re-fetch the information about all items within the filter because some
     * of them might have been deleted but and it's hard to tell which ones
     * exactly
     */
    void update();

    // The subclass should call these methods in relevant circumstances
    void onItemUpdatedInLocalStorage(
        const QString & localUid, const QString & name);

    void onItemRemovedFromLocalStorage(const QString & localUid);

private Q_SLOTS:
    void onNewItemAdded();

    void onItemRemovedFromList(
        QString localUid, QString name, QString linkedNotebookGuid,
        QString linkedNotebookUsername);

    void onModelReady();

private:
    void persistFilteredItems();
    void restoreFilteredItems();

    void addNewItemWidget();
    void clearLayout();

    NewListItemLineEdit * findNewItemWidget();

private:
    QString m_name;
    FlowLayout * m_pLayout = nullptr;
    Account m_account;
    QPointer<AbstractItemModel> m_pItemModel;
    bool m_isReady = false;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_ABSTRACT_FILTER_BY_MODEL_ITEM_WIDGET_H
