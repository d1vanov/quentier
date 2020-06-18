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

#include "ListItemWidget.h"
#include "ui_ListItemWidget.h"

#include <quentier/logging/QuentierLogger.h>

namespace quentier {

ListItemWidget::ListItemWidget(
        const QString & itemName, const QString & itemLocalUid,
        QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::ListItemWidget)
{
    m_pUi->setupUi(this);

    m_itemLocalUid = itemLocalUid;
    setName(itemName);
    adjustSize();

    m_pUi->userLabel->hide();
    m_pUi->linkedNotebookUsernameLabel->hide();

    QObject::connect(
        m_pUi->deleteItemButton,
        &QPushButton::clicked,
        this,
        &ListItemWidget::onRemoveItemButtonPressed);
}

ListItemWidget::ListItemWidget(
        const QString & itemName, const QString & itemLocalUid,
        const QString & linkedNotebookUsername,
        const QString & linkedNotebookGuid, QWidget * parent) :
    QWidget(parent),
    m_pUi(new Ui::ListItemWidget)
{
    m_pUi->setupUi(this);

    m_itemLocalUid = itemLocalUid;
    setName(itemName);

    m_linkedNotebookGuid = linkedNotebookGuid;
    setLinkedNotebookUsername(linkedNotebookUsername);

    adjustSize();

    QObject::connect(
        m_pUi->deleteItemButton,
        &QPushButton::clicked,
        this,
        &ListItemWidget::onRemoveItemButtonPressed);
}

ListItemWidget::~ListItemWidget()
{
    delete m_pUi;
}

QString ListItemWidget::name() const
{
    return m_pUi->itemNameLabel->text();
}

void ListItemWidget::setName(QString name)
{
    m_pUi->itemNameLabel->setText(std::move(name));
}

QString ListItemWidget::localUid() const
{
    return m_itemLocalUid;
}

void ListItemWidget::setLocalUid(QString localUid)
{
    m_itemLocalUid = std::move(localUid);
}

QString ListItemWidget::linkedNotebookUsername() const
{
    return m_pUi->linkedNotebookUsernameLabel->text();
}

void ListItemWidget::setLinkedNotebookUsername(QString name)
{
    m_pUi->linkedNotebookUsernameLabel->setText(std::move(name));
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
    Q_EMIT itemRemovedFromList(
        m_itemLocalUid,
        m_pUi->itemNameLabel->text(),
        m_linkedNotebookGuid,
        m_pUi->linkedNotebookUsernameLabel->text());
}

} // namespace quentier
