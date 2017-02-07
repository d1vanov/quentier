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

#ifndef LIB_QUENTIER_NOTE_EDITOR_DECRYPTION_DIALOG_H
#define LIB_QUENTIER_NOTE_EDITOR_DECRYPTION_DIALOG_H

#include <quentier/utility/EncryptionManager.h>
#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <QDialog>
#include <QSharedPointer>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(DecryptionDialog)
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class DecryptionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit DecryptionDialog(const QString & encryptedText, const QString & cipher, const QString & hint,
                              const size_t keyLength, QSharedPointer<EncryptionManager> encryptionManager,
                              QSharedPointer<DecryptedTextManager> decryptedTextManager,
                              QWidget * parent = Q_NULLPTR, bool decryptPermanentlyFlag = false);
    virtual ~DecryptionDialog();

    QString passphrase() const;
    bool rememberPassphrase() const;
    bool decryptPermanently() const;

    QString decryptedText() const;

Q_SIGNALS:
    void accepted(QString cipher, size_t keyLength, QString encryptedText,
                  QString passphrase, QString decryptedText,
                  bool rememberPassphrase, bool decryptPermanently);

private Q_SLOTS:
    void setHint(const QString & hint);
    void setRememberPassphraseDefaultState(const bool checked);
    void onRememberPassphraseStateChanged(int checked);
    void onShowPasswordStateChanged(int checked);

    void onDecryptPermanentlyStateChanged(int checked);

    virtual void accept() Q_DECL_OVERRIDE;

private:
    void setError(const ErrorString & error);

private:
    Ui::DecryptionDialog *                  m_pUI;
    QString                                 m_encryptedText;
    QString                                 m_cipher;
    QString                                 m_hint;

    QString                                 m_cachedDecryptedText;

    QSharedPointer<EncryptionManager>       m_encryptionManager;
    QSharedPointer<DecryptedTextManager>    m_decryptedTextManager;

    size_t                                  m_keyLength;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DECRYPTION_DIALOG_H
