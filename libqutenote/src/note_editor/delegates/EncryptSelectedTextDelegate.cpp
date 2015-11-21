#include "EncryptSelectedTextDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace qute_note {

EncryptSelectedTextDelegate::EncryptSelectedTextDelegate(NoteEditorPrivate & noteEditor,
                                                         NoteEditorPage * pOriginalPage,
                                                         FileIOThreadWorker * pFileIOThreadWorker) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor),
    m_pOriginalPage(pOriginalPage),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
    m_originalHtml(),
    m_originalPageFilePath(noteEditor.noteEditorPagePath()),
    m_writeOriginalHtmlToPageSourceRequestId()
{}

void EncryptSelectedTextDelegate::start()
{
    QNDEBUG("EncryptSelectedTextDelegate::start");

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        requestOriginalPageHtml();
    }
}

void EncryptSelectedTextDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("EncryptSelectedTextDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageConvertedToNote,Note));

    requestOriginalPageHtml();
}

void EncryptSelectedTextDelegate::onOriginalPageHtmlReceived(const QString & html)
{
    QNDEBUG("EncryptSelectedTextDelegate::onOriginalPageHtmlReceived");

    m_originalHtml = html;

    QObject::connect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageModified));

    bool cancelled = false;
    m_noteEditor.doEncryptSelectedTextDialog(&cancelled);
    if (cancelled) {
        QObject::disconnect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                            this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageModified));
        emit finished();
    }
}

void EncryptSelectedTextDelegate::onOriginalPageModified()
{
    QNDEBUG("EncryptSelectedTextDelegate::onOriginalPageModified");

    QObject::disconnect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageModified));

    QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                     this, QNSLOT(EncryptSelectedTextDelegate,onModifiedNoteReceived,Note));

#ifdef USE_QT_WEB_ENGINE
    m_pOriginalPage->toHtml(HtmlCallbackFunctor(*this, &EncryptSelectedTextDelegate::onModifiedPageHtmlReceived));
#else
    QString html = m_pOriginalPage->mainFrame()->toHtml();
    onModifiedPageHtmlReceived(html);
#endif

    m_noteEditor.convertToNote();
}

void EncryptSelectedTextDelegate::onModifiedPageHtmlReceived(const QString & html)
{
    QNDEBUG("EncryptSelectedTextDelegate::onModifiedPageHtmlReceived");

    emit receivedHtmlWithEncryption(html);
}

void EncryptSelectedTextDelegate::onModifiedNoteReceived(Note note)
{
    QNDEBUG("EncryptSelectedTextDelegate::onModifiedNoteReceived");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(EncryptSelectedTextDelegate,onModifiedNoteReceived));

    m_noteEditor.switchEditorPage();

    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(EncryptSelectedTextDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::connect(this, QNSIGNAL(EncryptSelectedTextDelegate,writeFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    m_writeOriginalHtmlToPageSourceRequestId = QUuid::createUuid();
    emit writeFile(m_originalPageFilePath, m_originalHtml.toLocal8Bit(), m_writeOriginalHtmlToPageSourceRequestId);
}

void EncryptSelectedTextDelegate::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    if (requestId != m_writeOriginalHtmlToPageSourceRequestId) {
        return;
    }

    QNDEBUG("EncryptSelectedTextDelegate::onWriteFileRequestProcessed: success = "
            << (success ? "true" : "false") << ", error = " << errorDescription
            << ", request id =" << requestId);

    QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                        this, QNSLOT(EncryptSelectedTextDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::disconnect(this, QNSIGNAL(EncryptSelectedTextDelegate,writeFile,QString,QByteArray,QUuid),
                        m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    if (!success) {
        errorDescription = QT_TR_NOOP("Can't finalize the encrypted text processing, "
                                      "can't write the original HTML to the previous "
                                      "note editor page for undo stack: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QUrl url = QUrl::fromLocalFile(m_originalPageFilePath);

#ifdef USE_QT_WEB_ENGINE
    m_pOriginalPage->setUrl(url);
    m_pOriginalPage->load(url);
#else
    m_pOriginalPage->mainFrame()->setUrl(url);
    m_pOriginalPage->mainFrame()->load(url);
#endif

    emit finished();
}

void EncryptSelectedTextDelegate::requestOriginalPageHtml()
{
    QNDEBUG("EncryptSelectedTextDelegate::requestOriginalPageHtml");

#ifdef USE_QT_WEB_ENGINE
    m_pOriginalPage->toHtml(HtmlCallbackFunctor(*this, &EncryptSelectedTextDelegate::onOriginalPageHtmlReceived));
#else
    QString html = m_pOriginalPage->mainFrame()->toHtml();
    onOriginalPageHtmlReceived(html);
#endif
}

} // namespace qute_note
