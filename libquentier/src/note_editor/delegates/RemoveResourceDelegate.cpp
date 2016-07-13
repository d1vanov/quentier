#include "RemoveResourceDelegate.h"
#include "../NoteEditor_p.h"
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(m_noteEditor.page()); \
    if (Q_UNLIKELY(!page)) { \
        QNLocalizedString error = QT_TR_NOOP("can't remove the attachment: no note editor page"); \
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
        QNLocalizedString error = QT_TR_NOOP("can't remove the attachment: the data hash is missing");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString javascript = "resourceManager.removeResource('" + m_resource.dataHash().toHex() + "');";

    GET_PAGE()
    page->executeJavaScript(javascript, JsCallback(*this, &RemoveResourceDelegate::onResourceReferenceRemovedFromNoteContent));
}

void RemoveResourceDelegate::onResourceReferenceRemovedFromNoteContent(const QVariant & data)
{
    QNDEBUG("RemoveResourceDelegate::onResourceReferenceRemovedFromNoteContent");

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QNLocalizedString error = QT_TR_NOOP("can't parse the result of attachment reference removal from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QNLocalizedString error;

        auto errorIt = resultMap.find("error");
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error = QT_TR_NOOP("can't parse the error of attachment reference removal from JavaScript");
        }
        else {
            error = QT_TR_NOOP("can't remove the attachment reference from the note editor");
            error += ": ";
            error += errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_noteEditor.removeResourceFromNote(m_resource);

    emit finished(m_resource);
}

} // namespace quentier
