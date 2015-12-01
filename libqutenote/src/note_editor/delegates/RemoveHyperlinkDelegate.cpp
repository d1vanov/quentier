#include "RemoveHyperlinkDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace qute_note {

RemoveHyperlinkDelegate::RemoveHyperlinkDelegate(NoteEditorPrivate & noteEditor,
                                                 NoteEditorPage * pOriginalPage,
                                                 FileIOThreadWorker * pFileIOThreadWorker) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor),
    m_pOriginalPage(pOriginalPage),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
    m_modifiedHtml(),
    m_writeModifiedHtmlToPageSourceRequestId()
{}

void RemoveHyperlinkDelegate::start()
{
    QNDEBUG("RemoveHyperlinkDelegate::start");

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(RemoveHyperlinkDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        removeHyperlink();
    }
}

void RemoveHyperlinkDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("RemoveHyperlinkDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(RemoveHyperlinkDelegate,onOriginalPageConvertedToNote,Note));

    removeHyperlink();
}

void RemoveHyperlinkDelegate::removeHyperlink()
{
    QNDEBUG("RemoveHyperlinkDelegate::removeHyperlink");

    QObject::connect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(RemoveHyperlinkDelegate,onOriginalPageModified));

    // TODO: m_noteEditor.doRemoveHyperlinkUnderCursor();
}

void RemoveHyperlinkDelegate::onOriginalPageModified()
{
    QNDEBUG("RemoveHyperlinkDelegate::onOriginalPageModified");

    QObject::disconnect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(RemoveHyperlinkDelegate,onOriginalPageModified));

#ifdef USE_QT_WEB_ENGINE
    m_pOriginalPage->toHtml(HtmlCallbackFunctor(*this, &RemoveHyperlinkDelegate::onModifiedPageHtmlReceived));
#else
    QString html = m_pOriginalPage->mainFrame()->toHtml();
    onModifiedPageHtmlReceived(html);
#endif
}

void RemoveHyperlinkDelegate::onModifiedPageHtmlReceived(const QString & html)
{
    QNDEBUG("RemoveHyperlinkDelegate::onModifiedPageHtmlReceived");

    // Now the tricky part begins: we need to undo the change
    // for the original page and then create the new page
    // and set this modified HTML there

    m_modifiedHtml = html;
    // TODO: m_noteEditor.setNotePageHtmlAfterRemovingHyperlink(m_modifiedHtml);

    QObject::connect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(RemoveHyperlinkDelegate,onOriginalPageModificationUndone));

    // TODO: m_noteEditor.undoLastHyperlinkRemoval
}

void RemoveHyperlinkDelegate::onOriginalPageModificationUndone()
{
    QNDEBUG("RemoveHyperlinkDelegate::onOriginalPageModificationUndone");

    QObject::disconnect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(RemoveHyperlinkDelegate,onOriginalPageModificationUndone));

    m_noteEditor.switchEditorPage(/* should convert from note = */ false);

    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(RemoveHyperlinkDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::connect(this, QNSIGNAL(RemoveHyperlinkDelegate,writeFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    m_writeModifiedHtmlToPageSourceRequestId = QUuid::createUuid();
    emit writeFile(m_noteEditor.noteEditorPagePath(), m_modifiedHtml.toLocal8Bit(),
                   m_writeModifiedHtmlToPageSourceRequestId);
}

void RemoveHyperlinkDelegate::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    if (requestId != m_writeModifiedHtmlToPageSourceRequestId) {
        return;
    }

    QNDEBUG("RemoveHyperlinkDelegate::onWriteFileRequestProcessed: success = "
            << (success ? "true" : "false") << ", error description = " << errorDescription
            << ", request id = " << requestId);

    QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                        this, QNSLOT(RemoveHyperlinkDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoveHyperlinkDelegate,writeFile,QString,QByteArray,QUuid),
                        m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    if (Q_UNLIKELY(!success)) {
        errorDescription = QT_TR_NOOP("Can't finalize the removal of hyperlink processing, "
                                      "can't write the modified HTML to the note editor: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QUrl url = QUrl::fromLocalFile(m_noteEditor.noteEditorPagePath());

    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page());
    if (Q_UNLIKELY(!page)) {
        errorDescription = QT_TR_NOOP("Can't finalize the removal of hyperlink processing: "
                                      "can't get the pointer to note editor page");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(RemoveHyperlinkDelegate,onModifiedPageLoaded));

#ifdef USE_QT_WEB_ENGINE
    page->setUrl(url);
    page->load(url);
#else
    page->mainFrame()->setUrl(url);
    page->mainFrame()->load(url);
#endif
}

void RemoveHyperlinkDelegate::onModifiedPageLoaded()
{
    QNDEBUG("RemoveHyperlinkDelegate::onModifiedPageLoaded");

    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page());
    if (Q_UNLIKELY(!page)) {
        QString errorDescription = QT_TR_NOOP("Can't finalize the removal of hyperlink processing: "
                                              "can't get the pointer to note editor page");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(RemoveHyperlinkDelegate,onModifiedPageLoaded));
    emit finished();
}

} // namespace qute_note
