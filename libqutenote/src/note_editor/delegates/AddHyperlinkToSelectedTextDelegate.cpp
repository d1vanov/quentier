#include "AddHyperlinkToSelectedTextDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace qute_note {

AddHyperlinkToSelectedTextDelegate::AddHyperlinkToSelectedTextDelegate(NoteEditorPrivate & noteEditor,
                                                                       NoteEditorPage * pOriginalPage,
                                                                       FileIOThreadWorker * pFileIOThreadWorker) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor),
    m_pOriginalPage(pOriginalPage),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
    m_modifiedHtml(),
    m_writeModifiedHtmlToPageSourceRequestId()
{}

void AddHyperlinkToSelectedTextDelegate::start()
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::start");

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        addHyperlinkToSelectedText();
    }
}

void AddHyperlinkToSelectedTextDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onOriginalPageConvertedToNote,Note));

    addHyperlinkToSelectedText();
}

void AddHyperlinkToSelectedTextDelegate::addHyperlinkToSelectedText()
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::addHyperlinkToSelectedText");

    QObject::connect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onOriginalPageModified));

    QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,editHyperlinkDialogCancelled),
                     this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onAddHyperlinkDialogCancelled));

    m_noteEditor.doAddHyperlinkToSelectedTextDialog();
}

void AddHyperlinkToSelectedTextDelegate::onAddHyperlinkDialogCancelled()
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onAddHyperlinkDialogCancelled");

    QObject::disconnect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onOriginalPageModified));

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,editHyperlinkDialogCancelled),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onAddHyperlinkDialogCancelled));

    emit finished();
}

void AddHyperlinkToSelectedTextDelegate::onOriginalPageModified()
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onOriginalPageModified");

    QObject::disconnect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onOriginalPageModified));

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,editHyperlinkDialogCancelled),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onAddHyperlinkDialogCancelled));

#ifdef USE_QT_WEB_ENGINE
    m_pOriginalPage->toHtml(HtmlCallbackFunctor(*this, &AddHyperlinkToSelectedTextDelegate::onModifiedPageHtmlReceived));
#else
    QString html = m_pOriginalPage->mainFrame()->toHtml();
    onModifiedPageHtmlReceived(html);
#endif
}

void AddHyperlinkToSelectedTextDelegate::onModifiedPageHtmlReceived(const QString & html)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onModifiedPageHtmlReceived");

    // Now the tricky part begins: we need to undo the change
    // for the original page and then create the new page
    // and set this modified HTML there

    m_modifiedHtml = html;
    m_noteEditor.setNotePageHtmlAfterAddingHyperlink(m_modifiedHtml);

    QObject::connect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onOriginalPageModificationUndone));

    m_noteEditor.undoLastHyperlinkAddition();
}

void AddHyperlinkToSelectedTextDelegate::onOriginalPageModificationUndone()
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onOriginalPageModificationUndone");

    QObject::disconnect(m_pOriginalPage, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onOriginalPageModificationUndone));

    m_noteEditor.switchEditorPage(/* should convert from note = */ false);

    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::connect(this, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,writeFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    m_writeModifiedHtmlToPageSourceRequestId = QUuid::createUuid();
    emit writeFile(m_noteEditor.noteEditorPagePath(), m_modifiedHtml.toLocal8Bit(),
                   m_writeModifiedHtmlToPageSourceRequestId);
}

void AddHyperlinkToSelectedTextDelegate::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    if (requestId != m_writeModifiedHtmlToPageSourceRequestId) {
        return;
    }

    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onWriteFileRequestProcessed: success = "
            << (success ? "true" : "false") << ", error description = " << errorDescription
            << ", request id = " << requestId);

    QObject::disconnect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onWriteFileRequestProcessed,bool,QString,QUuid));
    QObject::disconnect(this, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,writeFile,QString,QByteArray,QUuid),
                        m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));

    if (Q_UNLIKELY(!success)) {
        errorDescription = QT_TR_NOOP("Can't finalize the addition of hyperlink processing, "
                                      "can't write the modified HTML to the note editor: ") + errorDescription;
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QUrl url = QUrl::fromLocalFile(m_noteEditor.noteEditorPagePath());

    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page());
    if (Q_UNLIKELY(!page)) {
        errorDescription = QT_TR_NOOP("Can't finalize the addition of hyperlink processing: "
                                      "can't get the pointer to note editor page");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onModifiedPageLoaded));

#ifdef USE_QT_WEB_ENGINE
    page->setUrl(url);
    page->load(url);
#else
    page->mainFrame()->setUrl(url);
    page->mainFrame()->load(url);
#endif
}

void AddHyperlinkToSelectedTextDelegate::onModifiedPageLoaded()
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onModifiedPageLoaded");

    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page());
    if (Q_UNLIKELY(!page)) {
        QString errorDescription = QT_TR_NOOP("Can't finalize the addition of hyperlink processing: "
                                              "can't get the pointer to note editor page");
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onModifiedPageLoaded));
    emit finished();
}

} // namespace qute_note
