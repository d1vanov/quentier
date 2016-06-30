#include "DecryptEncryptedTextDelegate.h"
#include "ParsePageScrollData.h"
#include "../NoteEditor_p.h"
#include "../NoteEditorPage.h"
#include "../dialogs/DecryptionDialog.h"
#include <quentier/utility/FileIOThreadWorker.h>
#include <quentier/utility/EncryptionManager.h>
#include <quentier/enml/ENMLConverter.h>
#include <quentier/note_editor/DecryptedTextManager.h>
#include <quentier/logging/QuentierLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace quentier {

#define CHECK_NOTE_EDITOR() \
    if (Q_UNLIKELY(m_pNoteEditor.isNull())) { \
        QNDEBUG("Note editor is null"); \
        return; \
    }

#define GET_PAGE() \
    CHECK_NOTE_EDITOR() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_pNoteEditor->page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't decrypt encrypted text: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

DecryptEncryptedTextDelegate::DecryptEncryptedTextDelegate(const QString & encryptedTextId, const QString & encryptedText,
                                                           const QString & cipher, const QString & length, const QString & hint,
                                                           NoteEditorPrivate * pNoteEditor,
                                                           QSharedPointer<EncryptionManager> encryptionManager,
                                                           QSharedPointer<DecryptedTextManager> decryptedTextManager) :
    m_encryptedTextId(encryptedTextId),
    m_encryptedText(encryptedText),
    m_cipher(cipher),
    m_length(0),
    m_hint(hint),
    m_decryptedText(),
    m_passphrase(),
    m_rememberForSession(false),
    m_decryptPermanently(false),
    m_pNoteEditor(pNoteEditor),
    m_encryptionManager(encryptionManager),
    m_decryptedTextManager(decryptedTextManager)
{
    if (length.isEmpty())
    {
        m_length = 128;
    }
    else
    {
        bool conversionResult = false;
        m_length = static_cast<size_t>(length.toInt(&conversionResult));
        if (Q_UNLIKELY(!conversionResult)) {
            m_length = 0;   // NOTE: postponing the error report until the attempt to start the delegate
        }
    }
}

void DecryptEncryptedTextDelegate::start()
{
    QNDEBUG("DecryptEncryptedTextDelegate::start");

    CHECK_NOTE_EDITOR()

    if (Q_UNLIKELY(!m_length)) {
        QString errorDescription = QT_TR_NOOP("Can't decrypt the encrypted text: can't convert encryption key length from string to number");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    if (m_pNoteEditor->isModified()) {
        QObject::connect(m_pNoteEditor.data(), QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(DecryptEncryptedTextDelegate,onOriginalPageConvertedToNote,Note));
        m_pNoteEditor->convertToNote();
    }
    else {
        raiseDecryptionDialog();
    }
}

void DecryptEncryptedTextDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onOriginalPageConvertedToNote");

    CHECK_NOTE_EDITOR()

    Q_UNUSED(note)

    QObject::disconnect(m_pNoteEditor.data(), QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(DecryptEncryptedTextDelegate,onOriginalPageConvertedToNote,Note));

    raiseDecryptionDialog();
}

void DecryptEncryptedTextDelegate::raiseDecryptionDialog()
{
    QNDEBUG("DecryptEncryptedTextDelegate::raiseDecryptionDialog");

    if (m_cipher.isEmpty()) {
        m_cipher = "AES";
    }

    QScopedPointer<DecryptionDialog> pDecryptionDialog(new DecryptionDialog(m_encryptedText, m_cipher, m_hint, m_length,
                                                                            m_encryptionManager, m_decryptedTextManager, m_pNoteEditor));
    pDecryptionDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pDecryptionDialog.data(), QNSIGNAL(DecryptionDialog,accepted,QString,size_t,QString,QString,QString,bool,bool),
                     this, QNSLOT(DecryptEncryptedTextDelegate,onEncryptedTextDecrypted,QString,size_t,QString,QString,QString,bool,bool));
    QNTRACE("Will exec decryption dialog now");
    int res = pDecryptionDialog->exec();
    if (res == QDialog::Rejected) {
        emit cancelled();
        return;
    }
}

void DecryptEncryptedTextDelegate::onEncryptedTextDecrypted(QString cipher, size_t keyLength, QString encryptedText,
                                                            QString passphrase, QString decryptedText, bool rememberForSession,
                                                            bool decryptPermanently)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onEncryptedTextDecrypted: encrypted text = " << encryptedText
            << ", remember for session = " << (rememberForSession ? "true" : "false")
            << ", decrypt permanently = " << (decryptPermanently ? "true" : "false"));

    CHECK_NOTE_EDITOR()

    m_decryptedText = decryptedText;
    m_passphrase = passphrase;
    m_rememberForSession = rememberForSession;
    m_decryptPermanently = decryptPermanently;

    Q_UNUSED(cipher)
    Q_UNUSED(keyLength)

    QString decryptedTextHtml;
    if (!m_decryptPermanently) {
        decryptedTextHtml = ENMLConverter::decryptedTextHtml(m_decryptedText, m_encryptedText, m_hint,
                                                             m_cipher, m_length, m_pNoteEditor->GetFreeDecryptedTextId());
    }
    else {
        decryptedTextHtml = m_decryptedText;
    }

    ENMLConverter::escapeString(decryptedTextHtml);

    GET_PAGE()
    page->executeJavaScript("encryptDecryptManager.decryptEncryptedText('" + m_encryptedTextId + "', '" +
                            decryptedTextHtml + "');", JsCallback(*this, &DecryptEncryptedTextDelegate::onDecryptionScriptFinished));
}

void DecryptEncryptedTextDelegate::onDecryptionScriptFinished(const QVariant & data)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onDecryptionScriptFinished: " << data);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of text decryption script from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QString error;

        auto errorIt = resultMap.find("error");
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error = QT_TR_NOOP("Internal error: can't parse the error of text decryption from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't decrypt the encrypted text: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit finished(m_encryptedText, m_cipher, m_length, m_hint, m_decryptedText, m_passphrase, m_rememberForSession, m_decryptPermanently);
}

} // namespace quentier
