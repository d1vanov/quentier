#include "NoteEditorView.h"
#include "NoteEditorController.h"
#include <logging/QuteNoteLogger.h>
#include <QMimeDatabase>
#include <QDropEvent>
#include <QUrl>
#include <QFileInfo>

namespace qute_note {

NoteEditorView::NoteEditorView(QWidget * parent) :
    QWebView(parent),
    m_pNoteEditorController(nullptr),
    m_noteEditorResourceInserters()
{
    setAcceptDrops(true);
}

void NoteEditorView::setController(NoteEditorController * pController)
{
    m_pNoteEditorController = pController;
}

void NoteEditorView::addResourceInserterForMimeType(const QString & mimeTypeName,
                                                    INoteEditorResourceInserter * pResourceInserter)
{
    QNDEBUG("NoteEditorView::addResourceInserterForMimeType: mime type = " << mimeTypeName);

    if (!pResourceInserter) {
        QNINFO("Detected attempt to add null pointer to resource inserter for mime type " << mimeTypeName);
        return;
    }

    m_noteEditorResourceInserters[mimeTypeName] = pResourceInserter;
}

bool NoteEditorView::removeResourceInserterForMimeType(const QString & mimeTypeName)
{
    QNDEBUG("NoteEditorView::removeResourceInserterForMimeType: mime type = " << mimeTypeName);

    auto it = m_noteEditorResourceInserters.find(mimeTypeName);
    if (it == m_noteEditorResourceInserters.end()) {
        QNTRACE("Can't remove resource inserter for mime type " << mimeTypeName << ": not found");
        return false;
    }

    Q_UNUSED(m_noteEditorResourceInserters.erase(it));
    return true;
}

bool NoteEditorView::hasResourceInsertedForMimeType(const QString & mimeTypeName) const
{
    auto it = m_noteEditorResourceInserters.find(mimeTypeName);
    return (it != m_noteEditorResourceInserters.end());
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

    QList<QUrl> urls = pMimeData->urls();
    typedef QList<QUrl>::iterator Iter;
    Iter urlsEnd = urls.end();
    for(Iter it = urls.begin(); it != urlsEnd; ++it)
    {
        QString url = it->toString();
        if (url.toLower().startsWith("file://")) {
            url.remove(0,6);
            dropFile(url);
        }
    }
}

void NoteEditorView::dropFile(QString & filepath)
{
    QNDEBUG("NoteEditorView::dropFile: " << filepath);

    QFileInfo fileInfo(filepath);
    if (!fileInfo.isFile()) {
        QNINFO("Detected attempt to drop something else rather than file: " << filepath);
        return;
    }

    if (!fileInfo.isReadable()) {
        QNINFO("Detected attempt to drop file which is not readable: " << filepath);
        return;
    }

    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileInfo);

    if (!mimeType.isValid()) {
        QNINFO("Detected invalid mime type for file " << filepath);
        return;
    }

    if (!m_pNoteEditorController) {
        QNWARNING("Note editor controller is not installed");
        return;
    }

    QByteArray data = QFile(filepath).readAll();
    m_pNoteEditorController->insertNewResource(data, mimeType);

    QString mimeTypeName = mimeType.name();
    auto resourceInsertersEnd = m_noteEditorResourceInserters.end();
    for(auto it = m_noteEditorResourceInserters.begin(); it != resourceInsertersEnd; ++it)
    {
        if (mimeTypeName != it.key()) {
            continue;
        }

        QNTRACE("Found resource inserter for mime type " << mimeTypeName);
        INoteEditorResourceInserter * pResourceInserter = it.value();
        if (!pResourceInserter) {
            QNINFO("Detected null pointer to registered resource inserter for mime type " << mimeTypeName);
            continue;
        }

        pResourceInserter->insertResource(data, mimeType, *this);
        return;
    }

    // If we are still here, no specific resource inserter was found, so will process two generic cases: image or any other type
    if (mimeTypeName.startsWith("image"))
    {
        // TODO: process image
        return;
    }

    // TODO: process generic mime type
}

} // namespace qute_note

