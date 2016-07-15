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

#include "EncryptionManagerTests.h"
#include <quentier/utility/EncryptionManager.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/QNLocalizedString.h>

namespace quentier {
namespace test {

bool decryptAesTest(QString & error)
{
    EncryptionManager manager;

    const QString encryptedText = "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                  "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                  "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                  "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                  "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                  "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                  "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=";

    const QString passphrase = "thisismyriflethisismygunthisisforfortunethisisforfun";

    const QString originalText = "<span style=\"display: inline !important; float: none; \">Ok, here's some really long text. "
                                 "I can type and type it on and on and it will not stop any time soon just yet. The password "
                                 "is going to be long also.&nbsp;</span>";

    QString decryptedText;
    QNLocalizedString errorMessage;
    bool res = manager.decrypt(encryptedText, passphrase, "AES", 128, decryptedText, errorMessage);
    if (!res) {
        error = errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    if (decryptedText != originalText) {
        error = "Decrypted text differs from the original; original text = " + originalText + "; decrypted text = " + decryptedText;
        QNWARNING(error);
        return false;
    }

    return true;
}

bool encryptDecryptTest(QString & error)
{
    EncryptionManager manager;

    const QString textToEncrypt = "Very-very secret";
    const QString passphrase = "rough_awakening^";

    QNLocalizedString errorMessage;
    QString encryptedText;
    QString cipher;
    size_t keyLength = 0;
    bool res = manager.encrypt(textToEncrypt, passphrase, cipher, keyLength, encryptedText, errorMessage);
    if (!res) {
        error = errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    errorMessage.clear();
    QString decryptedText;
    res = manager.decrypt(encryptedText, passphrase, cipher, keyLength, decryptedText, errorMessage);
    if (!res) {
        error = errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    if (decryptedText != "Very-very secret") {
        error = "Decrypted text differs from the original (\"Very-very secret\": " + decryptedText;
        QNWARNING(error);
        return false;
    }

    return true;

}

bool decryptRc2Test(QString & error)
{
    EncryptionManager manager;

    const QString encryptedText = "K+sUXSxI2Mt075+pSDxR/gnCNIEnk5XH1P/D0Eie17"
                                  "JIWgGnNo5QeMo3L0OeBORARGvVtBlmJx6vJY2Ij/2En"
                                  "MVy6/aifSdZXAxRlfnTLvI1IpVgHpTMzEfy6zBVMo+V"
                                  "Bt2KglA+7L0iSjA0hs3GEHI6ZgzhGfGj";

    const QString passphrase = "my_own_encryption_key_1988";

    const QString originalText = "<span style=\"display: inline !important; float: none; \">"
                                 "Ok, here's a piece of text I'm going to encrypt now</span>";

    QNLocalizedString errorMessage;
    QString decryptedText;
    bool res = manager.decrypt(encryptedText, passphrase, "RC2", 64, decryptedText, errorMessage);
    if (!res) {
        error = errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    if (decryptedText != originalText) {
        error = "Decrypted text differs from the original; original text = " + originalText +
                "; decrypted text = " + decryptedText;
        QNWARNING(error);
        return false;
    }

    return true;
}

} // namespace test
} // namespace quentier
