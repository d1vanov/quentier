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

#include "ListItemWidget.h"
#include "ui_ListItemWidget.h"

#include <quentier/logging/QuentierLogger.h>

namespace quentier {

ListItemWidget::ListItemWidget(
    QString itemName, QString itemLocalId, QWidget * parent) :
    QWidget{parent},
    m_ui{new Ui::ListItemWidget}
{
    m_ui->setupUi(this);

    m_itemLocalId = std::move(itemLocalId);
    setName(std::move(itemName));
    adjustSize();

    m_ui->userLabel->hide();
    m_ui->linkedNotebookUsernameLabel->hide();

    QObject::connect(
        m_ui->deleteItemButton, &QPushButton::pressed, this,
        &ListItemWidget::onRemoveItemButtonPressed);

    QNDEBUG(
        "widget::ListItemWidget",
        "Created new ListItemWidget: local "
            << "id = " << m_itemLocalId << ", name = " << itemName);
}

ListItemWidget::ListItemWidget(
    QString itemName, QString itemLocalId, QString linkedNotebookGuid,
    QString linkedNotebookUsername, QWidget * parent) :
    QWidget{parent},
    m_ui{new Ui::ListItemWidget}
{
    m_ui->setupUi(this);

    m_itemLocalId = std::move(itemLocalId);
    setName(std::move(itemName));

    if (linkedNotebookGuid.isEmpty()) {
        m_ui->userLabel->hide();
        m_ui->linkedNotebookUsernameLabel->hide();
    }
    else {
        m_linkedNotebookGuid = std::move(linkedNotebookGuid);
        setLinkedNotebookUsername(std::move(linkedNotebookUsername));
    }

    adjustSize();

    QObject::connect(
        m_ui->deleteItemButton, &QPushButton::pressed, this,
        &ListItemWidget::onRemoveItemButtonPressed);

    QNDEBUG(
        "widget::ListItemWidget",
        "Created new ListItemWidget: local id = "
            << m_itemLocalId << ", name = " << itemName
            << ", linked notebook guid = " << m_linkedNotebookGuid
            << ", linked notebook username = " << linkedNotebookUsername);
}

ListItemWidget::~ListItemWidget()
{
    QNDEBUG(
        "widget::ListItemWidget",
        "Destroying ListItemWidget: "
            << "local id " << m_itemLocalId
            << ", item name: " << m_ui->itemNameLabel->text()
            << ", linked notebook guid = " << m_linkedNotebookGuid);

    delete m_ui;
}

QString ListItemWidget::name() const
{
    return m_ui->itemNameLabel->text();
}

void ListItemWidget::setName(QString name)
{
    m_ui->itemNameLabel->setText(std::move(name));
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
    return m_ui->linkedNotebookUsernameLabel->text();
}

void ListItemWidget::setLinkedNotebookUsername(QString name)
{
    m_ui->linkedNotebookUsernameLabel->setText(std::move(name));
}

QString ListItemWidget::linkedNotebookGuid() const
{
    return m_linkedNotebookGuid;
}

void ListItemWidget::setLinkedNotebookGuid(QString guid)
{
    m_linkedNotebookGuid = std::move(guid);

    if (m_linkedNotebookGuid.isEmpty()) {
        m_ui->userLabel->hide();
        m_ui->linkedNotebookUsernameLabel->hide();
    }
    else {
        m_ui->userLabel->show();
        m_ui->linkedNotebookUsernameLabel->show();
    }
}

QSize ListItemWidget::sizeHint() const
{
    return m_ui->frame->minimumSizeHint();
}

QSize ListItemWidget::minimumSizeHint() const
{
    return m_ui->frame->minimumSizeHint();
}

void ListItemWidget::setItemRemovable(bool removable)
{
    m_ui->deleteItemButton->setHidden(!removable);
}

void ListItemWidget::onRemoveItemButtonPressed()
{
    QNDEBUG(
        "widget::ListItemWidget",
        "Remove button pressed on item with local id "
            << m_itemLocalId << ", item name: " << m_ui->itemNameLabel->text()
            << ", linked notebook guid = " << m_linkedNotebookGuid);

    Q_EMIT itemRemovedFromList(
        m_itemLocalId, m_ui->itemNameLabel->text(), m_linkedNotebookGuid,
        m_ui->linkedNotebookUsernameLabel->text());
}

} // namespace quentier
