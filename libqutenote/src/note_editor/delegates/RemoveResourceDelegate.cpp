#include "RemoveResourceDelegate.h"
#include "ParsePageScrollData.h"
#include "../NoteEditor_p.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't remove attachment: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

RemoveResourceDelegate::RemoveResourceDelegate(const ResourceWrapper & resourceToRemove, NoteEditorPrivate & noteEditor,
                                               FileIOThreadWorker * pFileIOThreadWorker) :
    m_noteEditor(noteEditor),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
    m_resource(resourceToRemove),
    m_modifiedHtml(),
    m_writeModifiedHtmlToPageSourceRequestId(),
    m_pageXOffset(-1),
    m_pageYOffset(-1)
{}

void RemoveResourceDelegate::start()
{
    QNDEBUG("RemoveResourceDelegate::start");

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(RemoveResourceDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        requestPageScroll();
    }
}

void RemoveResourceDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("RemoveResourceDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(RemoveResourceDelegate,onOriginalPageConvertedToNote,Note));

    requestPageScroll();
}

void RemoveResourceDelegate::requestPageScroll()
{
    QNDEBUG("RemoveResourceDelegate::requestPageScroll");

    GET_PAGE()
    page->executeJavaScript("getCurrentScroll();", JsResultCallbackFunctor(*this, &RemoveResourceDelegate::onPageScrollReceived));
}

void RemoveResourceDelegate::onPageScrollReceived(const QVariant & data)
{
    QNDEBUG("RemoveResourceDelegate::onPageScrollReceived: " << data);

    QString errorDescription;
    bool res = parsePageScrollData(data, m_pageXOffset, m_pageYOffset, errorDescription);
    if (Q_UNLIKELY(!res)) {
        errorDescription = QT_TR_NOOP("Can't remove resource: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    doStart();
}

void RemoveResourceDelegate::doStart()
{
    QNDEBUG("RemoveResourceDelegate::doStart");

    if (Q_UNLIKELY(!m_resource.hasDataHash())) {
        QString error = QT_TR_NOOP("Can't remove resource: the resource to be removed doesn't contain the data hash");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_noteEditor.switchEditorPage(/* should convert to note = */ false);

    GET_PAGE()
    QObject::connect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool),
                     this, QNSLOT(RemoveResourceDelegate,onSwitchedPageLoaded,bool));

    m_noteEditor.updateFromNote();
}

void RemoveResourceDelegate::onSwitchedPageLoaded(bool ok)
{
    QNDEBUG("RemoveResourceDelegate::onSwitchedPageLoaded: ok = " << (ok ? "true" : "false"));

    Q_UNUSED(ok)

    GET_PAGE()
    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool),
                        this, QNSLOT(RemoveResourceDelegate,onSwitchedPageLoaded,bool));

    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(RemoveResourceDelegate,onSwitchedPageJavaScriptLoaded));
}

void RemoveResourceDelegate::onSwitchedPageJavaScriptLoaded()
{
    QNDEBUG("RemoveResourceDelegate::onSwitchedPageJavaScriptLoaded");

    GET_PAGE()
    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(RemoveResourceDelegate,onSwitchedPageJavaScriptLoaded));

    m_noteEditor.skipNextContentChange();

    QString javascript = "removeResource('" + m_resource.dataHash() + "');";

    page->executeJavaScript(javascript, JsResultCallbackFunctor(*this, &RemoveResourceDelegate::onResourceReferenceRemovedFromNoteContent));
}

void RemoveResourceDelegate::onResourceReferenceRemovedFromNoteContent(const QVariant & data)
{
    QNDEBUG("RemoveResourceDelegate::onResourceReferenceRemovedFromNoteContent");

    Q_UNUSED(data)

    GET_PAGE()
#ifdef USE_QT_WEB_ENGINE
    page->toHtml(HtmlCallbackFunctor(*this, &RemoveResourceDelegate::onPageHtmlWithoutResourceReceived));
#else
    QString html = page->mainFrame()->toHtml();
    onPageHtmlWithoutResourceReceived(html);
#endif
}

void RemoveResourceDelegate::onPageHtmlWithoutResourceReceived(const QString & html)
{
    QNDEBUG("RemoveResourceDelegate::onPageHtmlWithoutResourceReceived");

    m_modifiedHtml = html;

    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(RemoveResourceDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::connect(this, QNSIGNAL(RemoveResourceDelegate,writeFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    m_writeModifiedHtmlToPageSourceRequestId = QUuid::createUuid();
    emit writeFile(m_noteEditor.noteEditorPagePath(), m_modifiedHtml.toLocal8Bit(),
                   m_writeModifiedHtmlToPageSourceRequestId);
}

void RemoveResourceDelegate::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    if (requestId != m_writeModifiedHtmlToPageSourceRequestId) {
        return;
    }

    QNDEBUG("RemoveResourceDelegate::onWriteFileRequestProcessed: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId);

    QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                        this, QNSLOT(RemoveResourceDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::disconnect(this, QNSIGNAL(RemoveResourceDelegate,writeFile,QString,QByteArray,QUuid),
                        m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    if (Q_UNLIKELY(!success)) {
        errorDescription = QT_TR_NOOP("Can't finalize the removal of attachment processing, "
                                      "can't write the modified HTML to the note editor: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QUrl url = QUrl::fromLocalFile(m_noteEditor.noteEditorPagePath());

    GET_PAGE()
    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(RemoveResourceDelegate,onModifiedPageLoaded));

    m_noteEditor.setPageOffsetsForNextLoad(m_pageXOffset, m_pageYOffset);

#ifdef USE_QT_WEB_ENGINE
    page->setUrl(url);
    page->load(url);
#else
    page->mainFrame()->setUrl(url);
    page->mainFrame()->load(url);
#endif
}

void RemoveResourceDelegate::onModifiedPageLoaded()
{
    QNDEBUG("RemoveResourceDelegate::onModifiedPageLoaded");

    GET_PAGE()
    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(RemoveResourceDelegate,onModifiedPageLoaded));

    m_noteEditor.removeResourceFromNote(m_resource);

    emit finished(m_resource, m_modifiedHtml, m_pageXOffset, m_pageYOffset);
}

} // namespace qute_note
