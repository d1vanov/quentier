#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__DECRYPT_ENCRYPTED_TEXT_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__DECRYPT_ENCRYPTED_TEXT_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include "HtmlCallbackFunctor.hpp"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <QObject>
#include <QUuid>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(NoteEditorPage)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)
QT_FORWARD_DECLARE_CLASS(EncryptionManager)
QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

/**
 * @brief The DecryptEncryptedTextDelegate class encapsulates a chain of callbacks
 * required for proper implementation of decryption for encrypted text
 * considering the details of wrapping this action around undo stack and necessary switching
 * of note editor page during the process
 */
class DecryptEncryptedTextDelegate: public QObject
{
    Q_OBJECT
public:
    explicit DecryptEncryptedTextDelegate(QString encryptedText, QString cipher,
                                          QString length, QString hint,
                                          NoteEditorPrivate & noteEditor,
                                          FileIOThreadWorker * pFileIOThreadWorker,
                                          QSharedPointer<EncryptionManager> encryptionManager,
                                          DecryptedTextManager & decryptedTextManager);

    void start();

Q_SIGNALS:
    void finished(QString htmlWithDecryptedText, int pageXOffset, int pageYOffset, QString encryptedText,
                  QString cipher, size_t length, QString hint, QString decryptedText, QString passphrase);
    void cancelled();
    void notifyError(QString error);

// private signals
    void writeFile(QString absoluteFilePath, QByteArray data, QUuid requestId);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onPageScrollReceived(const QVariant & data);

    void onNewPageLoaded(bool ok);
    void onNewPageJavaScriptLoaded();

    void onEncryptedTextDecrypted(QString cipher, size_t keyLength,
                                  QString encryptedText, QString passphrase,
                                  QString decryptedText, bool rememberForSession,
                                  bool decryptPermanently, bool createDecryptUndoCommand);
    void onPermanentDecryptionScriptFinished(const QVariant & data);
    void onModifiedPageHtmlReceived(const QString & html);

    void onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);

    void onModifiedPageLoaded(bool ok);
    void onModifiedPageJavaScriptLoaded();

private:
    void raiseDecryptionDialog();
    void requestPageScroll();
    void continueDecryptionProcessing();
    void requestPageHtml();

private:
    typedef JsResultCallbackFunctor<DecryptEncryptedTextDelegate> JsCallback;
    typedef HtmlCallbackFunctor<DecryptEncryptedTextDelegate> HtmlCallback;

private:
    QString     m_encryptedText;
    QString     m_cipher;
    size_t      m_length;
    QString     m_hint;
    QString     m_decryptedText;
    QString     m_passphrase;
    bool        m_decryptPermanently;

    NoteEditorPrivate &                 m_noteEditor;
    FileIOThreadWorker *                m_pFileIOThreadWorker;
    QSharedPointer<EncryptionManager>   m_encryptionManager;
    DecryptedTextManager &              m_decryptedTextManager;

    QString     m_modifiedHtml;
    QUuid       m_writeModifiedHtmlToPageSourceRequestId;

    int         m_pageXOffset;
    int         m_pageYOffset;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__DECRYPT_ENCRYPTED_TEXT_DELEGATE_H
