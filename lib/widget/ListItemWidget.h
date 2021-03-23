/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_WIDGET_LIST_ITEM_WIDGET_H
#define QUENTIER_LIB_WIDGET_LIST_ITEM_WIDGET_H

#include <QWidget>

namespace Ui {
class ListItemWidget;
}

namespace quentier {

/**
 * @brief The ListItemWidget class represents a single item within some list,
 * for example, a tag within the list of currently selected note's tags
 *
 * It is a very simple widget combining the item name label and the button
 * intended to signal the desire to remove the item from the list
 */
class ListItemWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit ListItemWidget(
        const QString & itemName, const QString & itemLocalId,
        QWidget * parent = nullptr);

    explicit ListItemWidget(
        const QString & itemName, const QString & itemLocalId,
        const QString & linkedNotebookGuid,
        const QString & linkedNotebookUsername, QWidget * parent = nullptr);

    ~ListItemWidget() override;

    [[nodiscard]] QString name() const;
    void setName(const QString & name);

    [[nodiscard]] QString localId() const;
    void setLocalId(QString localId);

    [[nodiscard]] QString linkedNotebookUsername() const;
    void setLinkedNotebookUsername(const QString & name);

    [[nodiscard]] QString linkedNotebookGuid() const;
    void setLinkedNotebookGuid(QString guid);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

Q_SIGNALS:
    void itemRemovedFromList(
        QString localId, QString name, QString linkedNotebookGuid,
        QString linkedNotebookUsername);

public Q_SLOTS:
    void setItemRemovable(bool removable);

private Q_SLOTS:
    void onRemoveItemButtonPressed();

private:
    Ui::ListItemWidget * m_pUi;

    QString m_itemLocalId;
    QString m_linkedNotebookGuid;
};

} // namespace quentier

#endif // QUENTIER_LIB_WIDGET_LIST_ITEM_WIDGET_H
