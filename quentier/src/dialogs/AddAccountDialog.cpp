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

#include "AddAccountDialog.h"
#include "ui_AddAccountDialog.h"
#include <QStringListModel>

AddAccountDialog::AddAccountDialog(QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::AddAccountDialog)
{
    m_pUi->setupUi(this);

    QStringList accountTypes;
    accountTypes.reserve(2);
    accountTypes << QStringLiteral("Evernote");
    accountTypes << tr("Local");
    m_pUi->accountTypeComboBox->setModel(new QStringListModel(accountTypes, this));
    m_pUi->accountTypeComboBox->setCurrentIndex(0);

    QObject::connect(m_pUi->accountTypeComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onCurrentAccountTypeChanged(int)));

    QStringList evernoteServers;
    evernoteServers.reserve(3);
    evernoteServers << QStringLiteral("Evernote");
    evernoteServers << QStringLiteral("Yinxiang Biji");
    evernoteServers << QStringLiteral("Evernote sandbox");
    m_pUi->evernoteServerComboBox->setModel(new QStringListModel(evernoteServers, this));
    m_pUi->evernoteServerComboBox->setCurrentIndex(0);
}

AddAccountDialog::~AddAccountDialog()
{
    delete m_pUi;
}

void AddAccountDialog::onCurrentAccountTypeChanged(int index)
{
    m_pUi->evernoteServerComboBox->setDisabled(index != 0);
}
