#ifndef LIB_QUENTIER_NOTE_EDITOR_DECRYPTION_DIALOG_H
#define LIB_QUENTIER_NOTE_EDITOR_DECRYPTION_DIALOG_H

#include <quentier/utility/EncryptionManager.h>
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
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
    void setError(const QNLocalizedString & error);

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
