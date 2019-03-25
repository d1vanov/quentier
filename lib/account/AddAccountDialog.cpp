/*
 * Copyright 2016-2019 Dmitry Ivanov
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
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/Utility.h>
#include <quentier/types/ErrorString.h>

#include <QStringListModel>
#include <QPushButton>

#include <algorithm>
#include <limits>

namespace quentier {

AddAccountDialog::AddAccountDialog(const QVector<Account> & availableAccounts,
                                   QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::AddAccountDialog),
    m_availableAccounts(availableAccounts),
    m_onceSuggestedFullName(false)
{
    m_pUi->setupUi(this);
    setWindowTitle(tr("Add account"));

    setupNetworkProxySettingsFrame();
    adjustSize();

    m_pUi->statusText->setHidden(true);

    QStringList accountTypes;
    accountTypes.reserve(2);
    accountTypes << QStringLiteral("Evernote");
    accountTypes << tr("Local");
    m_pUi->accountTypeComboBox->setModel(new QStringListModel(accountTypes, this));

    QObject::connect(m_pUi->accountTypeComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onCurrentAccountTypeChanged(int)));

    QObject::connect(m_pUi->accountUsernameLineEdit,
                     QNSIGNAL(QLineEdit,editingFinished),
                     this,
                     QNSLOT(AddAccountDialog,onLocalAccountNameChosen));
    QObject::connect(m_pUi->accountUsernameLineEdit,
                     QNSIGNAL(QLineEdit,textEdited,QString),
                     this,
                     QNSLOT(AddAccountDialog,onLocalAccountUsernameEdited,QString));

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
    QNDEBUG(QStringLiteral("AddAccountDialog::onCurrentAccountTypeChanged: ")
            << QStringLiteral("index = ") << index);

    bool isLocal = (index != 0);

    m_pUi->useNetworkProxyCheckBox->setHidden(isLocal);
    m_pUi->networkProxyFrame->setHidden(
        isLocal || !m_pUi->useNetworkProxyCheckBox->isChecked());

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
        QNTRACE(QStringLiteral("Suggesting the current user's full name: ")
                << fullName);
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
        showLocalAccountAlreadyExistsMessage();
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

void AddAccountDialog::onLocalAccountUsernameEdited(const QString & username)
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onLocalAccountUsernameEdited: ")
            << username);

    bool isLocal = (m_pUi->accountTypeComboBox->currentIndex() != 0);
    if (!isLocal) {
        QNTRACE(QStringLiteral("The chosen account type is not local, won't do "
                               "anything"));
        return;
    }

    QPushButton * pOkButton = m_pUi->buttonBox->button(QDialogButtonBox::Ok);

    if (localAccountAlreadyExists(username))
    {
        showLocalAccountAlreadyExistsMessage();
        if (pOkButton) {
            pOkButton->setDisabled(true);
        }

        return;
    }

    m_pUi->statusText->setText(QString());
    m_pUi->statusText->setHidden(true);
    if (pOkButton) {
        pOkButton->setDisabled(false);
    }
}

void AddAccountDialog::onUseNetworkProxyToggled(bool checked)
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onUseNetworkProxyToggled: checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));
    m_pUi->networkProxyFrame->setVisible(checked);

    if (!checked) {
        m_pUi->statusText->clear();
        m_pUi->statusText->setHidden(true);
    }

    adjustSize();
}

void AddAccountDialog::onNetworkProxyTypeChanged(int index)
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onNetworkProxyTypeChanged: index = ")
            << index);
    evaluateNetworkProxySettingsValidity();
}

void AddAccountDialog::onNetworkProxyHostChanged()
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onNetworkProxyHostChanged"));
    evaluateNetworkProxySettingsValidity();
}

void AddAccountDialog::onNetworkProxyPortChanged(int port)
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onNetworkProxyPortChanged: port = ")
            << port);
    evaluateNetworkProxySettingsValidity();
}

void AddAccountDialog::onNetworkProxyShowPasswordToggled(bool checked)
{
    QNDEBUG(QStringLiteral("AddAccountDialog::onNetworkProxyShowPasswordToggled: ")
            << QStringLiteral("checked = ")
            << (checked ? QStringLiteral("true") : QStringLiteral("false")));
    m_pUi->networkProxyPasswordLineEdit->setEchoMode(checked
                                                     ? QLineEdit::Normal
                                                     : QLineEdit::Password);
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

    if (isLocal)
    {
        QString fullName = userFullName();
        Q_EMIT localAccountAdditionRequested(name, fullName);
    }
    else
    {
        QNetworkProxy proxy = QNetworkProxy(QNetworkProxy::NoProxy);

        if ((m_pUi->accountTypeComboBox->currentIndex() == 0) &&
            m_pUi->useNetworkProxyCheckBox->isChecked())
        {
            ErrorString errorDescription;

            proxy = networkProxy(errorDescription);
            if (!errorDescription.isEmpty()) {
                proxy = QNetworkProxy(QNetworkProxy::NoProxy);
            }
        }

        QString server = evernoteServerUrl();
        Q_EMIT evernoteAccountAdditionRequested(server, proxy);
    }

    QDialog::accept();
}

void AddAccountDialog::setupNetworkProxySettingsFrame()
{
    // 1) Hide the network proxy frame
    m_pUi->networkProxyFrame->hide();

    // 2) Setup use network proxy checkbox
    QObject::connect(m_pUi->useNetworkProxyCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(AddAccountDialog,onUseNetworkProxyToggled,bool));

    // 3) Setup the network proxy types combo box
    QStringList networkProxyTypes;
    networkProxyTypes.reserve(3);

    // Allow only "No proxy", "Http proxy" and "Socks5 proxy"
    // If proxy type parsed from the settings is different, fall back to "No proxy"
    // Also treat QNetworkProxy::DefaultProxy as "No proxy" because the default
    // proxy type of QNetworkProxy is QNetworkProxy::NoProxy

    QString noProxyItem = tr("No proxy");
    QString httpProxyItem = tr("Http proxy");
    QString socks5ProxyItem = tr("Socks5 proxy");

    networkProxyTypes << noProxyItem;
    networkProxyTypes << httpProxyItem;
    networkProxyTypes << socks5ProxyItem;

    m_pUi->networkProxyTypeComboBox->setModel(
        new QStringListModel(networkProxyTypes, this));

    QObject::connect(m_pUi->networkProxyTypeComboBox,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(onNetworkProxyTypeChanged(int)));

    // 4) Setup the valid range for port spin box
    m_pUi->networkProxyPortSpinBox->setMinimum(0);
    m_pUi->networkProxyPortSpinBox->setMaximum(
        std::max(std::numeric_limits<quint16>::max()-1, 0));

    // 5) Connect to other editors of network proxy pieces

    QObject::connect(m_pUi->networkProxyHostLineEdit,
                     QNSIGNAL(QLineEdit,editingFinished),
                     this,
                     QNSLOT(AddAccountDialog,onNetworkProxyHostChanged));
    QObject::connect(m_pUi->networkProxyPortSpinBox,
                     SIGNAL(valueChanged(int)),
                     this,
                     SLOT(onNetworkProxyPortChanged(int)));

    QObject::connect(m_pUi->networkProxyPasswordShowCheckBox,
                     QNSIGNAL(QCheckBox,toggled,bool),
                     this,
                     QNSLOT(AddAccountDialog,onNetworkProxyShowPasswordToggled,bool));

    // NOTE: don't need to watch for edits of network proxy username and password
    // since can't judge their influence on the validity of network proxy anyway
}

void AddAccountDialog::showLocalAccountAlreadyExistsMessage()
{
    ErrorString message(QT_TR_NOOP("Account with such username already exists"));
    QNDEBUG(message);
    m_pUi->statusText->setText(message.localizedString());
    m_pUi->statusText->setHidden(false);
}

void AddAccountDialog::evaluateNetworkProxySettingsValidity()
{
    QNDEBUG(QStringLiteral("AddAccountDialog::evaluateNetworkProxySettingsValidity"));

    QPushButton * pOkButton = m_pUi->buttonBox->button(QDialogButtonBox::Ok);

    ErrorString errorDescription;
    if ((m_pUi->accountTypeComboBox->currentIndex() == 0) &&
        m_pUi->useNetworkProxyCheckBox->isChecked())
    {
        QNetworkProxy currentProxy = networkProxy(errorDescription);
    }

    if (!errorDescription.isEmpty())
    {
        m_pUi->statusText->setText(errorDescription.localizedString());
        m_pUi->statusText->show();

        if (pOkButton) {
            pOkButton->setEnabled(false);
        }
    }
    else
    {
        m_pUi->statusText->clear();
        m_pUi->statusText->hide();

        if (pOkButton) {
            pOkButton->setEnabled(true);
        }
    }
}

QNetworkProxy AddAccountDialog::networkProxy(ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("AddAccountDialog::networkProxy"));

    errorDescription.clear();

    QNetworkProxy proxy;

    int proxyTypeInt = m_pUi->networkProxyTypeComboBox->currentIndex();
    switch(proxyTypeInt)
    {
    case 1:
        proxy.setType(QNetworkProxy::HttpProxy);
        break;
    case 2:
        proxy.setType(QNetworkProxy::Socks5Proxy);
        break;
    default:
        {
            proxy.setType(QNetworkProxy::NoProxy);
            QNTRACE(QStringLiteral("No proxy"));
            return proxy;
        }
    }

    QString host = m_pUi->networkProxyHostLineEdit->text();
    QUrl proxyHostUrl = host;
    if (!proxyHostUrl.isValid()) {
        errorDescription.setBase(QT_TR_NOOP("Network proxy host url is not valid"));
        errorDescription.setDetails(host);
        QNDEBUG(errorDescription);
        return QNetworkProxy(QNetworkProxy::NoProxy);
    }

    proxy.setHostName(host);

    int proxyPort = m_pUi->networkProxyPortSpinBox->value();
    if (Q_UNLIKELY((proxyPort < 0) ||
                   (proxyPort >= std::numeric_limits<quint16>::max())))
    {
        errorDescription.setBase(QT_TR_NOOP("Network proxy port is not valid"));
        errorDescription.setDetails(QString::number(proxyPort));
        QNDEBUG(errorDescription);
        return QNetworkProxy(QNetworkProxy::NoProxy);
    }

    proxy.setPort(static_cast<quint16>(proxyPort));

    proxy.setUser(m_pUi->networkProxyUserLineEdit->text());
    proxy.setPassword(m_pUi->networkProxyPasswordLineEdit->text());

    QNTRACE(QStringLiteral("Network proxy: type = ") << proxy.type()
            << QStringLiteral(", host = ") << proxy.hostName()
            << QStringLiteral(", port = ") << proxy.port()
            << QStringLiteral(", user = ") << proxy.user());
    return proxy;
}

} // namespace quentier
