#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTION_DIALOG_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTION_DIALOG_H

#include <qute_note/utility/EncryptionManager.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QDialog>
#include <QSharedPointer>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(EncryptionDialog)
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

class EncryptionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit EncryptionDialog(const QString & textToEncrypt,
                              QSharedPointer<EncryptionManager> encryptionManager,
                              DecryptedTextManager & decryptedTextManager,
                              QWidget * parent = Q_NULLPTR);
    virtual ~EncryptionDialog();

    QString passphrase() const;
    bool rememberPassphrase() const;

    QString encryptedText() const;
    QString hint() const;

Q_SIGNALS:
    void accepted(QString textToEncrypt, QString encryptedText,
                  QString passphrase, QString cipher, size_t keyLength,
                  QString hint, bool rememberForSession);

private Q_SLOTS:
    void setRememberPassphraseDefaultState(const bool checked);
    void onRememberPassphraseStateChanged(int checked);

    virtual void accept() Q_DECL_OVERRIDE;

private:
    void setError(const QString & error);

private:
    Ui::EncryptionDialog *              m_pUI;
    QString                             m_textToEncrypt;
    QString                             m_cachedEncryptedText;
    QSharedPointer<EncryptionManager>   m_encryptionManager;
    DecryptedTextManager &              m_decryptedTextManager;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__ENCRYPTION_DIALOG_H
