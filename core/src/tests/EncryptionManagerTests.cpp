#include "EncryptionManagerTests.h"
#include <tools/EncryptionManager.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
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
    bool res = manager.decrypt(encryptedText, passphrase, "AES", 128, decryptedText, error);
    if (!res) {
        QNWARNING(error);
        return false;
    }

    if (decryptedText != originalText) {
        error = QT_TR_NOOP("Decrypted text differs from the original; original text = ") + originalText + 
                QT_TR_NOOP("; decrypted text = ") + decryptedText;
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

bool decryptRc2Test(QString &error)
{
    EncryptionManager manager;

    const QString encryptedText = "K+sUXSxI2Mt075+pSDxR/gnCNIEnk5XH1P/D0Eie17"
                                  "JIWgGnNo5QeMo3L0OeBORARGvVtBlmJx6vJY2Ij/2En"
                                  "MVy6/aifSdZXAxRlfnTLvI1IpVgHpTMzEfy6zBVMo+V"
                                  "Bt2KglA+7L0iSjA0hs3GEHI6ZgzhGfGj";

    const QString passphrase = "my_own_encryption_key_1988";

    const QString originalText = "Ok, here's a piece of text I'm going to encrypt now";

    QString decryptedText;
    bool res = manager.decrypt(encryptedText, passphrase, "RC2", 64, decryptedText, error);
    if (!res) {
        QNWARNING(error);
        return false;
    }

    if (decryptedText != originalText) {
        error = QT_TR_NOOP("Decrypted text differs from the original; original text = ") + originalText +
                QT_TR_NOOP("; decrypted text = ") + decryptedText;
        QNWARNING(error);
        return false;
    }

    return true;
}

} // namespace test
} // namespace qute_note
