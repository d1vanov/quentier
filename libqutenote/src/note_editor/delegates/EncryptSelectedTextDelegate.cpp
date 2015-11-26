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
    m_modifiedHtml(),
    m_writeModifiedHtmlToPageSourceRequestId()
{}

void EncryptSelectedTextDelegate::start()
{
    QNDEBUG("EncryptSelectedTextDelegate::start");

    if (m_noteEditor.isModified())
    {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        encryptSelectedText();
    }
}

void EncryptSelectedTextDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("EncryptSelectedTextDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageConvertedToNote,Note));

    encryptSelectedText();
}

void EncryptSelectedTextDelegate::encryptSelectedText()
{
    QNDEBUG("EncryptSelectedTextDelegate::encryptSelectedText");

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

#ifdef USE_QT_WEB_ENGINE
    m_pOriginalPage->toHtml(HtmlCallbackFunctor(*this, &EncryptSelectedTextDelegate::onModifiedPageHtmlReceived));
#else
    QString html = m_pOriginalPage->mainFrame()->toHtml();
    onModifiedPageHtmlReceived(html);
#endif
}

void EncryptSelectedTextDelegate::onModifiedPageHtmlReceived(const QString & html)
{
    QNDEBUG("EncryptSelectedTextDelegate::onModifiedPageHtmlReceived");

    // Now the tricky part begins: we need to undo the change
    // for the original page and then create the new page
    // and set this modified HTML there

    m_modifiedHtml = html;
    m_noteEditor.setNotePageHtmlAfterEncryption(m_modifiedHtml);

    QObject::connect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageModificationUndone));

    m_noteEditor.undoLastEncryption();
}

void EncryptSelectedTextDelegate::onOriginalPageModificationUndone()
{
    QNDEBUG("EncryptSelectedTextDelegate::onOriginalPageModificationUndone");

    QObject::disconnect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(EncryptSelectedTextDelegate,onOriginalPageModificationUndone));

    m_noteEditor.switchEditorPage(/* should convert from note = */ false);

    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(EncryptSelectedTextDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::connect(this, QNSIGNAL(EncryptSelectedTextDelegate,writeFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    m_writeModifiedHtmlToPageSourceRequestId = QUuid::createUuid();
    emit writeFile(m_noteEditor.noteEditorPagePath(), m_modifiedHtml.toLocal8Bit(),
                   m_writeModifiedHtmlToPageSourceRequestId);
}

void EncryptSelectedTextDelegate::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    if (requestId != m_writeModifiedHtmlToPageSourceRequestId) {
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
                                      "can't write the modified HTML to the note editor : ")
                                      + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QUrl url = QUrl::fromLocalFile(m_noteEditor.noteEditorPagePath());

    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page());

    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(EncryptSelectedTextDelegate,onModifiedPageLoaded));

#ifdef USE_QT_WEB_ENGINE
    page->setUrl(url);
    page->load(url);
#else
    page->mainFrame()->setUrl(url);
    page->mainFrame()->load(url);
#endif
}

void EncryptSelectedTextDelegate::onModifiedPageLoaded()
{
    QNDEBUG("EncryptSelectedTextDelegate::onModifiedPageLoaded");

    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page());

    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(EncryptSelectedTextDelegate,onModifiedPageLoaded));
    emit finished();
}

} // namespace qute_note
