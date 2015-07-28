#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_DECRYPTION_DIALOG_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_DECRYPTION_DIALOG_H

#include "EncryptedAreaPlugin.h"
#include <qute_note/utility/EncryptionManager.h>
#include <QDialog>
#include <QSharedPointer>

namespace Ui {
QT_FORWARD_DECLARE_CLASS(NoteDecryptionDialog)
}

namespace qute_note {

class NoteDecryptionDialog: public QDialog
{
    Q_OBJECT
public:
    explicit NoteDecryptionDialog(const QString & encryptedText,
                                  const QString & cipher,
                                  const QString & hint,
                                  const size_t keyLength,
                                  QSharedPointer<EncryptionManager> encryptionManager,
                                  DecryptedTextCachePtr decryptedTextCache,
                                  QWidget * parent = nullptr);
    virtual ~NoteDecryptionDialog();

    QString passphrase() const;
    bool rememberPassphrase() const;

    QString decryptedText() const;

private Q_SLOTS:
    void setHint(const QString & hint);
    void setRememberPassphraseDefaultState(const bool checked);
    void onRememberPassphraseStateChanged();

    virtual void accept();

private:
    void setError(const QString & error);

private:
    Ui::NoteDecryptionDialog *          m_pUI;
    QString                             m_encryptedText;
    QString                             m_cipher;
    QString                             m_hint;

    QString                             m_cachedDecryptedText;

    QSharedPointer<EncryptionManager>   m_encryptionManager;
    DecryptedTextCachePtr               m_decryptedTextCache;

    size_t                              m_keyLength;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_DECRYPTION_DIALOG_H
