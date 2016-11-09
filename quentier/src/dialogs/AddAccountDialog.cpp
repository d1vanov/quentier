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
#include <quentier/logging/QuentierLogger.h>
#include <QStringListModel>
#include <QPushButton>

using namespace quentier;

AddAccountDialog::AddAccountDialog(const QVector<Account> & availableAccounts,
                                   QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::AddAccountDialog),
    m_availableAccounts(availableAccounts)
{
    m_pUi->setupUi(this);

    m_pUi->statusText->setHidden(true);

    QStringList accountTypes;
    accountTypes.reserve(2);
    accountTypes << QStringLiteral("Evernote");
    accountTypes << tr("Local");
    m_pUi->accountTypeComboBox->setModel(new QStringListModel(accountTypes, this));

    QObject::connect(m_pUi->accountTypeComboBox, SIGNAL(currentIndexChanged(int)),
                     this, SLOT(onCurrentAccountTypeChanged(int)));

    QObject::connect(m_pUi->accountNameLineEdit, QNSIGNAL(QLineEdit,editingFinished),
                     this, QNSLOT(AddAccountDialog,onLocalAccountNameChosen));

    QStringList evernoteServers;
    evernoteServers.reserve(3);
    evernoteServers << QStringLiteral("Evernote");
    evernoteServers << QStringLiteral("Yinxiang Biji");
    evernoteServers << QStringLiteral("Evernote sandbox");
    m_pUi->evernoteServerComboBox->setModel(new QStringListModel(evernoteServers, this));

    // Offer the creation of a new Evernote account by default
    m_pUi->accountTypeComboBox->setCurrentIndex(0);
    m_pUi->evernoteServerComboBox->setCurrentIndex(0);

    // The name for Evernote account would be acquired through OAuth
    m_pUi->accountNameLabel->setHidden(true);
    m_pUi->accountNameLineEdit->setHidden(true);
}

AddAccountDialog::~AddAccountDialog()
{
    delete m_pUi;
}

bool AddAccountDialog::isLocal() const
{
    return (m_pUi->accountTypeComboBox->currentIndex() != 0);
}

QString AddAccountDialog::localAccountName() const
{
    return m_pUi->accountNameLineEdit->text();
}

QString AddAccountDialog::evernoteServerUrl() const
{
    switch(m_pUi->evernoteServerComboBox->currentIndex())
    {
    case 1:
        return QStringLiteral("app.yinxiang.com");
    case 2:
        return QStringLiteral("sandbox.evernote.com");
    default:
        return QStringLiteral("www.evernote.com");
    }
}

void AddAccountDialog::onCurrentAccountTypeChanged(int index)
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onCurrentAccountTypeChanged: index = ") << index);

    bool isLocal = (index != 0);

    m_pUi->evernoteServerComboBox->setHidden(isLocal);
    m_pUi->evernoteServerLabel->setHidden(isLocal);

    m_pUi->accountNameLineEdit->setHidden(!isLocal);
    m_pUi->accountNameLabel->setHidden(!isLocal);
}

void AddAccountDialog::onLocalAccountNameChosen()
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onLocalAccountNameChosen: ")
            << m_pUi->accountNameLineEdit->text());

    m_pUi->statusText->setHidden(true);

    QPushButton * okButton = m_pUi->buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setDisabled(false);
    }

    if (localAccountAlreadyExists(m_pUi->accountNameLineEdit->text()))
    {
        QNDEBUG(QStringLiteral("Local account with such name already exists"));

        m_pUi->statusText->setText(tr("Account with such name already exists"));
        m_pUi->statusText->setHidden(false);
        if (okButton) {
            okButton->setDisabled(true);
        }
    }
}

bool AddAccountDialog::localAccountAlreadyExists(const QString & name) const
{
    for(int i = 0, size = m_availableAccounts.size(); i < size; ++i)
    {
        const Account & availableAccount = m_availableAccounts[i];
        if (availableAccount.type() != Account::Type::Local) {
            continue;
        }

        if (name == availableAccount.name()) {
            return true;
        }
    }

    return false;
}

void AddAccountDialog::accept()
{
    bool isLocal = (m_pUi->accountTypeComboBox->currentIndex() == 0);
    QString name = m_pUi->accountNameLineEdit->text();

    if (isLocal && localAccountAlreadyExists(name)) {
        QDialog::reject();
        return;
    }

    if (isLocal) {
        emit localAccountAdditionRequested(name);
    }
    else {
        QString server = evernoteServerUrl();
        emit evernoteAccountAdditionRequested(server);
    }

    QDialog::accept();
}
