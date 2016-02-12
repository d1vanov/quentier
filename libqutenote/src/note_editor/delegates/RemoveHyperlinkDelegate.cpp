#include "RemoveHyperlinkDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't remove hyperlink: can't get note editor's page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

RemoveHyperlinkDelegate::RemoveHyperlinkDelegate(NoteEditorPrivate & noteEditor) :
    QObject(&noteEditor),
    m_noteEditor(noteEditor)
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
        findIdOfHyperlinkUnderCursor();
    }
}

void RemoveHyperlinkDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("RemoveHyperlinkDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    QObject::disconnect(&m_noteEditor, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                        this, QNSLOT(RemoveHyperlinkDelegate,onOriginalPageConvertedToNote,Note));

    findIdOfHyperlinkUnderCursor();
}

void RemoveHyperlinkDelegate::findIdOfHyperlinkUnderCursor()
{
    QNDEBUG("RemoveHyperlinkDelegate::findIdOfHyperlinkUnderCursor");

    QString javascript = "hyperlinkManager.findSelectedHyperlinkId();";
    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &RemoveHyperlinkDelegate::onHyperlinkIdFound));
}

void RemoveHyperlinkDelegate::onHyperlinkIdFound(const QVariant & data)
{
    QNDEBUG("RemoveHyperlinkDelegate::onHyperlinkIdFound: " << data);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of hyperlink data request from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of hyperlink data request from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't get hyperlink data from JavaScript: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    auto dataIt = resultMap.find("data");
    if (Q_UNLIKELY(dataIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: no hyperlink data received from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString dataStr = dataIt.value().toString();

    bool conversionResult = false;
    quint64 hyperlinkId = dataStr.toULongLong(&conversionResult);
    if (!conversionResult) {
        QString error = QT_TR_NOOP("Can't remove hyperlink under cursor: can't convert hyperlink id to a number");
        QNWARNING(error << ", data from JS: " << data);
        emit notifyError(error);
        return;
    }

    removeHyperlink(hyperlinkId);
}

void RemoveHyperlinkDelegate::removeHyperlink(const quint64 hyperlinkId)
{
    QNDEBUG("RemoveHyperlinkDelegate::removeHyperlink");

    QString javascript = "hyperlinkManager.removeHyperlink(" + QString::number(hyperlinkId) + ", false);";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &RemoveHyperlinkDelegate::onHyperlinkRemoved));
}

void RemoveHyperlinkDelegate::onHyperlinkRemoved(const QVariant & data)
{
    QNDEBUG("RemoveHyperlinkDelegate::onHyperlinkRemoved: " << data);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of hyperlink removal from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of hyperlink removal from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't remove hyperlink from JavaScript: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    emit finished();
}

} // namespace qute_note
