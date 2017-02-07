/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_NOTE_EDITOR_ENCRYPTION_DIALOG_H
#define LIB_QUENTIER_NOTE_EDITOR_ENCRYPTION_DIALOG_H

#include <quentier/utility/EncryptionManager.h>
#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <QDialog>
#include <QSharedPointer>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EncryptionDialog)
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class EncryptionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit EncryptionDialog(const QString & textToEncrypt,
                              QSharedPointer<EncryptionManager> encryptionManager,
                              QSharedPointer<DecryptedTextManager> decryptedTextManager,
                              QWidget * parent = Q_NULLPTR);
    virtual ~EncryptionDialog();

    QString passphrase() const;
    bool rememberPassphrase() const;

    QString encryptedText() const;
    QString hint() const;

Q_SIGNALS:
    void accepted(QString textToEncrypt, QString encryptedText,
                  QString cipher, size_t keyLength,
                  QString hint, bool rememberForSession);

private Q_SLOTS:
    void setRememberPassphraseDefaultState(const bool checked);
    void onRememberPassphraseStateChanged(int checked);

    virtual void accept() Q_DECL_OVERRIDE;

private:
    void setError(const ErrorString & error);

private:
    Ui::EncryptionDialog *                  m_pUI;
    QString                                 m_textToEncrypt;
    QString                                 m_cachedEncryptedText;
    QSharedPointer<EncryptionManager>       m_encryptionManager;
    QSharedPointer<DecryptedTextManager>    m_decryptedTextManager;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_ENCRYPTION_DIALOG_H
