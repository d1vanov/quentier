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
#include <quentier/utility/DesktopServices.h>
#include <QStringListModel>
#include <QPushButton>

using namespace quentier;

AddAccountDialog::AddAccountDialog(const QVector<Account> & availableAccounts,
                                   QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::AddAccountDialog),
    m_availableAccounts(availableAccounts),
    m_onceSuggestedFullName(false)
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

    QObject::connect(m_pUi->accountUsernameLineEdit, QNSIGNAL(QLineEdit,editingFinished),
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

    // The username for Evernote account would be acquired through OAuth
    m_pUi->accountUsernameLabel->setHidden(true);
    m_pUi->accountUsernameLineEdit->setHidden(true);

    // As well as the full name
    m_pUi->accountFullNameLabel->setHidden(true);
    m_pUi->accountFullNameLineEdit->setHidden(true);
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
    return m_pUi->accountUsernameLineEdit->text();
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

QString AddAccountDialog::userFullName() const
{
    return m_pUi->accountFullNameLineEdit->text();
}

void AddAccountDialog::onCurrentAccountTypeChanged(int index)
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onCurrentAccountTypeChanged: index = ") << index);

    bool isLocal = (index != 0);

    m_pUi->evernoteServerComboBox->setHidden(isLocal);
    m_pUi->evernoteServerLabel->setHidden(isLocal);

    m_pUi->accountUsernameLineEdit->setHidden(!isLocal);
    m_pUi->accountUsernameLabel->setHidden(!isLocal);

    m_pUi->accountFullNameLineEdit->setHidden(!isLocal);
    m_pUi->accountFullNameLabel->setHidden(!isLocal);

    if (isLocal && !m_onceSuggestedFullName &&
        m_pUi->accountFullNameLineEdit->text().isEmpty())
    {
        m_onceSuggestedFullName = true;
        QString fullName = getCurrentUserFullName();
        QNTRACE(QStringLiteral("Suggesting the current user's full name: ") << fullName);
        m_pUi->accountFullNameLineEdit->setText(fullName);
    }
}

void AddAccountDialog::onLocalAccountNameChosen()
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onLocalAccountNameChosen: ")
            << m_pUi->accountUsernameLineEdit->text());

    m_pUi->statusText->setHidden(true);

    QPushButton * pOkButton = m_pUi->buttonBox->button(QDialogButtonBox::Ok);
    if (pOkButton) {
        pOkButton->setDisabled(false);
    }

    if (localAccountAlreadyExists(m_pUi->accountUsernameLineEdit->text()))
    {
        QNDEBUG(QStringLiteral("Local account with such name already exists"));

        m_pUi->statusText->setText(tr("Account with such name already exists"));
        m_pUi->statusText->setHidden(false);
        if (pOkButton) {
            pOkButton->setDisabled(true);
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
    bool isLocal = (m_pUi->accountTypeComboBox->currentIndex() != 0);

    QString name;
    if (isLocal)
    {
        name = m_pUi->accountUsernameLineEdit->text();
        if (name.isEmpty()) {
            m_pUi->statusText->setText(tr("Please enter the name for the account"));
            m_pUi->statusText->setHidden(false);
            return;
        }
    }

    if (isLocal && localAccountAlreadyExists(name)) {
        return;
    }

    if (isLocal) {
        QString fullName = userFullName();
        emit localAccountAdditionRequested(name, fullName);
    }
    else {
        QString server = evernoteServerUrl();
        emit evernoteAccountAdditionRequested(server);
    }

    QDialog::accept();
}
