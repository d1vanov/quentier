/*
 * Copyright 2018-2024 Dmitry Ivanov
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

#include "DeleteAccountDialog.h"

#include "AccountModel.h"
#include "ui_DeleteAccountDialog.h"

#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>
#include <quentier/utility/FileSystem.h>
#include <quentier/utility/StandardPaths.h>

#include <QDir>
#include <QFileInfo>
#include <QPushButton>
#include <QTextStream>

namespace quentier {

DeleteAccountDialog::DeleteAccountDialog(
    Account account, AccountModel & model, QWidget * parent) :
    QDialog{parent}, m_account{std::move(account)},
    m_ui{new Ui::DeleteAccountDialog}, m_model{model}
{
    m_ui->setupUi(this);
    setWindowTitle(tr("Delete account"));

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_ui->statusBarLabel->hide();

    QString warning;
    QTextStream strm{&warning};
    strm << "<html><head/><body><p>"
            "<span style=\"font-size:12pt; "
            "font-weight:600; color:#ff0000;\">";

    strm << tr(
        "WARNING! The account deletion is permanent and cannot be "
        "reverted!");

    strm << "</span></p><p>";

    if (m_account.type() == Account::Type::Evernote) {
        strm << tr(
            "The account to be deleted is Evernote one; only "
            "Quentier's locally synchronized account data would be "
            "deleted, your Evernote account itself won't be touched");
        strm << "</p><p>";
    }

    strm << "<span style=\"font-weight:600;\">";
    strm << tr("Enter");
    strm << " \"Yes\" ";

    strm << tr(
        "to the below form to confirm your intention to delete the "
        "account data");

    strm << ".</span></p><p><span style=\" font-weight:600;\">";

    strm << tr("Account details");
    strm << ": </span></p><p>";

    strm << tr("type");
    strm << ": ";

    if (m_account.type() == Account::Type::Evernote) {
        strm << tr("Evernote");
    }
    else {
        strm << tr("local");
    }

    strm << "</p><p>";
    strm << tr("name");
    strm << ": ";
    strm << m_account.name();

    if (m_account.type() == Account::Type::Evernote) {
        QString evernoteAccountType;

        switch (m_account.evernoteAccountType()) {
        case Account::EvernoteAccountType::Free:
            evernoteAccountType = tr("Basic");
            break;
        case Account::EvernoteAccountType::Plus:
            evernoteAccountType = tr("Plus");
            break;
        case Account::EvernoteAccountType::Premium:
            evernoteAccountType = tr("Premium");
            break;
        case Account::EvernoteAccountType::Business:
            evernoteAccountType = tr("Business");
            break;
        }

        if (!evernoteAccountType.isEmpty()) {
            strm << "</p><p>";
            strm << tr("Evernote account type");
            strm << ": ";
            strm << evernoteAccountType;
        }

        strm << "</p><p>";
        strm << tr("Evernote host");
        strm << ": ";
        strm << m_account.evernoteHost();
    }

    strm << "</p></body></html>";
    strm.flush();

    m_ui->warningLabel->setText(warning);

    QObject::connect(
        m_ui->confirmationLineEdit, &QLineEdit::textEdited, this,
        &DeleteAccountDialog::onConfirmationLineEditTextEdited);
}

DeleteAccountDialog::~DeleteAccountDialog()
{
    delete m_ui;
}

void DeleteAccountDialog::onConfirmationLineEditTextEdited(const QString & text)
{
    QNDEBUG(
        "account",
        "DeleteAccountDialog::onConfirmationLineEditTextEdited: " << text);

    bool confirmed = (text.toLower() == QStringLiteral("yes"));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(confirmed);
}

void DeleteAccountDialog::accept()
{
    QNINFO("account", "DeleteAccountDialog::accept: account = " << m_account);

    m_ui->statusBarLabel->setText(QString());
    m_ui->statusBarLabel->hide();

    if (Q_UNLIKELY(!m_model.removeAccount(m_account))) {
        ErrorString error(
            QT_TR_NOOP("Internal error: failed to remove "
                       "the account from account model"));

        QNWARNING("account", error);
        setStatusBarText(error.localizedString());
        return;
    }

    const QString path = accountPersistentStoragePath(m_account);
    if (Q_UNLIKELY(!removeDir(path))) {
        // Double check
        const QFileInfo pathInfo{path};
        if (pathInfo.exists()) {
            QNWARNING(
                "account",
                "Failed to remove account's persistence "
                    << "storage: " << QDir::toNativeSeparators(path));
        }
    }

    QDialog::accept();
}

void DeleteAccountDialog::setStatusBarText(const QString & text)
{
    m_ui->statusBarLabel->setText(text);

    if (text.isEmpty()) {
        m_ui->statusBarLabel->hide();
    }
    else {
        m_ui->statusBarLabel->show();
    }
}

} // namespace quentier
