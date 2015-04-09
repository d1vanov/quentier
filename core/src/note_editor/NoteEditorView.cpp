#include "NoteEditorView.h"
#include <logging/QuteNoteLogger.h>
#include <QDropEvent>

NoteEditorView::NoteEditorView(QWidget * parent) :
    QWebView(parent)
{
    setAcceptDrops(true);
}

void NoteEditorView::dropEvent(QDropEvent * pEvent)
{
    QNDEBUG("NoteEditorView::dropEvent");

    if (!pEvent) {
        QNINFO("Null drop event was detected");
        return;
    }

    const QMimeData * pMimeData = pEvent->mimeData();
    if (!pMimeData) {
        QNINFO("Null mime data from drop event was detected");
        return;
    }

    // TODO: implement
}

