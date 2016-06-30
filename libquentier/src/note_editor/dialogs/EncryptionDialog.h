#ifndef LIB_QUENTIER_NOTE_EDITOR_ENCRYPTION_DIALOG_H
#define LIB_QUENTIER_NOTE_EDITOR_ENCRYPTION_DIALOG_H

#include <quentier/utility/EncryptionManager.h>
#include <quentier/utility/Qt4Helper.h>
#include <QDialog>
#include <QSharedPointer>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EncryptionDialog)
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class EncryptionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit EncryptionDialog(const QString & textToEncrypt,
                              QSharedPointer<EncryptionManager> encryptionManager,
                              QSharedPointer<DecryptedTextManager> decryptedTextManager,
                              QWidget * parent = Q_NULLPTR);
    virtual ~EncryptionDialog();

    QString passphrase() const;
    bool rememberPassphrase() const;

    QString encryptedText() const;
    QString hint() const;

Q_SIGNALS:
    void accepted(QString textToEncrypt, QString encryptedText,
                  QString cipher, size_t keyLength,
                  QString hint, bool rememberForSession);

private Q_SLOTS:
    void setRememberPassphraseDefaultState(const bool checked);
    void onRememberPassphraseStateChanged(int checked);

    virtual void accept() Q_DECL_OVERRIDE;

private:
    void setError(const QString & error);

private:
    Ui::EncryptionDialog *                  m_pUI;
    QString                                 m_textToEncrypt;
    QString                                 m_cachedEncryptedText;
    QSharedPointer<EncryptionManager>       m_encryptionManager;
    QSharedPointer<DecryptedTextManager>    m_decryptedTextManager;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_ENCRYPTION_DIALOG_H
