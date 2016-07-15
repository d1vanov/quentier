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

#ifndef LIB_QUENTIER_NOTE_EDITOR_DECRYPTED_TEXT_MANAGER_P_H
#define LIB_QUENTIER_NOTE_EDITOR_DECRYPTED_TEXT_MANAGER_P_H

#include <quentier/utility/EncryptionManager.h>
#include <QtGlobal>
#include <QHash>

namespace quentier {

class DecryptedTextManagerPrivate
{
public:
    DecryptedTextManagerPrivate();

    void addEntry(const QString & hash, const QString & decryptedText,
                  const bool rememberForSession, const QString & passphrase,
                  const QString & cipher, const size_t keyLength);

    void removeEntry(const QString & hash);

    void clearNonRememberedForSessionEntries();

    bool findDecryptedTextByEncryptedText(const QString & encryptedText, QString & decryptedText,
                                          bool & rememberForSession) const;

    bool modifyDecryptedText(const QString & originalHash, const QString & newDecryptedText,
                             QString & newEncryptedText);

private:
    Q_DISABLE_COPY(DecryptedTextManagerPrivate)

private:
    class Data
    {
    public:
        Data() :
            m_decryptedText(),
            m_passphrase(),
            m_cipher(),
            m_keyLength(0),
            m_rememberForSession(false)
        {}

        QString m_decryptedText;
        QString m_passphrase;
        QString m_cipher;
        size_t  m_keyLength;
        bool    m_rememberForSession;
    };

    typedef QHash<QString, Data> DataHash;
    DataHash    m_dataHash;

    DataHash    m_staleDataHash;

    EncryptionManager   m_encryptionManager;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DECRYPTED_TEXT_MANAGER_P_H
