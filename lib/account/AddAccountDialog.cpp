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
#include <utility>

namespace quentier {

AddAccountDialog::AddAccountDialog(
    QList<Account> availableAccounts, QWidget * parent) :
    QDialog{parent}, m_availableAccounts{std::move(availableAccounts)},
    m_ui{new Ui::AddAccountDialog}
{
    m_ui->setupUi(this);
    setWindowTitle(tr("Add account"));

    setupNetworkProxySettingsFrame();
    adjustSize();

    m_ui->statusText->setHidden(true);

    QStringList accountTypes;
    accountTypes.reserve(2);
    accountTypes << QStringLiteral("Evernote");
    accountTypes << tr("Local");

    m_ui->accountTypeComboBox->setModel(
        new QStringListModel{accountTypes, this});

    QObject::connect(
        m_ui->accountTypeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &AddAccountDialog::onCurrentAccountTypeChanged);

    QObject::connect(
        m_ui->accountUsernameLineEdit, &QLineEdit::editingFinished, this,
        &AddAccountDialog::onLocalAccountNameChosen);

    QObject::connect(
        m_ui->accountUsernameLineEdit, &QLineEdit::textEdited, this,
        &AddAccountDialog::onLocalAccountUsernameEdited);

    QStringList evernoteServers;
    evernoteServers.reserve(2);
    evernoteServers << QStringLiteral("Evernote");
    evernoteServers << QStringLiteral("Yinxiang Biji");

    m_ui->evernoteServerComboBox->setModel(
        new QStringListModel(evernoteServers, this));

    // Offer the creation of a new Evernote account by default
    m_ui->accountTypeComboBox->setCurrentIndex(0);
    m_ui->evernoteServerComboBox->setCurrentIndex(0);

    // The username for Evernote account would be acquired through OAuth
    m_ui->accountUsernameLabel->setHidden(true);
    m_ui->accountUsernameLineEdit->setHidden(true);

    // As well as the full name
    m_ui->accountFullNameLabel->setHidden(true);
    m_ui->accountFullNameLineEdit->setHidden(true);
}

AddAccountDialog::~AddAccountDialog()
{
    delete m_ui;
}

bool AddAccountDialog::isLocal() const noexcept
{
    return m_ui->accountTypeComboBox->currentIndex() != 0;
}

QString AddAccountDialog::localAccountName() const
{
    return m_ui->accountUsernameLineEdit->text();
}

QString AddAccountDialog::evernoteServerUrl() const
{
    switch (m_ui->evernoteServerComboBox->currentIndex()) {
    case 1:
        return QStringLiteral("app.yinxiang.com");
    default:
        return QStringLiteral("www.evernote.com");
    }
}

QString AddAccountDialog::userFullName() const
{
    return m_ui->accountFullNameLineEdit->text();
}

void AddAccountDialog::onCurrentAccountTypeChanged(int index)
{
    QNDEBUG(
        "account::AddAccountDialog",
        "AddAccountDialog::onCurrentAccountTypeChanged: index = " << index);

    const bool isLocal = (index != 0);

    m_ui->useNetworkProxyCheckBox->setHidden(isLocal);

    m_ui->networkProxyFrame->setHidden(
        isLocal || !m_ui->useNetworkProxyCheckBox->isChecked());

    m_ui->evernoteServerComboBox->setHidden(isLocal);
    m_ui->evernoteServerLabel->setHidden(isLocal);

    m_ui->accountUsernameLineEdit->setHidden(!isLocal);
    m_ui->accountUsernameLabel->setHidden(!isLocal);

    m_ui->accountFullNameLineEdit->setHidden(!isLocal);
    m_ui->accountFullNameLabel->setHidden(!isLocal);

    if (isLocal && !m_onceSuggestedFullName &&
        m_ui->accountFullNameLineEdit->text().isEmpty())
    {
        m_onceSuggestedFullName = true;
        const QString fullName = getCurrentUserFullName();

        QNTRACE(
            "account::AddAccountDialog",
            "Suggesting the current user's full name: " << fullName);

        m_ui->accountFullNameLineEdit->setText(fullName);
    }
}

void AddAccountDialog::onLocalAccountNameChosen()
{
    QNDEBUG(
        "account::AddAccountDialog",
        "AddAccountDialog::onLocalAccountNameChosen: "
            << m_ui->accountUsernameLineEdit->text());

    m_ui->statusText->setHidden(true);

    auto * okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setDisabled(false);
    }

    if (localAccountAlreadyExists(m_ui->accountUsernameLineEdit->text())) {
        showLocalAccountAlreadyExistsMessage();
        if (okButton) {
            okButton->setDisabled(true);
        }
    }
}

bool AddAccountDialog::localAccountAlreadyExists(const QString & name) const
{
    for (const auto & availableAccount: std::as_const(m_availableAccounts)) {
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
        "account::AddAccountDialog",
        "AddAccountDialog::onLocalAccountUsernameEdited: " << username);

    const bool isLocal = (m_ui->accountTypeComboBox->currentIndex() != 0);
    if (!isLocal) {
        QNTRACE(
            "account::AddAccountDialog",
            "The chosen account type is not local, won't do anything");
        return;
    }

    auto * okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);

    if (localAccountAlreadyExists(username)) {
        showLocalAccountAlreadyExistsMessage();
        if (okButton) {
            okButton->setDisabled(true);
        }

        return;
    }

    m_ui->statusText->setText(QString());
    m_ui->statusText->setHidden(true);
    if (okButton) {
        okButton->setDisabled(false);
    }
}

void AddAccountDialog::onUseNetworkProxyToggled(bool checked)
{
    QNDEBUG(
        "account::AddAccountDialog",
        "AddAccountDialog::onUseNetworkProxyToggled: checked = "
            << (checked ? "true" : "false"));

    m_ui->networkProxyFrame->setVisible(checked);

    if (!checked) {
        m_ui->statusText->clear();
        m_ui->statusText->setHidden(true);
    }

    adjustSize();
}

void AddAccountDialog::onNetworkProxyTypeChanged(int index)
{
    QNDEBUG(
        "account::AddAccountDialog",
        "AddAccountDialog::onNetworkProxyTypeChanged: index = " << index);

    evaluateNetworkProxySettingsValidity();
}

void AddAccountDialog::onNetworkProxyHostChanged()
{
    QNDEBUG(
        "account::AddAccountDialog",
        "AddAccountDialog::onNetworkProxyHostChanged");
    evaluateNetworkProxySettingsValidity();
}

void AddAccountDialog::onNetworkProxyPortChanged(int port)
{
    QNDEBUG(
        "account::AddAccountDialog",
        "AddAccountDialog::onNetworkProxyPortChanged: port = " << port);
    evaluateNetworkProxySettingsValidity();
}

void AddAccountDialog::onNetworkProxyShowPasswordToggled(bool checked)
{
    QNDEBUG(
        "account::AddAccountDialog",
        "AddAccountDialog::onNetworkProxyShowPasswordToggled: checked = "
            << (checked ? "true" : "false"));

    m_ui->networkProxyPasswordLineEdit->setEchoMode(
        checked ? QLineEdit::Normal : QLineEdit::Password);
}

void AddAccountDialog::accept()
{
    const bool isLocal = (m_ui->accountTypeComboBox->currentIndex() != 0);

    QString name;
    if (isLocal) {
        name = m_ui->accountUsernameLineEdit->text();
        if (name.isEmpty()) {
            m_ui->statusText->setText(
                tr("Please enter the name for the account"));

            m_ui->statusText->setHidden(false);
            return;
        }
    }

    if (isLocal && localAccountAlreadyExists(name)) {
        return;
    }

    if (isLocal) {
        QString fullName = userFullName();
        Q_EMIT localAccountAdditionRequested(
            std::move(name), std::move(fullName));
    }
    else {
        QNetworkProxy proxy = QNetworkProxy(QNetworkProxy::NoProxy);

        if ((m_ui->accountTypeComboBox->currentIndex() == 0) &&
            m_ui->useNetworkProxyCheckBox->isChecked())
        {
            ErrorString errorDescription;
            proxy = networkProxy(errorDescription);
            if (!errorDescription.isEmpty()) {
                proxy = QNetworkProxy(QNetworkProxy::NoProxy);
            }
        }

        QString server = evernoteServerUrl();
        Q_EMIT evernoteAccountAdditionRequested(
            std::move(server), std::move(proxy));
    }

    QDialog::accept();
}

void AddAccountDialog::setupNetworkProxySettingsFrame()
{
    // 1) Hide the network proxy frame
    m_ui->networkProxyFrame->hide();

    // 2) Setup use network proxy checkbox
    QObject::connect(
        m_ui->useNetworkProxyCheckBox, &QCheckBox::toggled, this,
        &AddAccountDialog::onUseNetworkProxyToggled);

    // 3) Setup the network proxy types combo box
    QStringList networkProxyTypes;
    networkProxyTypes.reserve(3);

    // Allow only "No proxy", "Http proxy" and "Socks5 proxy"
    // If proxy type parsed from the settings is different, fall back to
    // "No proxy". Also treat QNetworkProxy::DefaultProxy as "No proxy" because
    // the default proxy type of QNetworkProxy is QNetworkProxy::NoProxy

    QString noProxyItem = tr("No proxy");
    QString httpProxyItem = tr("Http proxy");
    QString socks5ProxyItem = tr("Socks5 proxy");

    networkProxyTypes << noProxyItem;
    networkProxyTypes << httpProxyItem;
    networkProxyTypes << socks5ProxyItem;

    m_ui->networkProxyTypeComboBox->setModel(
        new QStringListModel(networkProxyTypes, this));

    QObject::connect(
        m_ui->networkProxyTypeComboBox,
        qOverload<int>(&QComboBox::currentIndexChanged), this,
        &AddAccountDialog::onNetworkProxyTypeChanged);

    // 4) Setup the valid range for port spin box
    m_ui->networkProxyPortSpinBox->setMinimum(0);
    m_ui->networkProxyPortSpinBox->setMaximum(
        std::max(std::numeric_limits<quint16>::max() - 1, 0));

    // 5) Connect to other editors of network proxy pieces

    QObject::connect(
        m_ui->networkProxyHostLineEdit, &QLineEdit::editingFinished, this,
        &AddAccountDialog::onNetworkProxyHostChanged);

    QObject::connect(
        m_ui->networkProxyPortSpinBox, qOverload<int>(&QSpinBox::valueChanged),
        this, &AddAccountDialog::onNetworkProxyPortChanged);

    QObject::connect(
        m_ui->networkProxyPasswordShowCheckBox, &QCheckBox::toggled, this,
        &AddAccountDialog::onNetworkProxyShowPasswordToggled);

    // NOTE: don't need to watch for edits of network proxy username and
    // password since can't judge their influence on the validity of network
    // proxy anyway
}

void AddAccountDialog::showLocalAccountAlreadyExistsMessage()
{
    ErrorString message{
        QT_TR_NOOP("Account with such username already exists")};
    QNDEBUG("account::AddAccountDialog", message);
    m_ui->statusText->setText(message.localizedString());
    m_ui->statusText->setHidden(false);
}

void AddAccountDialog::evaluateNetworkProxySettingsValidity()
{
    QNDEBUG(
        "account::AddAccountDialog",
        "AddAccountDialog::evaluateNetworkProxySettingsValidity");

    auto * okButton = m_ui->buttonBox->button(QDialogButtonBox::Ok);

    ErrorString errorDescription;
    if ((m_ui->accountTypeComboBox->currentIndex() == 0) &&
        m_ui->useNetworkProxyCheckBox->isChecked())
    {
        Q_UNUSED(networkProxy(errorDescription))
    }

    if (!errorDescription.isEmpty()) {
        m_ui->statusText->setText(errorDescription.localizedString());
        m_ui->statusText->show();

        if (okButton) {
            okButton->setEnabled(false);
        }
    }
    else {
        m_ui->statusText->clear();
        m_ui->statusText->hide();

        if (okButton) {
            okButton->setEnabled(true);
        }
    }
}

QNetworkProxy AddAccountDialog::networkProxy(
    ErrorString & errorDescription) const
{
    QNDEBUG("account::AddAccountDialog", "AddAccountDialog::networkProxy");

    errorDescription.clear();

    QNetworkProxy proxy;

    const int proxyTypeInt = m_ui->networkProxyTypeComboBox->currentIndex();
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
        QNTRACE("account::AddAccountDialog", "No proxy");
        return proxy;
    }
    }

    const QString host = m_ui->networkProxyHostLineEdit->text();
    const QUrl proxyHostUrl{host};
    if (!proxyHostUrl.isValid()) {
        errorDescription.setBase(
            QT_TR_NOOP("Network proxy host url is not valid"));

        errorDescription.setDetails(host);
        QNDEBUG("account::AddAccountDialog", errorDescription);
        return QNetworkProxy{QNetworkProxy::NoProxy};
    }

    proxy.setHostName(host);

    const int proxyPort = m_ui->networkProxyPortSpinBox->value();
    if (Q_UNLIKELY(
            proxyPort < 0 || proxyPort >= std::numeric_limits<quint16>::max()))
    {
        errorDescription.setBase(QT_TR_NOOP("Network proxy port is not valid"));
        errorDescription.setDetails(QString::number(proxyPort));
        QNDEBUG("account::AddAccountDialog", errorDescription);
        return QNetworkProxy(QNetworkProxy::NoProxy);
    }

    proxy.setPort(static_cast<quint16>(proxyPort));
    proxy.setUser(m_ui->networkProxyUserLineEdit->text());
    proxy.setPassword(m_ui->networkProxyPasswordLineEdit->text());

    QNTRACE(
        "account::AddAccountDialog",
        "Network proxy: type = "
            << proxy.type() << ", host = " << proxy.hostName()
            << ", port = " << proxy.port() << ", user = " << proxy.user());

    return proxy;
}

} // namespace quentier
