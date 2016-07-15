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

#ifndef LIB_QUENTIER_NOTE_EDITOR_DECRYPTED_TEXT_MANAGER_H
#define LIB_QUENTIER_NOTE_EDITOR_DECRYPTED_TEXT_MANAGER_H

#include <quentier/utility/Linkage.h>
#include <QtGlobal>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManagerPrivate)

class QUENTIER_EXPORT DecryptedTextManager
{
public:
    DecryptedTextManager();
    virtual ~DecryptedTextManager();

    void addEntry(const QString & hash, const QString & decryptedText,
                  const bool rememberForSession, const QString & passphrase,
                  const QString & cipher, const size_t keyLength);

    void removeEntry(const QString & hash);

    void clearNonRememberedForSessionEntries();

    bool findDecryptedTextByEncryptedText(const QString & encryptedText, QString & decryptedText,
                                          bool & rememberForSession) const;

    bool modifyDecryptedText(const QString & originalEncryptedText, const QString & newDecryptedText,
                             QString & newEncryptedText);

private:
    Q_DISABLE_COPY(DecryptedTextManager)

    DecryptedTextManagerPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(DecryptedTextManager)
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DECRYPTED_TEXT_MANAGER_H
