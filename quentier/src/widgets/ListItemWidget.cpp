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

#include "ListItemWidget.h"
#include "ui_ListItemWidget.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

ListItemWidget::ListItemWidget(const QString & name, QWidget *parent) :
    QWidget(parent),
    m_pUi(new Ui::ListItemWidget)
{
    m_pUi->setupUi(this);

    setName(name);
    adjustSize();

    QObject::connect(m_pUi->deleteItemButton, QNSIGNAL(QPushButton,clicked),
                     this, QNSLOT(ListItemWidget,onRemoveItemButtonPressed));
}

ListItemWidget::~ListItemWidget()
{
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

void ListItemWidget::setItemRemovable(bool removable)
{
    m_pUi->deleteItemButton->setHidden(!removable);
}

void ListItemWidget::onRemoveItemButtonPressed()
{
    emit itemRemovedFromList(m_pUi->itemNameLabel->text());
}

} // namespace quentier
