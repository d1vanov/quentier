#include "RemoveResourceDelegate.h"
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
    m_writeModifiedHtmlToPageSourceRequestId()
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
        doStart();
    }
}

void RemoveResourceDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("RemoveResourceDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(RemoveResourceDelegate,onOriginalPageConvertedToNote,Note));

    doStart();
}

void RemoveResourceDelegate::doStart()
{
    QNDEBUG("RemoveResourceDelegate::doStart");

    if (Q_UNLIKELY(!m_resource.hasDataHash())) {
        QString error = QT_TR_NOOP("Can't remove the resource: resource to be removed doesn't contain the data hash");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_noteEditor.skipNextContentChange();

    QString javascript = "removeResource('" + m_resource.dataHash() + "');";

    GET_PAGE()
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

    // Now the tricky part begins: we need to undo the change
    // for the original page and then create the new page
    // and set this modified HTML there

    m_modifiedHtml = html;

    // Now we need to undo the attachment removal we just did for the old page

    m_noteEditor.skipNextContentChange();
    m_noteEditor.undoPageAction();

    // Now can switch the page to the new one and set the modified HTML there
    m_noteEditor.switchEditorPage(/* should convert from note = */ false);

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

    emit finished(m_resource, m_modifiedHtml);
}

} // namespace qute_note
