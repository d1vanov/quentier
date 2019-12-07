/*
 * Copyright 2016-2019 Dmitry Ivanov
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

#include <QLineEdit>
#include <QPointer>
#include <QVector>

namespace Ui {
class NewListItemLineEdit;
}

QT_FORWARD_DECLARE_CLASS(QCompleter)
QT_FORWARD_DECLARE_CLASS(QStringListModel)
QT_FORWARD_DECLARE_CLASS(QModelIndex)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ItemModel)

class NewListItemLineEdit: public QLineEdit
{
    Q_OBJECT
public:
    explicit NewListItemLineEdit(
        ItemModel * pItemModel,
        const QStringList & reservedItemNames,
        const QString & linkedNotebookGuid,
        QWidget * parent = Q_NULLPTR);

    virtual ~NewListItemLineEdit();

    QStringList reservedItemNames() const;
    void updateReservedItemNames(const QStringList & reservedItemNames);

    QString linkedNotebookGuid() const;

    virtual QSize sizeHint() const Q_DECL_OVERRIDE;
    virtual QSize minimumSizeHint() const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void receivedFocusFromWindowSystem();

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void focusInEvent(QFocusEvent * pEvent) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onModelRowsInserted(const QModelIndex & parent, int start, int end);
    void onModelRowsRemoved(const QModelIndex & parent, int start, int end);

    void onModelDataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = QVector<int>());

private:
    void setupCompleter();

private:
    Ui::NewListItemLineEdit *   m_pUi;
    QPointer<ItemModel>         m_pItemModel;
    QStringList                 m_reservedItemNames;
    QString                     m_linkedNotebookGuid;
    QStringListModel *          m_pItemNamesModel;
    QCompleter *                m_pCompleter;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_NEW_LIST_ITEM_LINE_EDIT_H
