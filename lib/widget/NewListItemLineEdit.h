/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include <QHash>
#include <QLineEdit>
#include <QList>
#include <QPointer>

namespace Ui {

class NewListItemLineEdit;

} // namespace Ui

class QCompleter;
class QModelIndex;
class QStringListModel;

namespace quentier {

class AbstractItemModel;

class NewListItemLineEdit final : public QLineEdit
{
    Q_OBJECT
public:
    struct ItemInfo
    {
        QString m_name;
        QString m_linkedNotebookGuid;
        QString m_linkedNotebookUsername;
    };

public:
    explicit NewListItemLineEdit(
        AbstractItemModel * itemModel, QList<ItemInfo> reservedItems,
        QWidget * parent = nullptr);

    ~NewListItemLineEdit() override;

    [[nodiscard]] const QString & targetLinkedNotebookGuid() const;
    void setTargetLinkedNotebookGuid(QString linkedNotebookGuid);

    [[nodiscard]] QList<ItemInfo> reservedItems() const;
    void setReservedItems(QList<ItemInfo> items);

    void addReservedItem(ItemInfo item);
    void removeReservedItem(ItemInfo item);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

Q_SIGNALS:
    void receivedFocusFromWindowSystem();

protected:
    void keyPressEvent(QKeyEvent * event) override;
    void focusInEvent(QFocusEvent * event) override;

private Q_SLOTS:
    void onModelRowsInserted(const QModelIndex & parent, int start, int end);
    void onModelRowsRemoved(const QModelIndex & parent, int start, int end);

    void onModelDataChanged(
        const QModelIndex & topLeft, const QModelIndex & bottomRight,
        const QVector<int> & roles = QVector<int>());

private:
    void setupCompleter();

    [[nodiscard]] QStringList itemNamesForCompleter() const;

private:
    Ui::NewListItemLineEdit * m_ui;
    QPointer<AbstractItemModel> m_itemModel;
    QList<ItemInfo> m_reservedItems;
    QStringListModel * m_itemNamesModel;
    QCompleter * m_completer;
    QString m_targetLinkedNotebookGuid;
};

} // namespace quentier
