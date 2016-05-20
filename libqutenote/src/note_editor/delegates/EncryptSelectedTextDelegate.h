#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_ENCRYPT_SELECTED_TEXT_DELEGATE_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_ENCRYPT_SELECTED_TEXT_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <QPointer>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(EncryptionManager)
QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)

/**
 * @brief The EncryptSelectedTextDelegate class encapsulates a chain of callbacks
 * required for proper implementation of currently selected text encryption
 * considering the details of wrapping this action around the undo stack
 */
class EncryptSelectedTextDelegate: public QObject
{
    Q_OBJECT
public:
    explicit EncryptSelectedTextDelegate(NoteEditorPrivate * pNoteEditor, QSharedPointer<EncryptionManager> encryptionManager,
                                         QSharedPointer<DecryptedTextManager> decryptedTextManager);
    void start(const QString & selectionHtml);

Q_SIGNALS:
    void finished();
    void cancelled();
    void notifyError(QString error);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onSelectedTextEncrypted(QString selectedText, QString encryptedText, QString cipher,
                                 size_t keyLength, QString hint, bool rememberForSession);
    void onEncryptionScriptDone(const QVariant & data);

private:
    void raiseEncryptionDialog();
    void encryptSelectedText();

private:
    typedef JsResultCallbackFunctor<EncryptSelectedTextDelegate> JsCallback;

private:
    QPointer<NoteEditorPrivate>             m_pNoteEditor;
    QSharedPointer<EncryptionManager>       m_encryptionManager;
    QSharedPointer<DecryptedTextManager>    m_decryptedTextManager;

    QString                                 m_encryptedTextHtml;

    QString                                 m_selectionHtml;
    QString                                 m_encryptedText;
    QString                                 m_cipher;
    QString                                 m_keyLength;
    QString                                 m_hint;
    bool                                    m_rememberForSession;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_ENCRYPT_SELECTED_TEXT_DELEGATE_H
