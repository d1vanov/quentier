/*
 * Copyright 2016 Dmitry Ivanov
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

#ifndef QUENTIER_WIDGETS_NEW_LIST_ITEM_LINE_EDIT_H
#define QUENTIER_WIDGETS_NEW_LIST_ITEM_LINE_EDIT_H

#include <quentier/utility/Qt4Helper.h>
#include <QLineEdit>

namespace Ui {
class NewListItemLineEdit;
}

QT_FORWARD_DECLARE_CLASS(QCompleter)
QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(ItemModel)

class NewListItemLineEdit: public QLineEdit
{
    Q_OBJECT
public:
    explicit NewListItemLineEdit(ItemModel * pItemModel, const QStringList & reservedItemNames,
                                 QWidget * parent = Q_NULLPTR);
    virtual ~NewListItemLineEdit();

    void updateReservedItemNames(const QStringList & reservedItemNames);

protected:
    virtual void keyPressEvent(QKeyEvent * pEvent) Q_DECL_OVERRIDE;

private:
    void setupCompleter();

private:
    Ui::NewListItemLineEdit *   m_pUi;
    ItemModel *                 m_pItemModel;
    QStringList                 m_reservedItemNames;
    QStringListModel *          m_pItemNamesModel;
    QCompleter *                m_pCompleter;
};

} // namespace quentier

#endif // QUENTIER_WIDGETS_NEW_LIST_ITEM_LINE_EDIT_H
