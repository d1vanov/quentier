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

#include "DeleteAccountDialog.h"
#include "ui_DeleteAccountDialog.h"
#include <quentier/utility/StandardPaths.h>
#include <quentier/utility/Utility.h>
#include <quentier/logging/QuentierLogger.h>
#include <QPushButton>
#include <QDir>
#include <QFileInfo>

namespace quentier {

DeleteAccountDialog::DeleteAccountDialog(const Account & account,
                                         QWidget *parent) :
    QDialog(parent),
    m_pUi(new Ui::DeleteAccountDialog),
    m_account(account)
{
    m_pUi->setupUi(this);
    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QString warning = QStringLiteral("<html><head/><body><p><span style=\"font-size:12pt; font-weight:600; color:#ff0000;\">");
    warning += tr("WARNING! The account deletion is permanent and cannot be reverted!");
    warning += QStringLiteral("</span></p><p><br/></p><p><span style=\" font-weight:600; color:#000000;\">");

    if (m_account.type() == Account::Type::Evernote) {
        warning += QStringLiteral("</p><p><br/></p>");
        warning += tr("The account to be deleted is Evernote one; only Quentier's locally synchronized account "
                      "data would be deleted, your Evernote account itself won't be touched");
        warning += QStringLiteral("<p><br/></p>");
    }

    warning += tr("Enter");
    warning += QStringLiteral(" &quotYes&quot ");
    warning += tr("to the below form to confirm your intention to delete the account data");
    warning += QStringLiteral(".</span></p><p><br/></p><p><span style=\" font-weight:600; color:#000000;\">");
    warning += tr("Account details");
    warning += QStringLiteral(": </span></p><p>");

    warning += tr("type");
    warning += QStringLiteral(": ");

    if (m_account.type() == Account::Type::Evernote) {
        warning += tr("Evernote");
    }
    else {
        warning += tr("local");
    }

    warning += QStringLiteral("</p><p>");
    warning += tr("name");
    warning += QStringLiteral(": ");
    warning += m_account.name();

    if (m_account.type() == Account::Type::Evernote)
    {
        QString evernoteAccountType;

        switch(m_account.evernoteAccountType())
        {
        case Account::EvernoteAccountType::Free:
            evernoteAccountType = tr("Basic");
            break;
        case Account::EvernoteAccountType::Plus:
            evernoteAccountType = tr("Plus");
            break;
        case Account::EvernoteAccountType::Premium:
            evernoteAccountType  = tr("Premium");
            break;
        case Account::EvernoteAccountType::Business:
            evernoteAccountType = tr("Business");
            break;
        }

        if (!evernoteAccountType.isEmpty()) {
            warning += QStringLiteral("</p><p>");
            warning += tr("Evernote account type");
            warning += QStringLiteral(": ");
            warning += evernoteAccountType;
        }
    }

    warning += QStringLiteral("</p></body></html>");
    m_pUi->warningLabel->setText(warning);

    QObject::connect(m_pUi->confirmationLineEdit, SIGNAL(textEdited(QString)),
                     this, SLOT(onConfirmationLineEditTextEdited(QString)));
}

void DeleteAccountDialog::onConfirmationLineEditTextEdited(const QString & text)
{
    QNDEBUG(QStringLiteral("DeleteAccountDialog::onConfirmationLineEditTextEdited: ") << text);

    bool confirmed = (text == QStringLiteral("Yes"));
    m_pUi->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(confirmed);
}

void DeleteAccountDialog::accept()
{
    QString path = accountPersistentStoragePath(m_account);
    bool res = removeDir(path);
    if (Q_UNLIKELY(!res))
    {
        // Double check
        QFileInfo pathInfo(path);
        if (pathInfo.exists()) {
            QNWARNING(QStringLiteral("Failed to remove account's persistence storage: ")
                      << QDir::toNativeSeparators(path));
        }
    }

    QDialog::accept();
}

DeleteAccountDialog::~DeleteAccountDialog()
{
    delete m_pUi;
}

} // namespace quentier
