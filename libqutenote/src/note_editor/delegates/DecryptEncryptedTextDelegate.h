#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__DECRYPT_ENCRYPTED_TEXT_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__DECRYPT_ENCRYPTED_TEXT_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <QPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(EncryptionManager)
QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

/**
 * @brief The DecryptEncryptedTextDelegate class encapsulates a chain of callbacks
 * required for proper implementation of decryption for encrypted text
 * considering the details of wrapping this action around the undo stack
 */
class DecryptEncryptedTextDelegate: public QObject
{
    Q_OBJECT
public:
    explicit DecryptEncryptedTextDelegate(const QString & encryptedTextId, const QString & encryptedText,
                                          const QString & cipher, const QString & length, const QString & hint,
                                          NoteEditorPrivate * pNoteEditor,
                                          QSharedPointer<EncryptionManager> encryptionManager,
                                          QSharedPointer<DecryptedTextManager> decryptedTextManager);

    void start();

Q_SIGNALS:
    void finished(QString encryptedText, QString cipher, size_t length, QString hint,
                  QString decryptedText, QString passphrase, bool rememberForSession,
                  bool decryptPermanently);
    void cancelled();
    void notifyError(QString error);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onEncryptedTextDecrypted(QString cipher, size_t keyLength, QString encryptedText, QString passphrase,
                                  QString decryptedText, bool rememberForSession, bool decryptPermanently);
    void onDecryptionScriptFinished(const QVariant & data);

private:
    void raiseDecryptionDialog();

private:
    typedef JsResultCallbackFunctor<DecryptEncryptedTextDelegate> JsCallback;

private:
    QString     m_encryptedTextId;
    QString     m_encryptedText;
    QString     m_cipher;
    size_t      m_length;
    QString     m_hint;
    QString     m_decryptedText;
    QString     m_passphrase;
    bool        m_rememberForSession;
    bool        m_decryptPermanently;

    QPointer<NoteEditorPrivate>             m_pNoteEditor;
    QSharedPointer<EncryptionManager>       m_encryptionManager;
    QSharedPointer<DecryptedTextManager>    m_decryptedTextManager;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__DECRYPT_ENCRYPTED_TEXT_DELEGATE_H
