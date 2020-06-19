/*
 * Copyright 2016-2020 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIDGET_NEW_LIST_ITEM_LINE_EDIT_H
#define QUENTIER_LIB_WIDGET_NEW_LIST_ITEM_LINE_EDIT_H

#include <quentier/utility/Macros.h>

#include <QHash>
#include <QLineEdit>
#include <QPointer>
#include <QVector>

#include <utility>

namespace Ui {
class NewListItemLineEdit;
}

QT_FORWARD_DECLARE_CLASS(QCompleter)
QT_FORWARD_DECLARE_CLASS(QModelIndex)
QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ItemModel)

class NewListItemLineEdit: public QLineEdit
{
    Q_OBJECT
public:
    struct ItemInfo
    {
        QString     m_name;
        QString     m_linkedNotebookGuid;
        QString     m_linkedNotebookUsername;
    };

public:
    explicit NewListItemLineEdit(
        ItemModel * pItemModel,
        QVector<ItemInfo> reservedItems,
        QWidget * parent = nullptr);

    virtual ~NewListItemLineEdit();

    const QString & targetLinkedNotebookGuid() const;
    void setTargetLinkedNotebookGuid(QString linkedNotebookGuid);

    QVector<ItemInfo> reservedItems() const;
    void setReservedItems(QVector<ItemInfo> items);

    void addReservedItem(ItemInfo item);
    void removeReservedItem(ItemInfo item);

    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;

Q_SIGNALS:
    void receivedFocusFromWindowSystem();

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) override;
    virtual void focusInEvent(QFocusEvent * pEvent) override;

private Q_SLOTS:
    void onModelRowsInserted(const QModelIndex & parent, int start, int end);
    void onModelRowsRemoved(const QModelIndex & parent, int start, int end);

    void onModelDataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = QVector<int>());

private:
    void setupCompleter();

    QStringList itemNamesForCompleter() const;

private:
    Ui::NewListItemLineEdit *   m_pUi;
    QPointer<ItemModel>         m_pItemModel;
    QVector<ItemInfo>           m_reservedItems;
    QStringListModel *          m_pItemNamesModel;
    QCompleter *                m_pCompleter;
    QString                     m_targetLinkedNotebookGuid;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_NEW_LIST_ITEM_LINE_EDIT_H
