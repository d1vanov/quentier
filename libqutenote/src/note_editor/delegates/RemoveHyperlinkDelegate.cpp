#include "RemoveHyperlinkDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

#ifndef USE_QT_WEB_ENGINE
#include <QWebFrame>
#endif

namespace qute_note {

RemoveHyperlinkDelegate::RemoveHyperlinkDelegate(NoteEditorPrivate & noteEditor, NoteEditorPage * pOriginalPage) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor),
    m_pOriginalPage(pOriginalPage),
    m_hyperlinkId(0)
{}

void RemoveHyperlinkDelegate::start()
{
    QNDEBUG("RemoveHyperlinkDelegate::start");

    if (m_hyperlinkId != 0) {
        removeHyperlink();
        return;
    }

    if (m_noteEditor.isModified()) {
        QObject::connect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                         this, QNSLOT(RemoveHyperlinkDelegate,onOriginalPageConvertedToNote,Note));
        m_noteEditor.convertToNote();
    }
    else {
        findIdOfHyperlinkUnderCursor();
    }
}

void RemoveHyperlinkDelegate::setHyperlinkId(const quint64 hyperlinkId)
{
    QNDEBUG("RemoveHyperlinkDelegate::setHyperlinkId: " << hyperlinkId);
    m_hyperlinkId = hyperlinkId;
}

void RemoveHyperlinkDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("RemoveHyperlinkDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(RemoveHyperlinkDelegate,onOriginalPageConvertedToNote,Note));

    findIdOfHyperlinkUnderCursor();
}

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't remove hyperlink: can't get note editor's page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

void RemoveHyperlinkDelegate::findIdOfHyperlinkUnderCursor()
{
    QNDEBUG("RemoveHyperlinkDelegate::findIdOfHyperlinkUnderCursor");

    QString javascript = "findSelectedHyperlinkId();";
    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &RemoveHyperlinkDelegate::onHyperlinkIdFound));
}

void RemoveHyperlinkDelegate::onHyperlinkIdFound(const QVariant & data)
{
    QNDEBUG("RemoveHyperlinkDelegate::onHyperlinkIdFound: " << data);

    QString dataStr = data.toString();

    bool conversionResult = false;
    quint64 hyperlinkId = dataStr.toULongLong(&conversionResult);
    if (!conversionResult) {
        QString error = QT_TR_NOOP("Can't remove hyperlink under cursor: can't convert hyperlink id to a number");
        QNWARNING(error << ", data from JS: " << data);
        emit notifyError(error);
        return;
    }

    m_hyperlinkId = hyperlinkId;
    removeHyperlink();
}

void RemoveHyperlinkDelegate::removeHyperlink()
{
    QNDEBUG("RemoveHyperlinkDelegate::removeHyperlink");

    m_noteEditor.switchEditorPage(/* should convert from note = */ false);
    GET_PAGE()

    QObject::connect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool),
                     this, QNSLOT(RemoveHyperlinkDelegate,onNewPageLoadFinished,bool));

    m_noteEditor.updateFromNote();
}

void RemoveHyperlinkDelegate::onNewPageLoadFinished(bool ok)
{
    QNDEBUG("RemoveHyperlinkDelegate::onNewPageLoadFinished: ok = " << (ok ? "true" : "false"));
    Q_UNUSED(ok)

    GET_PAGE()

    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool),
                        this, QNSLOT(RemoveHyperlinkDelegate,onNewPageLoadFinished,bool));

    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(RemoveHyperlinkDelegate,onNewPageJavaScriptLoaded));
}

void RemoveHyperlinkDelegate::onNewPageJavaScriptLoaded()
{
    QNDEBUG("RemoveHyperlinkDelegate::onNewPageJavaScriptLoaded");

    GET_PAGE()

    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(RemoveHyperlinkDelegate,onNewPageJavaScriptLoaded));

    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                     this, QNSLOT(RemoveHyperlinkDelegate,onNewPageModified));

    m_noteEditor.skipNextContentChange();

    QString javascript = "removeHyperlink(" + QString::number(m_hyperlinkId) + ");";
    page->executeJavaScript(javascript);
}

void RemoveHyperlinkDelegate::onNewPageModified()
{
    QNDEBUG("RemoveHyperlinkDelegate::onNewPageModified");

    GET_PAGE()

    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded),
                        this, QNSLOT(RemoveHyperlinkDelegate,onNewPageModified));

    emit finished(m_hyperlinkId);
}

} // namespace qute_note
