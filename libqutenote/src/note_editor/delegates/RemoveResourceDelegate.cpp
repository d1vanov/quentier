#include "RemoveResourceDelegate.h"
#include "../NoteEditor_p.h"
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

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

    // TODO: implement
}

void RemoveResourceDelegate::onOriginalPageConvertedToNote(Note note)
{
    QNDEBUG("RemoveResourceDelegate::onOriginalPageConvertedToNote");

    Q_UNUSED(note)

    // TODO: implement
}

void RemoveResourceDelegate::onPageHtmlWithoutResourceReceived(const QString & html)
{
    QNDEBUG("RemoveResourceDelegate::onPageHtmlWithoutResourceReceived");

    Q_UNUSED(html)

    // TODO: implement
}

void RemoveResourceDelegate::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    QNDEBUG("RemoveResourceDelegate::onWriteFileRequestProcessed: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId);

    // TODO: implement
}

void RemoveResourceDelegate::onModifiedPageLoaded()
{
    QNDEBUG("RemoveResourceDelegate::onModifiedPageLoaded");

    // TODO: implement
}

void RemoveResourceDelegate::doStart()
{
    QNDEBUG("RemoveResourceDelegate::doStart");

    // TODO: implement
}

void RemoveResourceDelegate::removeResourceFromPage()
{
    QNDEBUG("RemoveResourceDelegate::removeResourceFromPage");

    // TODO: implement
}

} // namespace qute_note
