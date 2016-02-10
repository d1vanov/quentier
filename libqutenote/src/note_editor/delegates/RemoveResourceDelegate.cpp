#include "RemoveResourceDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QString error = QT_TR_NOOP("Can't remove attachment: can't get note editor page"); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

RemoveResourceDelegate::RemoveResourceDelegate(const ResourceWrapper & resourceToRemove, NoteEditorPrivate & noteEditor) :
    m_noteEditor(noteEditor),
    m_resource(resourceToRemove)
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
        QString error = QT_TR_NOOP("Can't remove resource: the resource to be removed doesn't contain the data hash");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString javascript = "resourceManager.removeResource('" + m_resource.dataHash() + "');";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &RemoveResourceDelegate::onResourceReferenceRemovedFromNoteContent));
}

void RemoveResourceDelegate::onResourceReferenceRemovedFromNoteContent(const QVariant & data)
{
    QNDEBUG("RemoveResourceDelegate::onResourceReferenceRemovedFromNoteContent");

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of resource reference removal from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of resource reference removal from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't remove resource reference from the note editor: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_noteEditor.removeResourceFromNote(m_resource);

    emit finished(m_resource);
}

} // namespace qute_note
