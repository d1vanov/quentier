/*
 * Copyright 2018 Dmitry Ivanov
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

#include "IconManagementWidget.h"
#include "ui_IconManagementWidget.h"
#include "../IconThemeManager.h"
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>

namespace quentier {

IconManagementWidget::IconManagementWidget(IProvider & provider, QWidget * parent) :
    QWidget(parent, Qt::Window),
    m_pUi(new Ui::IconManagementWidget),
    m_provider(provider),
    m_pickOverrideIconPushButtons(),
    m_overrideIconFilePathLineEdits()
{
    m_pUi->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    setupOverrideIconsTableWidget();
}

IconManagementWidget::~IconManagementWidget()
{
    delete m_pUi;
}

void IconManagementWidget::setupOverrideIconsTableWidget()
{
    QStringList usedIconNames = m_provider.usedIconNames();
    IconThemeManager & iconThemeManager = m_provider.iconThemeManager();

    int numIcons = usedIconNames.size();

    m_pUi->overrideIconsTableWidget->setRowCount(numIcons);
    m_pUi->overrideIconsTableWidget->setColumnCount(5);

    m_pickOverrideIconPushButtons.clear();
    m_pickOverrideIconPushButtons.reserve(numIcons);

    m_overrideIconFilePathLineEdits.clear();
    m_overrideIconFilePathLineEdits.reserve(numIcons);

    for(int i = 0; i < numIcons; ++i)
    {
        QTableWidgetItem * pIconFromThemeItem = new QTableWidgetItem(tr("Icon from theme"));
        pIconFromThemeItem->setIcon(QIcon::fromTheme(usedIconNames[i]));
        m_pUi->overrideIconsTableWidget->setItem(i, 0, pIconFromThemeItem);

        QTableWidgetItem * pIconFromFallbackThemeItem = new QTableWidgetItem(tr("Fallback icon"));
        pIconFromFallbackThemeItem->setIcon(iconThemeManager.iconFromFallbackTheme(usedIconNames[i]));
        m_pUi->overrideIconsTableWidget->setItem(i, 1, pIconFromFallbackThemeItem);

        QTableWidgetItem * pOverrideIconItem = new QTableWidgetItem(tr("Override icon"));
        pOverrideIconItem->setIcon(iconThemeManager.overrideIconFromTheme(usedIconNames[i]));
        m_pUi->overrideIconsTableWidget->setItem(i, 2, pOverrideIconItem);

        QWidget * pPickOverrideIconPushButtonWidget = new QWidget;
        QPushButton * pPickOverrideIconPushButton = new QPushButton;
        pPickOverrideIconPushButton->setObjectName(QString::number(i));
        QHBoxLayout * pPickOverrideIconPushButtonLayout = new QHBoxLayout(pPickOverrideIconPushButtonWidget);
        pPickOverrideIconPushButtonLayout->addWidget(pPickOverrideIconPushButton);
        pPickOverrideIconPushButtonLayout->setAlignment(Qt::AlignCenter);
        pPickOverrideIconPushButtonLayout->setContentsMargins(0, 0, 0, 0);
        pPickOverrideIconPushButtonWidget->setLayout(pPickOverrideIconPushButtonLayout);
        m_pUi->overrideIconsTableWidget->setCellWidget(i, 3, pPickOverrideIconPushButtonWidget);
        m_pickOverrideIconPushButtons << pPickOverrideIconPushButton;
        // TODO: connect to push button

        QWidget * pOverrideIconFilePathLineEditWidget = new QWidget;
        QLineEdit * pOverrideIconFilePathLineEdit = new QLineEdit;
        pOverrideIconFilePathLineEdit->setReadOnly(true);
        pOverrideIconFilePathLineEdit->setObjectName(QString::number(i));
        QHBoxLayout * pOverrideIconFilePathLineEditLayout = new QHBoxLayout(pOverrideIconFilePathLineEditWidget);
        pOverrideIconFilePathLineEditLayout->addWidget(pOverrideIconFilePathLineEdit);
        pOverrideIconFilePathLineEditLayout->setAlignment(Qt::AlignCenter);
        pOverrideIconFilePathLineEditLayout->setContentsMargins(0, 0, 0, 0);
        pOverrideIconFilePathLineEditWidget->setLayout(pOverrideIconFilePathLineEditLayout);
        m_pUi->overrideIconsTableWidget->setCellWidget(i, 4, pOverrideIconFilePathLineEditWidget);
        m_overrideIconFilePathLineEdits << pOverrideIconFilePathLineEdit;
    }
}

} // namespace quentier
