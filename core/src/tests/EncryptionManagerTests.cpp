#include "EncryptionManagerTests.h"
#include <tools/EncryptionManager.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

bool decryptTest(QString & error)
{
    EncryptionManager manager;

    const QString encryptedText = "RU5DMEukzbpnzHzrqquWEjGiTdkV53zn9AmJzbDXFB99A"
                                  "eAbVWd2kyzK3NSQdtBYHqEN2xU50dfeiumMgSl892UFPA"
                                  "ycM9Ow3E4eMQWXYVhyCQ5w5HFCZnQAhaS20X1n0ti+XiI"
                                  "aWRH+mDKPa0xMRSpuod2EYeWHwmxY3aRwW4ZbR1XU0TtN"
                                  "bQHR7s7hT3FalyLMrCJpmTJcHtzwNVuN5HfizUoXoQ2LDY"
                                  "SMrI1n7WutzVWL";

    const QString passphrase = "rough_awakening^";

    QString decryptedText;
    bool res = manager.decrypt(encryptedText, passphrase, "AES", 128, decryptedText, error);
    if (!res) {
        QNWARNING(error);
        return false;
    }

    if (decryptedText != "Very-very secret") {
        error = QT_TR_NOOP("Decrypted text differs from the original (\"Very-very secret\": ") + decryptedText;
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

    QString encryptedText;
    QString cipher;
    size_t keyLength = 0;
    bool res = manager.encrypt(textToEncrypt, passphrase, cipher, keyLength, encryptedText, error);
    if (!res) {
        QNWARNING(error);
        return false;
    }

    QString decryptedText;
    res = manager.decrypt(encryptedText, passphrase, cipher, keyLength, decryptedText, error);
    if (!res) {
        QNWARNING(error);
        return false;
    }

    if (decryptedText != "Very-very secret") {
        error = QT_TR_NOOP("Decrypted text differs from the original (\"Very-very secret\": ") + decryptedText;
        QNWARNING(error);
        return false;
    }

    return true;

}

} // namespace test
} // namespace qute_note
