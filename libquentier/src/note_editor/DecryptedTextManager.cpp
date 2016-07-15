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

#include <quentier/note_editor/DecryptedTextManager.h>
#include "DecryptedTextManager_p.h"

namespace quentier {

DecryptedTextManager::DecryptedTextManager() :
    d_ptr(new DecryptedTextManagerPrivate)
{}

DecryptedTextManager::~DecryptedTextManager()
{
    delete d_ptr;
}

void DecryptedTextManager::addEntry(const QString & hash, const QString & decryptedText,
                                    const bool rememberForSession, const QString & passphrase,
                                    const QString & cipher, const size_t keyLength)
{
    Q_D(DecryptedTextManager);
    d->addEntry(hash, decryptedText, rememberForSession, passphrase, cipher, keyLength);
}

void DecryptedTextManager::removeEntry(const QString & hash)
{
    Q_D(DecryptedTextManager);
    d->removeEntry(hash);
}

void DecryptedTextManager::clearNonRememberedForSessionEntries()
{
    Q_D(DecryptedTextManager);
    d->clearNonRememberedForSessionEntries();
}

bool DecryptedTextManager::findDecryptedTextByEncryptedText(const QString & encryptedText,
                                                            QString & decryptedText,
                                                            bool & rememberForSession) const
{
    Q_D(const DecryptedTextManager);
    return d->findDecryptedTextByEncryptedText(encryptedText, decryptedText, rememberForSession);
}

bool DecryptedTextManager::modifyDecryptedText(const QString & originalEncryptedText,
                                               const QString & newDecryptedText,
                                               QString & newEncryptedText)
{
    Q_D(DecryptedTextManager);
    return d->modifyDecryptedText(originalEncryptedText, newDecryptedText, newEncryptedText);
}

} // namespace quentier
