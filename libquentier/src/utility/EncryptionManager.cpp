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

#include <quentier/utility/EncryptionManager.h>
#include "EncryptionManager_p.h"

namespace quentier {

EncryptionManager::EncryptionManager(QObject * parent) :
    QObject(parent),
    d_ptr(new EncryptionManagerPrivate)
{}

EncryptionManager::~EncryptionManager()
{}

bool EncryptionManager::decrypt(const QString & encryptedText, const QString & passphrase,
                                const QString & cipher, const size_t keyLength,
                                QString & decryptedText, QNLocalizedString & errorDescription)
{
    Q_D(EncryptionManager);
    return d->decrypt(encryptedText, passphrase, cipher, keyLength, decryptedText, errorDescription);
}

bool EncryptionManager::encrypt(const QString & textToEncrypt, const QString & passphrase,
                                QString & cipher, size_t & keyLength,
                                QString & encryptedText, QNLocalizedString & errorDescription)
{
    Q_D(EncryptionManager);
    return d->encrypt(textToEncrypt, passphrase, cipher, keyLength, encryptedText, errorDescription);
}

void EncryptionManager::onDecryptTextRequest(QString encryptedText, QString passphrase,
                                             QString cipher, size_t keyLength, QUuid requestId)
{
    QString decrypted;
    QNLocalizedString errorDescription;
    bool res = decrypt(encryptedText, passphrase, cipher, keyLength, decrypted, errorDescription);
    emit decryptedText(decrypted, res, errorDescription, requestId);
}

void EncryptionManager::onEncryptTextRequest(QString textToEncrypt, QString passphrase,
                                             QString cipher, size_t keyLength, QUuid requestId)
{
    QString encrypted;
    QNLocalizedString errorDescription;
    bool res = encrypt(textToEncrypt, passphrase, cipher, keyLength, encrypted, errorDescription);
    emit encryptedText(encrypted, res, errorDescription, requestId);
}

} // namespace quentier
