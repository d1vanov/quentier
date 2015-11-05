#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTION_DIALOG_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTION_DIALOG_H

#include <qute_note/utility/EncryptionManager.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QDialog>
#include <QSharedPointer>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(DecryptionDialog)
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class DecryptionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit DecryptionDialog(const QString & encryptedText,
                              const QString & cipher,
                              const QString & hint,
                              const size_t keyLength,
                              QSharedPointer<EncryptionManager> encryptionManager,
                              DecryptedTextManager & decryptedTextManager,
                              QWidget * parent = Q_NULLPTR,
                              bool decryptPermanentlyFlag = false);
    virtual ~DecryptionDialog();

    QString passphrase() const;
    bool rememberPassphrase() const;

    QString decryptedText() const;

Q_SIGNALS:
    void accepted(QString encryptedText, QString decryptedText,
                  bool rememberPassphrase, bool decryptPermanently);

private Q_SLOTS:
    void setHint(const QString & hint);
    void setRememberPassphraseDefaultState(const bool checked);
    void onRememberPassphraseStateChanged(int checked);
    void onShowPasswordStateChanged(int checked);

    void onDecryptPermanentlyStateChanged(int checked);

    virtual void accept() Q_DECL_OVERRIDE;

private:
    void setError(const QString & error);

private:
    Ui::DecryptionDialog *              m_pUI;
    QString                             m_encryptedText;
    QString                             m_cipher;
    QString                             m_hint;

    QString                             m_cachedDecryptedText;

    QSharedPointer<EncryptionManager>   m_encryptionManager;
    DecryptedTextManager &              m_decryptedTextManager;

    size_t                              m_keyLength;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DECRYPTION_DIALOG_H
