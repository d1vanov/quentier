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

#include "ListItemWidget.h"
#include "ui_ListItemWidget.h"

#include <quentier/logging/QuentierLogger.h>

namespace quentier {

ListItemWidget::ListItemWidget(
    const QString & itemName, const QString & itemLocalId, QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::ListItemWidget)
{
    m_pUi->setupUi(this);

    m_itemLocalId = itemLocalId;
    setName(itemName);
    adjustSize();

    m_pUi->userLabel->hide();
    m_pUi->linkedNotebookUsernameLabel->hide();

    QObject::connect(
        m_pUi->deleteItemButton, &QPushButton::pressed, this,
        &ListItemWidget::onRemoveItemButtonPressed);

    QNDEBUG(
        "widget:list_item_widget",
        "Created new ListItemWidget: local "
            << "uid = " << m_itemLocalId << ", name = " << itemName);
}

ListItemWidget::ListItemWidget(
    const QString & itemName, const QString & itemLocalId,
    const QString & linkedNotebookGuid, const QString & linkedNotebookUsername,
    QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::ListItemWidget)
{
    m_pUi->setupUi(this);

    m_itemLocalId = itemLocalId;
    setName(itemName);

    if (linkedNotebookGuid.isEmpty()) {
        m_pUi->userLabel->hide();
        m_pUi->linkedNotebookUsernameLabel->hide();
    }
    else {
        m_linkedNotebookGuid = linkedNotebookGuid;
        setLinkedNotebookUsername(linkedNotebookUsername);
    }

    adjustSize();

    QObject::connect(
        m_pUi->deleteItemButton, &QPushButton::pressed, this,
        &ListItemWidget::onRemoveItemButtonPressed);

    QNDEBUG(
        "widget:list_item_widget",
        "Created new ListItemWidget: local "
            << "uid = " << m_itemLocalId << ", name = " << itemName
            << ", linked notebook guid = " << m_linkedNotebookGuid
            << ", linked noteobok username = " << linkedNotebookUsername);
}

ListItemWidget::~ListItemWidget()
{
    QNDEBUG(
        "widget:list_item_widget",
        "Destroying ListItemWidget: "
            << "local id " << m_itemLocalId
            << ", item name: " << m_pUi->itemNameLabel->text()
            << ", linked notebook guid = " << m_linkedNotebookGuid);

    delete m_pUi;
}

QString ListItemWidget::name() const
{
    return m_pUi->itemNameLabel->text();
}

void ListItemWidget::setName(const QString & name)
{
    m_pUi->itemNameLabel->setText(name);
}

QString ListItemWidget::localId() const
{
    return m_itemLocalId;
}

void ListItemWidget::setLocalId(QString localId)
{
    m_itemLocalId = std::move(localId);
}

QString ListItemWidget::linkedNotebookUsername() const
{
    return m_pUi->linkedNotebookUsernameLabel->text();
}

void ListItemWidget::setLinkedNotebookUsername(const QString & name)
{
    m_pUi->linkedNotebookUsernameLabel->setText(name);
}

QString ListItemWidget::linkedNotebookGuid() const
{
    return m_linkedNotebookGuid;
}

void ListItemWidget::setLinkedNotebookGuid(QString guid)
{
    m_linkedNotebookGuid = std::move(guid);

    if (m_linkedNotebookGuid.isEmpty()) {
        m_pUi->userLabel->hide();
        m_pUi->linkedNotebookUsernameLabel->hide();
    }
    else {
        m_pUi->userLabel->show();
        m_pUi->linkedNotebookUsernameLabel->show();
    }
}

QSize ListItemWidget::sizeHint() const
{
    return m_pUi->frame->minimumSizeHint();
}

QSize ListItemWidget::minimumSizeHint() const
{
    return m_pUi->frame->minimumSizeHint();
}

void ListItemWidget::setItemRemovable(bool removable)
{
    m_pUi->deleteItemButton->setHidden(!removable);
}

void ListItemWidget::onRemoveItemButtonPressed()
{
    QNDEBUG(
        "widget:list_item_widget",
        "Remove button pressed on item with "
            << "local id " << m_itemLocalId
            << ", item name: " << m_pUi->itemNameLabel->text()
            << ", linked notebook guid = " << m_linkedNotebookGuid);

    Q_EMIT itemRemovedFromList(
        m_itemLocalId, m_pUi->itemNameLabel->text(), m_linkedNotebookGuid,
        m_pUi->linkedNotebookUsernameLabel->text());
}

} // namespace quentier
