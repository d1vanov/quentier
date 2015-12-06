#include "AddHyperlinkToSelectedTextDelegate.h"
#include "../NoteEditor_p.h"
#include "../EditUrlDialog.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <QScopedPointer>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace qute_note {

AddHyperlinkToSelectedTextDelegate::AddHyperlinkToSelectedTextDelegate(NoteEditorPrivate & noteEditor,
                                                                       NoteEditorPage * pOriginalPage,
                                                                       FileIOThreadWorker * pFileIOThreadWorker,
                                                                       const quint64 hyperlinkIdToAdd) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor),
    m_pOriginalPage(pOriginalPage),
    m_pFileIOThreadWorker(pFileIOThreadWorker),
    m_hyperlinkId(hyperlinkIdToAdd),
    m_modifiedHtml(),
    m_writeModifiedHtmlToPageSourceRequestId()
{}

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't add hyperlink: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

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

    QString javascript = "getSelectionHtml(" + QString::number(m_hyperlinkId) + ");";
    GET_PAGE()
    page->executeJavaScript(javascript, JsResultCallbackFunctor(*this, &AddHyperlinkToSelectedTextDelegate::onInitialHyperlinkDataReceived));
}

void AddHyperlinkToSelectedTextDelegate::onInitialHyperlinkDataReceived(const QVariant & data)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onInitialHyperlinkDataReceived: " << data);
    raiseAddHyperlinkDialog(data.toString());
}

void AddHyperlinkToSelectedTextDelegate::raiseAddHyperlinkDialog(const QString & initialText)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::raiseAddHyperlinkDialog: initial text = " << initialText);

    QScopedPointer<EditUrlDialog> pEditUrlDialog(new EditUrlDialog(&m_noteEditor, initialText));
    pEditUrlDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pEditUrlDialog.data(), QNSIGNAL(EditUrlDialog,accepted,QString,QUrl,quint64,bool),
                     this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onAddHyperlinkDialogFinished,QString,QUrl,quint64,bool));
    QNTRACE("Will exec add hyperlink dialog now");
    int res = pEditUrlDialog->exec();
    if (res == QDialog::Rejected) {
        QNTRACE("Cancelled add hyperlink dialog");
        emit cancelled();
    }
}

void AddHyperlinkToSelectedTextDelegate::onAddHyperlinkDialogFinished(QString text, QUrl url, quint64 hyperlinkId, bool startupUrlWasEmpty)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onAddHyperlinkDialogFinished: text = " << text << ", url = " << url);

    Q_UNUSED(hyperlinkId);
    Q_UNUSED(startupUrlWasEmpty);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString urlString = url.toString(QUrl::FullyEncoded);
#else
    QString urlString = url.toString(QUrl::None);
#endif

    QString javascript = "setHyperlinkToSelection('" + text + "', '" + urlString +
                         "', " + QString::number(m_hyperlinkId) + ");";
    m_pOriginalPage->executeJavaScript(javascript, JsResultCallbackFunctor(*this, &AddHyperlinkToSelectedTextDelegate::onOriginalPageModified));
}

void AddHyperlinkToSelectedTextDelegate::onOriginalPageModified(const QVariant & data)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onOriginalPageModified");

    Q_UNUSED(data);

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

    // Now need to undo the hyperlink addition we just did for the old page

    m_noteEditor.skipPushingUndoCommandOnNextContentChange();

    QString javascript = "removeHyperlink(" + QString::number(m_hyperlinkId) + ");";
    GET_PAGE()
    page->executeJavaScript(javascript, JsResultCallbackFunctor(*this, &AddHyperlinkToSelectedTextDelegate::onOriginalPageModificationUndone));
}

void AddHyperlinkToSelectedTextDelegate::onOriginalPageModificationUndone(const QVariant & data)
{
    QNDEBUG("AddHyperlinkToSelectedTextDelegate::onOriginalPageModificationUndone");

    Q_UNUSED(data);

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

    GET_PAGE()
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

    GET_PAGE()
    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(AddHyperlinkToSelectedTextDelegate,onModifiedPageLoaded));
    emit finished();
}

} // namespace qute_note
