#include "DecryptEncryptedTextDelegate.h"
#include "ParsePageScrollData.h"
#include "../NoteEditor_p.h"
#include "../NoteEditorPage.h"
#include "../dialogs/DecryptionDialog.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/utility/EncryptionManager.h>
#include <qute_note/enml/ENMLConverter.h>
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/logging/QuteNoteLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't decrypt encrypted text: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

DecryptEncryptedTextDelegate::DecryptEncryptedTextDelegate(QString encryptedText, QString cipher,
                                                           QString length, QString hint,
                                                           NoteEditorPrivate & noteEditor,
                                                           FileIOThreadWorker * pFileIOThreadWorker,
                                                           QSharedPointer<EncryptionManager> encryptionManager,
                                                           DecryptedTextManager & decryptedTextManager) :
    m_encryptedText(encryptedText),
    m_cipher(cipher),
    m_length(0),
    m_hint(hint),
    m_decryptedText(),
    m_passphrase(),
    m_noteEditor(noteEditor),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
    m_encryptionManager(encryptionManager),
    m_decryptedTextManager(decryptedTextManager),
    m_modifiedHtml(),
    m_writeModifiedHtmlToPageSourceRequestId(),
    m_pageXOffset(-1),
    m_pageYOffset(-1)
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

    if (Q_UNLIKELY(!m_length)) {
        QString errorDescription = QT_TR_NOOP("Can't decrypt the encrypted text: can't convert encryption key length from string to number");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(DecryptEncryptedTextDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        requestPageScroll();
    }
}

void DecryptEncryptedTextDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(DecryptEncryptedTextDelegate,onOriginalPageConvertedToNote,Note));

    requestPageScroll();
}

void DecryptEncryptedTextDelegate::requestPageScroll()
{
    QNDEBUG("DecryptEncryptedTextDelegate::requestPageScroll");

    GET_PAGE()
    page->executeJavaScript("getCurrentScroll();", JsCallback(*this, &DecryptEncryptedTextDelegate::onPageScrollReceived));
}

void DecryptEncryptedTextDelegate::onPageScrollReceived(const QVariant & data)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onPageScrollReceived: " << data);

    QString errorDescription;
    bool res = parsePageScrollData(data, m_pageXOffset, m_pageYOffset, errorDescription);
    if (Q_UNLIKELY(!res)) {
        errorDescription = QT_TR_NOOP("Can't decrypt the encrypted text: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    m_noteEditor.switchEditorPage(/* should convert from note = */ false);

    GET_PAGE()
    QObject::connect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool), this, QNSLOT(DecryptEncryptedTextDelegate,onNewPageLoaded,bool));

    m_noteEditor.updateFromNote();
}

void DecryptEncryptedTextDelegate::onNewPageLoaded(bool ok)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onNewPageLoaded: ok = " << (ok ? "true" : "false"));

    if (!ok) {
        return;
    }

    GET_PAGE()
    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool), this, QNSLOT(DecryptEncryptedTextDelegate,onNewPageLoaded,bool));
    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded), this, QNSLOT(DecryptEncryptedTextDelegate,onNewPageJavaScriptLoaded));
}

void DecryptEncryptedTextDelegate::onNewPageJavaScriptLoaded()
{
    QNDEBUG("DecryptEncryptedTextDelegate::onNewPageJavaScriptLoaded");

    GET_PAGE()
    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded), this, QNSLOT(DecryptEncryptedTextDelegate,onNewPageJavaScriptLoaded));

    decryptEncryptedText();
}

void DecryptEncryptedTextDelegate::decryptEncryptedText()
{
    QNDEBUG("DecryptEncryptedTextDelegate::decryptEncryptedText");

    if (m_cipher.isEmpty()) {
        m_cipher = "AES";
    }

    QScopedPointer<DecryptionDialog> pDecryptionDialog(new DecryptionDialog(m_encryptedText,
                                                                            m_cipher, m_hint, m_length,
                                                                            m_encryptionManager,
                                                                            m_decryptedTextManager, &m_noteEditor));
    pDecryptionDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pDecryptionDialog.data(), QNSIGNAL(DecryptionDialog,accepted,QString,size_t,QString,QString,QString,bool,bool,bool),
                     this, QNSLOT(DecryptEncryptedTextDelegate,onEncryptedTextDecrypted,QString,size_t,QString,QString,QString,bool,bool,bool));
    QNTRACE("Will exec decryption dialog now");
    int res = pDecryptionDialog->exec();
    if (res == QDialog::Rejected) {
        emit cancelled();
        return;
    }
}

void DecryptEncryptedTextDelegate::onEncryptedTextDecrypted(QString cipher, size_t keyLength,
                                                            QString encryptedText, QString passphrase,
                                                            QString decryptedText, bool rememberForSession,
                                                            bool decryptPermanently, bool createDecryptUndoCommand)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onEncryptedTextDecrypted: encrypted text = " << encryptedText
            << ", remember for session = " << (rememberForSession ? "true" : "false")
            << ", decrypt permanently = " << (decryptPermanently ? "true" : "false"));

    GET_PAGE()

    m_decryptedText = decryptedText;
    m_passphrase = passphrase;

    Q_UNUSED(cipher)
    Q_UNUSED(keyLength)
    Q_UNUSED(createDecryptUndoCommand)

    if (decryptPermanently)
    {
        ENMLConverter::escapeString(decryptedText);

        QString javascript = "decryptEncryptedTextPermanently('" + encryptedText +
                             "', '" + decryptedText + "', 0);";
        QNTRACE("script: " << javascript);

        m_noteEditor.skipNextContentChange();
        page->executeJavaScript(javascript, JsCallback(*this, &DecryptEncryptedTextDelegate::onPermanentDecryptionScriptFinished));
    }
    else
    {
        QObject::connect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool), this, QNSLOT(DecryptEncryptedTextDelegate,onModifiedPageLoaded,bool));
        m_noteEditor.setPageOffsetsForNextLoad(m_pageXOffset, m_pageYOffset);
        m_noteEditor.updateFromNote();
    }
}

void DecryptEncryptedTextDelegate::onPermanentDecryptionScriptFinished(const QVariant & data)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onPermanentDecryptionScriptFinished");

    Q_UNUSED(data)

    requestPageHtml();
}

void DecryptEncryptedTextDelegate::requestPageHtml()
{
    QNDEBUG("DecryptEncryptedTextDelegate::requestPageHtml");

    GET_PAGE()

#ifdef USE_QT_WEB_ENGINE
    page->toHtml(HtmlCallback(*this, &DecryptEncryptedTextDelegate::onModifiedPageHtmlReceived));
#else
    QString html = page->mainFrame()->toHtml();
    onModifiedPageHtmlReceived(html);
#endif
}

void DecryptEncryptedTextDelegate::onModifiedPageHtmlReceived(const QString & html)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onModifiedPageHtmlReceived");

    m_modifiedHtml = html;

    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(DecryptEncryptedTextDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::connect(this, QNSIGNAL(DecryptEncryptedTextDelegate,writeFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    m_writeModifiedHtmlToPageSourceRequestId = QUuid::createUuid();
    emit writeFile(m_noteEditor.noteEditorPagePath(), m_modifiedHtml.toLocal8Bit(),
                   m_writeModifiedHtmlToPageSourceRequestId);
}

void DecryptEncryptedTextDelegate::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    if (requestId != m_writeModifiedHtmlToPageSourceRequestId) {
        return;
    }

    QNDEBUG("DecryptEncryptedTextDelegate::onWriteFileRequestProcessed: success = "
            << (success ? "true" : "false") << ", error = " << errorDescription
            << ", request id =" << requestId);

    QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                        this, QNSLOT(DecryptEncryptedTextDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::disconnect(this, QNSIGNAL(DecryptEncryptedTextDelegate,writeFile,QString,QByteArray,QUuid),
                        m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    if (Q_UNLIKELY(!success)) {
        errorDescription = QT_TR_NOOP("Can't finalize the decryption of encrypted text, "
                                      "can't write the modified HTML to the note editor: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QUrl url = QUrl::fromLocalFile(m_noteEditor.noteEditorPagePath());

    GET_PAGE()

    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(DecryptEncryptedTextDelegate,onModifiedPageJavaScriptLoaded));

    m_noteEditor.setPageOffsetsForNextLoad(m_pageXOffset, m_pageYOffset);

#ifdef USE_QT_WEB_ENGINE
    page->setUrl(url);
    page->load(url);
#else
    page->mainFrame()->setUrl(url);
    page->mainFrame()->load(url);
#endif
}

void DecryptEncryptedTextDelegate::onModifiedPageLoaded(bool ok)
{
    QNDEBUG("DecryptEncryptedTextDelegate::onModifiedPageLoaded: ok = " << (ok ? "true" : "false"));

    if (!ok) {
        return;
    }

    GET_PAGE()
    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool), this, QNSLOT(DecryptEncryptedTextDelegate,onModifiedPageLoaded,bool));
    QObject::connect(page, QNSLOT(NoteEditorPage,javaScriptLoaded), this, QNSLOT(DecryptEncryptedTextDelegate,onModifiedPageJavaScriptLoaded));
}

void DecryptEncryptedTextDelegate::onModifiedPageJavaScriptLoaded()
{
    QNDEBUG("DecryptEncryptedTextDelegate::onModifiedPageJavaScriptLoaded");

    GET_PAGE()

    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(DecryptEncryptedTextDelegate,onModifiedPageJavaScriptLoaded));

    emit finished(m_modifiedHtml, m_pageXOffset, m_pageYOffset, m_encryptedText, m_cipher,
                  m_length, m_hint, m_decryptedText, m_passphrase);
}

} // namespace qute_note
