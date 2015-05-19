#include "EncryptionManagerTests.h"
#include <tools/EncryptionManager.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

bool decryptTest(QString & error)
{
    EncryptionManager manager;

    const QString encryptedText = "RU5DMA4hFx2T/Uy8tI7R7aGAe8r/ncNiNxGykP7VbZQMMP"
                                  "6fcPzyQqGegjVvYQg4MtKBr57qb6vBLEpWHIKVY9XLQzeOX"
                                  "kqpy/kJtLRKTP21BsZrIeSX67XuM40z02M4XAZsIF7Ixxsg"
                                  "8m81SWYQ1oliEnac+FiWzI5dUwKpSdFVc7WB40/f2cK5Hpn"
                                  "PjtRcjcm6aT7sCXAXgslU6Qwrv43X1bmqMlZT0pxpKRFpKrnk"
                                  "PV89TvGfvGUyQGfc50B3VzqzJvF2R4pEe3mGzKramIGKei1w1"
                                  "aFiT2eufwDgfJghkDWgIIW7QbZUiywoMwITrwpYJh1NCbOb"
                                  "RGMqBLAB6kZFRVKbS5JWesKwgRmnHsdHQTpX4Sd/dYI3GC4"
                                  "FpdeK9C+E3YcBSVcCNBGt7N2+ce3o7q/K3Ynoq+8w0qHtNi1"
                                  "XGV2hD+LU6IK13FoRJwyT1hhkAMiFc3mgvT83hrynhVhByhk"
                                  "KGPixL3s1GFy5s51NmLSTur/b8pnicF+TgG4YXz8Jxvnalmf"
                                  "2SMd5voxuz9Ny5EK86P54MsuzjK0y6AO/te7pkxht6UP+SYIt"
                                  "JI2s1r5fl07Sa/xlkVH28xcL3ODpr0DTWEh/Qq2pXRNLfpcISd"
                                  "BJIMaIPlcJGR8567WJK0A07yhsS9c1PnVUb+cLlI9PIG3Qxtkfp";

    const QString passphrase = "thisismyriflethisismygunthisisforfortunethisisforfun";

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
