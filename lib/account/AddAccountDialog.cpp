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

#include "AddAccountDialog.h"
#include "ui_AddAccountDialog.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/System.h>

#include <QPushButton>
#include <QStringListModel>

#include <algorithm>
#include <limits>

namespace quentier {

AddAccountDialog::AddAccountDialog(
    QVector<Account> availableAccounts, QWidget * parent) :
    QDialog(parent),
    m_pUi(new Ui::AddAccountDialog),
    m_availableAccounts(std::move(availableAccounts))
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

    m_pUi->accountTypeComboBox->setModel(
        new QStringListModel(accountTypes, this));

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->accountTypeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &AddAccountDialog::onCurrentAccountTypeChanged);
#else
    QObject::connect(
        m_pUi->accountTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onCurrentAccountTypeChanged(int)));
#endif

    QObject::connect(
        m_pUi->accountUsernameLineEdit, &QLineEdit::editingFinished, this,
        &AddAccountDialog::onLocalAccountNameChosen);

    QObject::connect(
        m_pUi->accountUsernameLineEdit, &QLineEdit::textEdited, this,
        &AddAccountDialog::onLocalAccountUsernameEdited);

    QStringList evernoteServers;
    evernoteServers.reserve(3);
    evernoteServers << QStringLiteral("Evernote");
    evernoteServers << QStringLiteral("Yinxiang Biji");
    evernoteServers << QStringLiteral("Evernote sandbox");

    m_pUi->evernoteServerComboBox->setModel(
        new QStringListModel(evernoteServers, this));

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
    switch (m_pUi->evernoteServerComboBox->currentIndex()) {
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
    QNDEBUG(
        "account",
        "AddAccountDialog::onCurrentAccountTypeChanged: index = " << index);

    const bool isLocal = (index != 0);

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

        QNTRACE(
            "account", "Suggesting the current user's full name: " << fullName);

        m_pUi->accountFullNameLineEdit->setText(fullName);
    }
}

void AddAccountDialog::onLocalAccountNameChosen()
{
    QNDEBUG(
        "account",
        "AddAccountDialog::onLocalAccountNameChosen: "
            << m_pUi->accountUsernameLineEdit->text());

    m_pUi->statusText->setHidden(true);

    auto * pOkButton = m_pUi->buttonBox->button(QDialogButtonBox::Ok);
    if (pOkButton) {
        pOkButton->setDisabled(false);
    }

    if (localAccountAlreadyExists(m_pUi->accountUsernameLineEdit->text())) {
        showLocalAccountAlreadyExistsMessage();
        if (pOkButton) {
            pOkButton->setDisabled(true);
        }
    }
}

bool AddAccountDialog::localAccountAlreadyExists(const QString & name) const
{
    for (const auto & availableAccount: qAsConst(m_availableAccounts)) {
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
    QNDEBUG(
        "account",
        "AddAccountDialog::onLocalAccountUsernameEdited: " << username);

    const bool isLocal = (m_pUi->accountTypeComboBox->currentIndex() != 0);
    if (!isLocal) {
        QNTRACE(
            "account",
            "The chosen account type is not local, won't do "
                << "anything");
        return;
    }

    auto * pOkButton = m_pUi->buttonBox->button(QDialogButtonBox::Ok);

    if (localAccountAlreadyExists(username)) {
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
    QNDEBUG(
        "account",
        "AddAccountDialog::onUseNetworkProxyToggled: checked = "
            << (checked ? "true" : "false"));

    m_pUi->networkProxyFrame->setVisible(checked);

    if (!checked) {
        m_pUi->statusText->clear();
        m_pUi->statusText->setHidden(true);
    }

    adjustSize();
}

void AddAccountDialog::onNetworkProxyTypeChanged(int index)
{
    QNDEBUG(
        "account",
        "AddAccountDialog::onNetworkProxyTypeChanged: index = " << index);

    evaluateNetworkProxySettingsValidity();
}

void AddAccountDialog::onNetworkProxyHostChanged()
{
    QNDEBUG("account", "AddAccountDialog::onNetworkProxyHostChanged");
    evaluateNetworkProxySettingsValidity();
}

void AddAccountDialog::onNetworkProxyPortChanged(int port)
{
    QNDEBUG(
        "account",
        "AddAccountDialog::onNetworkProxyPortChanged: port = " << port);

    evaluateNetworkProxySettingsValidity();
}

void AddAccountDialog::onNetworkProxyShowPasswordToggled(bool checked)
{
    QNDEBUG(
        "account",
        "AddAccountDialog::onNetworkProxyShowPasswordToggled: "
            << "checked = " << (checked ? "true" : "false"));

    m_pUi->networkProxyPasswordLineEdit->setEchoMode(
        checked ? QLineEdit::Normal : QLineEdit::Password);
}

void AddAccountDialog::accept()
{
    const bool isLocal = (m_pUi->accountTypeComboBox->currentIndex() != 0);

    QString name;
    if (isLocal) {
        name = m_pUi->accountUsernameLineEdit->text();
        if (name.isEmpty()) {
            m_pUi->statusText->setText(
                tr("Please enter the name for the account"));

            m_pUi->statusText->setHidden(false);
            return;
        }
    }

    if (isLocal && localAccountAlreadyExists(name)) {
        return;
    }

    if (isLocal) {
        QString fullName = userFullName();
        Q_EMIT localAccountAdditionRequested(name, fullName);
    }
    else {
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
    QObject::connect(
        m_pUi->useNetworkProxyCheckBox, &QCheckBox::toggled, this,
        &AddAccountDialog::onUseNetworkProxyToggled);

    // 3) Setup the network proxy types combo box
    QStringList networkProxyTypes;
    networkProxyTypes.reserve(3);

    // Allow only "No proxy", "Http proxy" and "Socks5 proxy"
    // If proxy type parsed from the settings is different, fall back to
    // "No proxy". Also treat QNetworkProxy::DefaultProxy as "No proxy" because
    // the default proxy type of QNetworkProxy is QNetworkProxy::NoProxy

    const QString noProxyItem = tr("No proxy");
    const QString httpProxyItem = tr("Http proxy");
    const QString socks5ProxyItem = tr("Socks5 proxy");

    networkProxyTypes << noProxyItem;
    networkProxyTypes << httpProxyItem;
    networkProxyTypes << socks5ProxyItem;

    m_pUi->networkProxyTypeComboBox->setModel(
        new QStringListModel(networkProxyTypes, this));

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->networkProxyTypeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &AddAccountDialog::onNetworkProxyTypeChanged);
#else
    QObject::connect(
        m_pUi->networkProxyTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
        SLOT(onNetworkProxyTypeChanged(int)));
#endif

    // 4) Setup the valid range for port spin box
    m_pUi->networkProxyPortSpinBox->setMinimum(0);
    m_pUi->networkProxyPortSpinBox->setMaximum(
        std::max(std::numeric_limits<quint16>::max() - 1, 0));

    // 5) Connect to other editors of network proxy pieces

    QObject::connect(
        m_pUi->networkProxyHostLineEdit, &QLineEdit::editingFinished, this,
        &AddAccountDialog::onNetworkProxyHostChanged);

#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    QObject::connect(
        m_pUi->networkProxyPortSpinBox, qOverload<int>(&QSpinBox::valueChanged),
        this, &AddAccountDialog::onNetworkProxyPortChanged);
#else
    QObject::connect(
        m_pUi->networkProxyPortSpinBox, SIGNAL(valueChanged(int)), this,
        SLOT(onNetworkProxyPortChanged(int)));
#endif

    QObject::connect(
        m_pUi->networkProxyPasswordShowCheckBox, &QCheckBox::toggled, this,
        &AddAccountDialog::onNetworkProxyShowPasswordToggled);

    // NOTE: don't need to watch for edits of network proxy username and
    // password since can't judge their influence on the validity of network
    // proxy anyway
}

void AddAccountDialog::showLocalAccountAlreadyExistsMessage()
{
    ErrorString message(
        QT_TR_NOOP("Account with such username already exists"));
    QNDEBUG("account", message);
    m_pUi->statusText->setText(message.localizedString());
    m_pUi->statusText->setHidden(false);
}

void AddAccountDialog::evaluateNetworkProxySettingsValidity()
{
    QNDEBUG(
        "account", "AddAccountDialog::evaluateNetworkProxySettingsValidity");

    auto * pOkButton = m_pUi->buttonBox->button(QDialogButtonBox::Ok);

    ErrorString errorDescription;
    if ((m_pUi->accountTypeComboBox->currentIndex() == 0) &&
        m_pUi->useNetworkProxyCheckBox->isChecked())
    {
        Q_UNUSED(networkProxy(errorDescription))
    }

    if (!errorDescription.isEmpty()) {
        m_pUi->statusText->setText(errorDescription.localizedString());
        m_pUi->statusText->show();

        if (pOkButton) {
            pOkButton->setEnabled(false);
        }
    }
    else {
        m_pUi->statusText->clear();
        m_pUi->statusText->hide();

        if (pOkButton) {
            pOkButton->setEnabled(true);
        }
    }
}

QNetworkProxy AddAccountDialog::networkProxy(
    ErrorString & errorDescription) const
{
    QNDEBUG("account", "AddAccountDialog::networkProxy");

    errorDescription.clear();

    QNetworkProxy proxy;

    const int proxyTypeInt = m_pUi->networkProxyTypeComboBox->currentIndex();
    switch (proxyTypeInt) {
    case 1:
        proxy.setType(QNetworkProxy::HttpProxy);
        break;
    case 2:
        proxy.setType(QNetworkProxy::Socks5Proxy);
        break;
    default:
    {
        proxy.setType(QNetworkProxy::NoProxy);
        QNTRACE("account", "No proxy");
        return proxy;
    }
    }

    const QString host = m_pUi->networkProxyHostLineEdit->text();
    const QUrl proxyHostUrl = host;
    if (!proxyHostUrl.isValid()) {
        errorDescription.setBase(
            QT_TR_NOOP("Network proxy host url is not valid"));

        errorDescription.setDetails(host);
        QNDEBUG("account", errorDescription);
        return QNetworkProxy(QNetworkProxy::NoProxy);
    }

    proxy.setHostName(host);

    const int proxyPort = m_pUi->networkProxyPortSpinBox->value();
    if (Q_UNLIKELY(
            (proxyPort < 0) ||
            (proxyPort >= std::numeric_limits<quint16>::max())))
    {
        errorDescription.setBase(QT_TR_NOOP("Network proxy port is not valid"));
        errorDescription.setDetails(QString::number(proxyPort));
        QNDEBUG("account", errorDescription);
        return QNetworkProxy(QNetworkProxy::NoProxy);
    }

    proxy.setPort(static_cast<quint16>(proxyPort));

    proxy.setUser(m_pUi->networkProxyUserLineEdit->text());
    proxy.setPassword(m_pUi->networkProxyPasswordLineEdit->text());

    QNTRACE(
        "account",
        "Network proxy: type = "
            << proxy.type() << ", host = " << proxy.hostName()
            << ", port = " << proxy.port() << ", user = " << proxy.user());
    return proxy;
}

} // namespace quentier
