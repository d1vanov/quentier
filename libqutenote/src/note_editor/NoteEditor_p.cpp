#include "NoteEditor_p.h"
#include "EncryptionDialog.h"
#include "DecryptionDialog.h"
#include "EditUrlDialog.h"
#include "delegates/EncryptSelectedTextDelegate.h"
#include "javascript_glue/ResourceInfoJavaScriptHandler.h"
#include "javascript_glue/TextCursorPositionJavaScriptHandler.h"
#include "javascript_glue/ContextMenuEventJavaScriptHandler.h"
#include "javascript_glue/PageMutationHandler.h"
#include "undo_stack/NoteEditorContentEditUndoCommand.h"
#include "undo_stack/EncryptUndoCommand.h"
#include "undo_stack/DecryptUndoCommand.h"

#ifndef USE_QT_WEB_ENGINE
#include "EncryptedAreaPlugin.h"
#include <qute_note/utility/ApplicationSettings.h>
#include <QWebFrame>
typedef QWebSettings WebSettings;
#else
#include "javascript_glue/EnCryptElementOnClickHandler.h"
#include "javascript_glue/GenericResourceOpenAndSaveButtonsOnClickHandler.h"
#include "javascript_glue/GenericResourceImageJavaScriptHandler.h"
#include "javascript_glue/HyperlinkClickJavaScriptHandler.h"
#include "WebSocketClientWrapper.h"
#include "WebSocketTransport.h"
#include "GenericResourceImageWriter.h"
#include <qute_note/utility/ApplicationSettings.h>
#include <qute_note/utility/DesktopServices.h>
#include <QPainter>
#include <QIcon>
#include <QFontMetrics>
#include <QPixmap>
#include <QBuffer>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebChannel>
#include <QWebEngineSettings>
typedef QWebEngineSettings WebSettings;
#endif

#include <qute_note/note_editor/ResourceFileStorageManager.h>
#include <qute_note/exception/ResourceLocalFileStorageFolderNotFoundException.h>
#include <qute_note/exception/NoteEditorPluginInitializationException.h>
#include <qute_note/types/Note.h>
#include <qute_note/types/Notebook.h>
#include <qute_note/types/ResourceWrapper.h>
#include <qute_note/enml/ENMLConverter.h>
#include <qute_note/utility/Utility.h>
#include <qute_note/types/ResourceAdapter.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <qute_note/utility/FileIOThreadWorker.h>
#include <qute_note/utility/QuteNoteCheckPtr.h>
#include <qute_note/utility/DesktopServices.h>
#include <qute_note/utility/ShortcutManager.h>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QDropEvent>
#include <QMimeType>
#include <QMimeData>
#include <QMimeDatabase>
#include <QThread>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QKeySequence>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFontDialog>
#include <QFontDatabase>
#include <QUndoStack>

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(this->page()); \
    if (Q_UNLIKELY(!page)) { \
        QNFATAL("Can't get access to note editor's underlying page!"); \
        return; \
    }

namespace qute_note {

void NoteEditorPageDeleter(NoteEditorPage *& page) { delete page; page = Q_NULLPTR; }

NoteEditorPrivate::NoteEditorPrivate(NoteEditor & noteEditor) :
    INoteEditorBackend(&noteEditor),
    m_noteEditorPageFolderPath(),
    m_noteEditorPagePath(),
    m_noteEditorImageResourcesStoragePath(),
    m_font(),
    m_jQueryJs(),
    m_resizableTableColumnsJs(),
    m_debounceJs(),
    m_onFixedWidthTableResizeJs(),
    m_getSelectionHtmlJs(),
    m_snapSelectionToWordJs(),
    m_replaceSelectionWithHtmlJs(),
    m_findSelectedHyperlinkElementJs(),
    m_setHyperlinkToSelectionJs(),
    m_getHyperlinkFromSelectionJs(),
    m_removeHyperlinkFromSelectionJs(),
    m_provideSrcForResourceImgTagsJs(),
    m_setupEnToDoTagsJs(),
    m_flipEnToDoCheckboxStateJs(),
    m_onResourceInfoReceivedJs(),
    m_determineStatesForCurrentTextCursorPositionJs(),
    m_determineContextMenuEventTargetJs(),
    m_changeFontSizeForSelectionJs(),
    m_decryptEncryptedTextPermanentlyJs(),
    m_pageMutationObserverJs(),
#ifndef USE_QT_WEB_ENGINE
    m_qWebKitSetupJs(),
#else
    m_provideSrcForGenericResourceImagesJs(),
    m_onGenericResourceImageReceivedJs(),
    m_provideSrcAndOnClickScriptForEnCryptImgTagsJs(),
    m_qWebChannelJs(),
    m_qWebChannelSetupJs(),
    m_notifyTextCursorPositionChangedJs(),
    m_setupTextCursorPositionTrackingJs(),
    m_genericResourceOnClickHandlerJs(),
    m_setupGenericResourceOnClickHandlerJs(),
    m_clickInterceptorJs(),
    m_pWebSocketServer(new QWebSocketServer("QWebChannel server", QWebSocketServer::NonSecureMode, this)),
    m_pWebSocketClientWrapper(new WebSocketClientWrapper(m_pWebSocketServer, this)),
    m_pWebChannel(new QWebChannel(this)),
    m_pEnCryptElementClickHandler(new EnCryptElementOnClickHandler(this)),
    m_pGenericResourceOpenAndSaveButtonsOnClickHandler(new GenericResourceOpenAndSaveButtonsOnClickHandler(this)),
    m_pGenericResourceImageWriter(Q_NULLPTR),
    m_pHyperlinkClickJavaScriptHandler(new HyperlinkClickJavaScriptHandler(this)),
    m_webSocketServerPort(0),
#endif
    m_pPageMutationHandler(new PageMutationHandler(this)),
    m_pUndoStack(Q_NULLPTR),
    m_pPreliminaryUndoCommandQueue(Q_NULLPTR),
    m_contextMenuSequenceNumber(1),     // NOTE: must start from 1 as JavaScript treats 0 as null!
    m_lastContextMenuEventGlobalPos(),
    m_lastContextMenuEventPagePos(),
    m_pContextMenuEventJavaScriptHandler(new ContextMenuEventJavaScriptHandler(this)),
    m_pTextCursorPositionJavaScriptHandler(new TextCursorPositionJavaScriptHandler(this)),
    m_currentTextFormattingState(),
    m_writeNoteHtmlToFileRequestId(),
    m_isPageEditable(false),
    m_pendingConversionToNote(false),
    m_pendingNotePageLoad(false),
    m_pendingIndexHtmlWritingToFile(false),
    m_pendingJavaScriptExecution(false),
    m_skipPushingUndoCommandOnNextContentChange(false),
    m_pNote(Q_NULLPTR),
    m_pNotebook(Q_NULLPTR),
    m_modified(false),
    m_watchingForContentChange(false),
    m_contentChangedSinceWatchingStart(false),
    m_secondsToWaitBeforeConversionStart(30),
    m_pageToNoteContentPostponeTimerId(0),
    m_encryptionManager(new EncryptionManager),
    m_decryptedTextManager(),
    m_enmlConverter(),
#ifndef USE_QT_WEB_ENGINE
    m_pluginFactory(Q_NULLPTR),
#endif
    m_pGenericTextContextMenu(Q_NULLPTR),
    m_pImageResourceContextMenu(Q_NULLPTR),
    m_pNonImageResourceContextMenu(Q_NULLPTR),
    m_pEncryptedTextContextMenu(Q_NULLPTR),
    m_pagePrefix("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
                 "<html><head>"
                 "<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"UTF-8\" />"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-crypt.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/hover.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-decrypted.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-media-generic.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-media-image.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-todo.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/link.css\">"
                 "<title></title></head>"),
    m_lastSelectedHtml(),
    m_lastSelectedHtmlForEncryption(),
    m_enmlCachedMemory(),
    m_htmlCachedMemory(),
    m_errorCachedMemory(),
    m_skipRulesForHtmlToEnmlConversion(),
    m_pIOThread(Q_NULLPTR),
    m_pResourceFileStorageManager(Q_NULLPTR),
    m_pFileIOThreadWorker(Q_NULLPTR),
    m_resourceInfo(),
    m_pResourceInfoJavaScriptHandler(new ResourceInfoJavaScriptHandler(m_resourceInfo, this)),
    m_resourceLocalFileStorageFolder(),
    m_genericResourceLocalGuidBySaveToStorageRequestIds(),
    m_resourceFileStoragePathsByResourceLocalGuid(),
    m_localGuidsOfResourcesWantedToBeSaved(),
    m_localGuidsOfResourcesWantedToBeOpened(),
    m_manualSaveResourceToFileRequestIds(),
    m_fileSuffixesForMimeType(),
    m_fileFilterStringForMimeType(),
#ifdef USE_QT_WEB_ENGINE
    m_genericResourceImageFilePathsByResourceHash(),
    m_pGenericResoureImageJavaScriptHandler(new GenericResourceImageJavaScriptHandler(m_genericResourceImageFilePathsByResourceHash, this)),
    m_saveGenericResourceImageToFileRequestIds(),
#endif
    m_currentContextMenuExtraData(),
    m_droppedFilePathsAndMimeTypesByReadRequestIds(),
    m_lastFreeEnToDoIdNumber(1),
    m_lastFreeHyperlinkIdNumber(1),
    m_lastFreeEnCryptIdNumber(1),
    m_lastFreeEnDecryptedIdNumber(1),
    m_lastEncryptedText(),
    m_pagesStack(&NoteEditorPageDeleter),
    m_lastNoteEditorPageFreeIndex(0),
    q_ptr(&noteEditor)
{
    QString initialHtml = m_pagePrefix + "<body></body></html>";
    m_noteEditorPageFolderPath = applicationPersistentStoragePath() + "/NoteEditorPage";
    m_noteEditorImageResourcesStoragePath = m_noteEditorPageFolderPath + "/imageResources";

    setupSkipRulesForHtmlToEnmlConversion();
    setupFileIO();

#ifdef USE_QT_WEB_ENGINE
    setupWebSocketServer();
    setupJavaScriptObjects();
#endif

    setupTextCursorPositionJavaScriptHandlerConnections();
    setupGeneralSignalSlotConnections();
    setupNoteEditorPage();
    setupScripts();

    setAcceptDrops(true);

    m_resourceLocalFileStorageFolder = ResourceFileStorageManager::resourceFileStorageLocation(this);
    if (m_resourceLocalFileStorageFolder.isEmpty()) {
        QString error = QT_TR_NOOP("Can't get resource file storage folder");
        QNWARNING(error);
        throw ResourceLocalFileStorageFolderNotFoundException(error);
    }
    QNTRACE("Resource local file storage folder: " << m_resourceLocalFileStorageFolder);

    m_writeNoteHtmlToFileRequestId = QUuid::createUuid();
    emit writeNoteHtmlToFile(m_noteEditorPagePath, initialHtml.toLocal8Bit(),
                             m_writeNoteHtmlToFileRequestId);
    m_pendingIndexHtmlWritingToFile = true;
    QNTRACE("Emitted the request to write the index html file, request id: " << m_writeNoteHtmlToFileRequestId);
}

NoteEditorPrivate::~NoteEditorPrivate()
{
    delete m_pNote;
    delete m_pNotebook;

    m_pIOThread->quit();
}

void NoteEditorPrivate::onNoteLoadFinished(bool ok)
{
    QNDEBUG("NoteEditorPrivate::onNoteLoadFinished: ok = " << (ok ? "true" : "false"));

    m_pendingNotePageLoad = false;

    if (!ok) {
        QNWARNING("Note page was not loaded successfully");
        return;
    }

    m_pendingJavaScriptExecution = true;

    GET_PAGE()

    page->executeJavaScript(m_jQueryJs);

#ifndef USE_QT_WEB_ENGINE
    QWebFrame * frame = page->mainFrame();
    if (Q_UNLIKELY(!frame)) {
        return;
    }

    frame->addToJavaScriptWindowObject("pageMutationObserver", m_pPageMutationHandler,
                                       QScriptEngine::QtOwnership);
    frame->addToJavaScriptWindowObject("resourceCache", m_pResourceInfoJavaScriptHandler,
                                       QScriptEngine::QtOwnership);
    frame->addToJavaScriptWindowObject("textCursorPositionHandler", m_pTextCursorPositionJavaScriptHandler,
                                       QScriptEngine::QtOwnership);
    frame->addToJavaScriptWindowObject("contextMenuEventHandler", m_pContextMenuEventJavaScriptHandler,
                                       QScriptEngine::QtOwnership);

    page->executeJavaScript(m_onResourceInfoReceivedJs);
    page->executeJavaScript(m_qWebKitSetupJs);
#else
    page->executeJavaScript(m_qWebChannelJs);
    page->executeJavaScript(QString("(function(){window.websocketserverport = ") +
                            QString::number(m_webSocketServerPort) + QString("})();"));
    page->executeJavaScript(m_onResourceInfoReceivedJs);
    page->executeJavaScript(m_onGenericResourceImageReceivedJs);
    page->executeJavaScript(m_qWebChannelSetupJs);
    page->executeJavaScript(m_genericResourceOnClickHandlerJs);
    page->executeJavaScript(m_setupGenericResourceOnClickHandlerJs);
    page->executeJavaScript(m_provideSrcAndOnClickScriptForEnCryptImgTagsJs);
    page->executeJavaScript(m_provideSrcForGenericResourceImagesJs);
    page->executeJavaScript(m_clickInterceptorJs);
    page->executeJavaScript(m_notifyTextCursorPositionChangedJs);
#endif

    page->executeJavaScript(m_resizableTableColumnsJs);
    page->executeJavaScript(m_debounceJs);
    page->executeJavaScript(m_onFixedWidthTableResizeJs);
    page->executeJavaScript(m_getSelectionHtmlJs);
    page->executeJavaScript(m_snapSelectionToWordJs);
    page->executeJavaScript(m_findSelectedHyperlinkElementJs);
    page->executeJavaScript(m_replaceSelectionWithHtmlJs);
    page->executeJavaScript(m_setHyperlinkToSelectionJs);
    page->executeJavaScript(m_getHyperlinkFromSelectionJs);
    page->executeJavaScript(m_removeHyperlinkFromSelectionJs);
    page->executeJavaScript(m_provideSrcForResourceImgTagsJs);
    page->executeJavaScript(m_setupEnToDoTagsJs);
    page->executeJavaScript(m_flipEnToDoCheckboxStateJs);
    page->executeJavaScript(m_determineStatesForCurrentTextCursorPositionJs);
    page->executeJavaScript(m_determineContextMenuEventTargetJs);
    page->executeJavaScript(m_changeFontSizeForSelectionJs);
    page->executeJavaScript(m_decryptEncryptedTextPermanentlyJs);

    setPageEditable(true);
    updateColResizableTableBindings();
    saveNoteResourcesToLocalFiles();

#ifdef USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
    page->executeJavaScript(m_setupTextCursorPositionTrackingJs);
    setupTextCursorPositionTracking();
    setupGenericResourceImages();
#endif

    // NOTE: executing page mutation observer's script last
    // so that it doesn't catch the mutations originating from the above scripts
    page->executeJavaScript(m_pageMutationObserverJs);

    QNTRACE("Sent commands to execute all the page's necessary scripts");
}

void NoteEditorPrivate::onContentChanged()
{
    QNTRACE("NoteEditorPrivate::onContentChanged");

    if (m_pendingNotePageLoad || m_pendingIndexHtmlWritingToFile || m_pendingJavaScriptExecution) {
        QNTRACE("Skipping the content change as the note page has not fully loaded yet");
        return;
    }

    if (m_skipPushingUndoCommandOnNextContentChange) {
        m_skipPushingUndoCommandOnNextContentChange = false;
        QNTRACE("Skipping the push of edit undo command on this content change");
    }
    else {
        pushNoteContentEditUndoCommand();
    }

    m_modified = true;

    if (Q_LIKELY(m_watchingForContentChange)) {
        m_contentChangedSinceWatchingStart = true;
        return;
    }

    m_pageToNoteContentPostponeTimerId = startTimer(SEC_TO_MSEC(m_secondsToWaitBeforeConversionStart));
    m_watchingForContentChange = true;
    m_contentChangedSinceWatchingStart = false;
    QNTRACE("Started timer to postpone note editor page's content to ENML conversion: timer id = "
            << m_pageToNoteContentPostponeTimerId);
}

void NoteEditorPrivate::onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                                 QString fileStoragePath, int errorCode,
                                                 QString errorDescription)
{
    auto it = m_genericResourceLocalGuidBySaveToStorageRequestIds.find(requestId);
    if (it == m_genericResourceLocalGuidBySaveToStorageRequestIds.end()) {
        return;
    }

    QNDEBUG("NoteEditorPrivate::onResourceSavedToStorage: requestId = " << requestId
            << ", data hash = " << dataHash << ", file storage path = " << fileStoragePath
            << ", error code = " << errorCode << ", error description: " << errorDescription);

    if (errorCode != 0) {
        errorDescription.prepend(QT_TR_NOOP("Can't write resource to local file: "));
        emit notifyError(errorDescription);
        QNWARNING(errorDescription + ", error code = " << errorCode);
        return;
    }

    const QString localGuid = it.value();
    m_resourceFileStoragePathsByResourceLocalGuid[localGuid] = fileStoragePath;

    QString resourceDisplayName;
    QString resourceDisplaySize;

    if (m_pNote)
    {
        QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
        const int numResources = resourceAdapters.size();
        for(int i = 0; i < numResources; ++i)
        {
            ResourceAdapter & resourceAdapter = resourceAdapters[i];
            if (resourceAdapter.localGuid() != localGuid) {
                continue;
            }

            if (!resourceAdapter.hasDataHash()) {
                resourceAdapter.setDataHash(dataHash);
            }

            resourceDisplayName = resourceAdapter.displayName();
            if (resourceAdapter.hasDataSize()) {
                resourceDisplaySize = humanReadableSize(resourceAdapter.dataSize());
            }
        }
    }

    QString dataHashStr = QString::fromLocal8Bit(dataHash.constData(), dataHash.size());

    m_resourceInfo.cacheResourceInfo(dataHashStr, resourceDisplayName,
                                     resourceDisplaySize, fileStoragePath);

    Q_UNUSED(m_genericResourceLocalGuidBySaveToStorageRequestIds.erase(it));

    if (m_genericResourceLocalGuidBySaveToStorageRequestIds.isEmpty() && !m_pendingNotePageLoad) {
        QNTRACE("All current note's resources were saved to local file storage and are actual. "
                "Will set filepaths to these local files to src attributes of img resource tags");
        provideSrcForResourceImgTags();
    }

    auto saveIt = m_localGuidsOfResourcesWantedToBeSaved.find(localGuid);
    if (saveIt != m_localGuidsOfResourcesWantedToBeSaved.end())
    {
        QNTRACE("Resource with local guid " << localGuid << " is pending manual saving to file");

        Q_UNUSED(m_localGuidsOfResourcesWantedToBeSaved.erase(saveIt));

        if (Q_UNLIKELY(!m_pNote)) {
            QString error = QT_TR_NOOP("Can't save resource: no note is set to the editor");
            QNINFO(error << ", resource local guid = " << localGuid);
            emit notifyError(error);
            return;
        }

        QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
        int numResources = resourceAdapters.size();
        for(int i = 0; i < numResources; ++i)
        {
            const ResourceAdapter & resource = resourceAdapters[i];
            if (resource.localGuid() == localGuid) {
                manualSaveResourceToFile(resource);
                return;
            }
        }

        QString error = QT_TR_NOOP("Can't save resource: can't find resource to save within note's resources");
        QNINFO(error << ", resource local guid = " << localGuid);
        emit notifyError(error);
    }

    auto openIt = m_localGuidsOfResourcesWantedToBeOpened.find(localGuid);
    if (openIt != m_localGuidsOfResourcesWantedToBeOpened.end()) {
        QNTRACE("Resource with local guid " << localGuid << " is pending opening in application");
        Q_UNUSED(m_localGuidsOfResourcesWantedToBeOpened.erase(openIt));
        openResource(fileStoragePath);
    }
}

void NoteEditorPrivate::onDroppedFileRead(bool success, QString errorDescription,
                                          QByteArray data, QUuid requestId)
{
    QNTRACE("NoteEditorPrivate::onDroppedFileRead: success = " << (success ? "true" : "false")
            << ", error description = " << errorDescription << ", request id = " << requestId);

    auto it = m_droppedFilePathsAndMimeTypesByReadRequestIds.find(requestId);
    if (it == m_droppedFilePathsAndMimeTypesByReadRequestIds.end()) {
        return;
    }

    const QString filePath = it.value().first;
    QFileInfo fileInfo(filePath);

    const QMimeType mimeType = it.value().second;

    Q_UNUSED(m_droppedFilePathsAndMimeTypesByReadRequestIds.erase(it));

    if (!success) {
        QNDEBUG("Could not read the content of the dropped file for request id " << requestId
                << ": " << errorDescription);
        return;
    }

    if (!m_pNote) {
        QNDEBUG("Current note is empty");
        return;
    }

    QNDEBUG("Successfully read the content of the dropped file for request id " << requestId);
    QByteArray dataHash;
    QString newResourceLocalGuid = attachResourceToNote(data, dataHash, mimeType, fileInfo.fileName());

    QUuid saveResourceToStorageRequestId = QUuid::createUuid();

    QString fileStoragePath;
    if (mimeType.name().startsWith("image/")) {
        fileStoragePath = m_noteEditorImageResourcesStoragePath;
    }
    else {
        fileStoragePath = m_resourceLocalFileStorageFolder;
    }

    fileStoragePath += "/" + newResourceLocalGuid;

    QString fileInfoSuffix = fileInfo.completeSuffix();
    if (!fileInfoSuffix.isEmpty())
    {
        fileStoragePath += "." + fileInfoSuffix;
    }
    else
    {
        const QStringList suffixes = mimeType.suffixes();
        if (!suffixes.isEmpty()) {
            fileStoragePath += "." + suffixes.front();
        }
    }

    m_genericResourceLocalGuidBySaveToStorageRequestIds[saveResourceToStorageRequestId] = newResourceLocalGuid;
    emit saveResourceToStorage(newResourceLocalGuid, data, dataHash, fileStoragePath, saveResourceToStorageRequestId);

    QNTRACE("Emitted request to save the dropped resource to local file storage: generated local guid = "
            << newResourceLocalGuid << ", data hash = " << dataHash << ", request id = "
            << saveResourceToStorageRequestId << ", mime type name = " << mimeType.name());
}

#ifdef USE_QT_WEB_ENGINE
void NoteEditorPrivate::onGenericResourceImageSaved(const bool success, const QByteArray resourceActualHash,
                                                    const QString filePath, const QString errorDescription,
                                                    const QUuid requestId)
{
    QNDEBUG("NoteEditorPrivate::onGenericResourceImageSaved: success = " << (success ? "true" : "false")
            << ", resource actual hash = " << resourceActualHash
            << ", file path = " << filePath << ", error description = " << errorDescription
            << ", requestId = " << requestId);

    auto it = m_saveGenericResourceImageToFileRequestIds.find(requestId);
    if (it == m_saveGenericResourceImageToFileRequestIds.end()) {
        QNDEBUG("Haven't found request id in the cache");
        return;
    }

    Q_UNUSED(m_saveGenericResourceImageToFileRequestIds.erase(it));

    if (Q_UNLIKELY(!success)) {
        emit notifyError(QT_TR_NOOP("Can't save generic resource image to file: ") + errorDescription);
        return;
    }

    m_genericResourceImageFilePathsByResourceHash[resourceActualHash] = filePath;
    QNDEBUG("Cached generic resource image file path " << filePath << " for resource hash " << resourceActualHash);

    if (m_saveGenericResourceImageToFileRequestIds.empty()) {
        provideSrcForGenericResourceImages();
        setupGenericResourceOnClickHandler();
    }
}

void NoteEditorPrivate::onHyperlinkClicked(QString url)
{
    openUrl(QUrl(url));
}

#else

void NoteEditorPrivate::onHyperlinkClicked(QUrl url)
{
    openUrl(url);
}

#endif

void NoteEditorPrivate::onJavaScriptLoaded()
{
    QNDEBUG("NoteEditorPrivate::onJavaScriptLoaded");

    NoteEditorPage * pSenderPage = qobject_cast<NoteEditorPage*>(sender());
    if (Q_UNLIKELY(!pSenderPage)) {
        QNWARNING("Can't get the pointer to NoteEditor page from which the event of JavaScrupt loading came in");
        return;
    }

    GET_PAGE()
    if (page != pSenderPage) {
        QNDEBUG("Skipping JavaScript loaded event from page which is not the currently set one");
        return;
    }

    if (m_pendingJavaScriptExecution)
    {
        m_pendingJavaScriptExecution = false;

#ifndef USE_QT_WEB_ENGINE
        m_htmlCachedMemory = page->mainFrame()->toHtml();
        onPageHtmlReceived(m_htmlCachedMemory);
#else
        page->toHtml(NoteEditorCallbackFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
#endif
    }
}

void NoteEditorPrivate::onOpenResourceRequest(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::onOpenResourceRequest: " << resourceHash);

    if (Q_UNLIKELY(!m_pNote)) {
        QString error = QT_TR_NOOP("Can't open resource: no note is set to the editor");
        QNWARNING(error << ", resource hash = " << resourceHash);
        emit notifyError(error);
        return;
    }

    QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
    int resourceIndex = resourceIndexByHash(resourceAdapters, resourceHash);
    if (Q_UNLIKELY(resourceIndex < 0)) {
        QString error = QT_TR_NOOP("Resource to be opened was not found in the note");
        QNWARNING(error << ", resource hash = " << resourceHash);
        emit notifyError(error);
        return;
    }

    const ResourceAdapter & resource = resourceAdapters[resourceIndex];
    const QString resourceLocalGuid = resource.localGuid();

    // See whether this resource has already been written to file
    auto it = m_resourceFileStoragePathsByResourceLocalGuid.find(resourceLocalGuid);
    if (it == m_resourceFileStoragePathsByResourceLocalGuid.end()) {
        // It must be being written to file at the moment - all note's resources are saved to files on note load -
        // so just mark this resource local guid as pending for open
        Q_UNUSED(m_localGuidsOfResourcesWantedToBeOpened.insert(resourceLocalGuid));
        return;
    }

    openResource(it.value());
}

void NoteEditorPrivate::onSaveResourceRequest(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::onSaveResourceRequest: " << resourceHash);

    if (Q_UNLIKELY(!m_pNote)) {
        QString error = QT_TR_NOOP("Can't save resource: no note is set to the editor");
        QNINFO(error << ", resource hash = " << resourceHash);
        emit notifyError(error);
        return;
    }

    QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
    int resourceIndex = resourceIndexByHash(resourceAdapters, resourceHash);
    if (Q_UNLIKELY(resourceIndex < 0)) {
        QString error = QT_TR_NOOP("Resource to be saved was not found in the note");
        QNINFO(error << ", resource hash = " << resourceHash);
        return;
    }

    const ResourceAdapter & resource = resourceAdapters[resourceIndex];
    const QString resourceLocalGuid = resource.localGuid();

    // See whether this resource has already been written to file
    auto it = m_resourceFileStoragePathsByResourceLocalGuid.find(resourceLocalGuid);
    if (it == m_resourceFileStoragePathsByResourceLocalGuid.end()) {
        // It must be being written to file at the moment - all note's resources are saved to files on note load -
        // so just mark this resource local guid as pending for save
        Q_UNUSED(m_localGuidsOfResourcesWantedToBeSaved.insert(resourceLocalGuid));
        return;
    }

    manualSaveResourceToFile(resource);
}

void NoteEditorPrivate::onEnCryptElementClicked(QString encryptedText, QString cipher,
                                                QString length, QString hint, bool *pCancelled)
{
    QNDEBUG("NoteEditorPrivate::onEnCryptElementClicked");

    if (cipher.isEmpty()) {
        cipher = "AES";
    }

    if (length.isEmpty()) {
        length = "128";
    }

    bool conversionResult = false;
    size_t keyLength = static_cast<size_t>(length.toInt(&conversionResult));
    if (!conversionResult) {
        QNFATAL("NoteEditorPrivate::onEnCryptElementClicked: can't convert encryption key from string to number: " << length);
        return;
    }

    QScopedPointer<DecryptionDialog> pDecryptionDialog(new DecryptionDialog(encryptedText,
                                                                            cipher, hint, keyLength,
                                                                            m_encryptionManager,
                                                                            m_decryptedTextManager, this));
    pDecryptionDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pDecryptionDialog.data(), QNSIGNAL(DecryptionDialog,accepted,QString,size_t,QString,QString,QString,bool,bool,bool),
                     this, QNSLOT(NoteEditorPrivate,onEncryptedAreaDecryption,QString,size_t,QString,QString,QString,bool,bool,bool));
    QNTRACE("Will exec decryption dialog now");
    int res = pDecryptionDialog->exec();
    if (pCancelled) {
        *pCancelled = (res == QDialog::Rejected);
    }

    QNTRACE("Executed decryption dialog: " << (res == QDialog::Accepted ? "accepted" : "rejected"));
}

void NoteEditorPrivate::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNTRACE("NoteEditorPrivate::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNINFO("detected null pointer to context menu event");
        return;
    }

    if (m_pendingIndexHtmlWritingToFile || m_pendingNotePageLoad || m_pendingJavaScriptExecution) {
        QNINFO("Ignoring context menu event for now, until the note is fully loaded...");
        return;
    }

    m_lastContextMenuEventGlobalPos = pEvent->globalPos();
    m_lastContextMenuEventPagePos = pEvent->pos();

    QNTRACE("Context menu event's global pos: x = " << m_lastContextMenuEventGlobalPos.x()
            << ", y = " << m_lastContextMenuEventGlobalPos.y() << "; pos relative to child widget: x = "
            << m_lastContextMenuEventPagePos.x() << ", y = " << m_lastContextMenuEventPagePos.y()
            << "; context menu sequence number = " << m_contextMenuSequenceNumber);

    determineContextMenuEventTarget();
}

void NoteEditorPrivate::onContextMenuEventReply(QString contentType, QString selectedHtml,
                                                bool insideDecryptedTextFragment,
                                                QStringList extraData, quint64 sequenceNumber)
{
    QNDEBUG("NoteEditorPrivate::onContextMenuEventReply: content type = " << contentType
            << ", selected html = " << selectedHtml << ", inside decrypted text fragment = "
            << (insideDecryptedTextFragment ? "true" : "false") << ", extraData: [" << extraData.join(", ")
            << "], sequence number = " << sequenceNumber);

    if (!checkContextMenuSequenceNumber(sequenceNumber)) {
        QNTRACE("Sequence number is not valid, not doing anything");
        return;
    }

    ++m_contextMenuSequenceNumber;

    if (contentType == "GenericText") {
        setupGenericTextContextMenu(selectedHtml, insideDecryptedTextFragment);
    }
    else if (contentType == "ImageResource")
    {
        if (Q_UNLIKELY(extraData.empty())) {
            QString error = QT_TR_NOOP("Can't display context menu for image resource: "
                                       "extra data from JavaScript is empty");
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        if (Q_UNLIKELY(extraData.size() != 1)) {
            QString error = QT_TR_NOOP("Can't display context menu for image resource: "
                                       "extra data from JavaScript has wrong size");
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        const QString & resourceHash = extraData[0];
        setupImageResourceContextMenu(resourceHash);
    }
    else if (contentType == "NonImageResource") {
        setupNonImageResourceContextMenu();
    }
    else if (contentType == "EncryptedText")
    {
        if (Q_UNLIKELY(extraData.empty())) {
            QString error = QT_TR_NOOP("Can't display context menu for encrypted text: "
                                       "extra data from JavaScript is empty");
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        if (Q_UNLIKELY(extraData.size() != 4)) {
            QString error = QT_TR_NOOP("Can't display context menu for encrypted text: "
                                       "extra data from JavaScript has wrong size");
            QNWARNING(error << ": " << extraData.join(", "));
            emit notifyError(error);
            return;
        }

        const QString & cipher = extraData[0];
        const QString & keyLength = extraData[1];
        const QString & encryptedText = extraData[2];
        const QString & hint = extraData[3];

        setupEncryptedTextContextMenu(cipher, keyLength, encryptedText, hint);
    }
    else {
        QNWARNING("Unknown content type on context menu event reply: " << contentType << ", sequence number " << sequenceNumber);
    }
}

void NoteEditorPrivate::onTextCursorPositionChange()
{
    QNDEBUG("NoteEditorPrivate::onTextCursorPositionChange");
    if (!m_pendingIndexHtmlWritingToFile && !m_pendingNotePageLoad && !m_pendingJavaScriptExecution) {
        determineStatesForCurrentTextCursorPosition();
    }
}

void NoteEditorPrivate::onTextCursorBoldStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorBoldStateChanged: " << (state ? "bold" : "not bold"));
    m_currentTextFormattingState.m_bold = state;

    emit textBoldState(state);
}

void NoteEditorPrivate::onTextCursorItalicStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorItalicStateChanged: " << (state ? "italic" : "not italic"));
    m_currentTextFormattingState.m_italic = state;

    emit textItalicState(state);
}

void NoteEditorPrivate::onTextCursorUnderlineStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorUnderlineStateChanged: " << (state ? "underline" : "not underline"));
    m_currentTextFormattingState.m_underline = state;

    emit textUnderlineState(state);
}

void NoteEditorPrivate::onTextCursorStrikethgouthStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorStrikethgouthStateChanged: " << (state ? "strikethrough" : "not strikethrough"));
    m_currentTextFormattingState.m_strikethrough = state;

    emit textStrikethroughState(state);
}

void NoteEditorPrivate::onTextCursorAlignLeftStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorAlignLeftStateChanged: " << (state ? "true" : "false"));

    if (state) {
        m_currentTextFormattingState.m_alignment = Alignment::Left;
    }

    emit textAlignLeftState(state);
}

void NoteEditorPrivate::onTextCursorAlignCenterStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorAlignCenterStateChanged: " << (state ? "true" : "false"));

    if (state) {
        m_currentTextFormattingState.m_alignment = Alignment::Center;
    }

    emit textAlignCenterState(state);
}

void NoteEditorPrivate::onTextCursorAlignRightStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorAlignRightStateChanged: " << (state ? "true" : "false"));

    if (state) {
        m_currentTextFormattingState.m_alignment = Alignment::Right;
    }

    emit textAlignRightState(state);
}

void NoteEditorPrivate::onTextCursorInsideOrderedListStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorInsideOrderedListStateChanged: " << (state ? "true" : "false"));
    m_currentTextFormattingState.m_insideOrderedList = state;

    emit textInsideOrderedListState(state);
}

void NoteEditorPrivate::onTextCursorInsideUnorderedListStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorInsideUnorderedListStateChanged: " << (state ? "true" : "false"));
    m_currentTextFormattingState.m_insideUnorderedList = state;

    emit textInsideUnorderedListState(state);
}

void NoteEditorPrivate::onTextCursorInsideTableStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorInsideTableStateChanged: " << (state ? "true" : "false"));
    m_currentTextFormattingState.m_insideTable = state;

    emit textInsideTableState(state);
}

void NoteEditorPrivate::onTextCursorOnImageResourceStateChanged(bool state, QString resourceHash)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorOnImageResourceStateChanged: " << (state ? "yes" : "no")
            << ", resource hash = " << resourceHash);

    m_currentTextFormattingState.m_onImageResource = state;
    if (state) {
        m_currentTextFormattingState.m_resourceHash = resourceHash;
    }
}

void NoteEditorPrivate::onTextCursorOnNonImageResourceStateChanged(bool state, QString resourceHash)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorOnNonImageResourceStateChanged: " << (state ? "yes" : "no")
            << ", resource hash = " << resourceHash);

    m_currentTextFormattingState.m_onNonImageResource = state;
    if (state) {
        m_currentTextFormattingState.m_resourceHash = resourceHash;
    }
}

void NoteEditorPrivate::onTextCursorOnEnCryptTagStateChanged(bool state, QString encryptedText, QString cipher, QString length)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorOnEnCryptTagStateChanged: " << (state ? "yes" : "no")
            << ", encrypted text = " << encryptedText << ", cipher = " << cipher << ", length = " << length);
    m_currentTextFormattingState.m_onEnCryptTag = state;
    if (state) {
        m_currentTextFormattingState.m_encryptedText = encryptedText;
        m_currentTextFormattingState.m_cipher = cipher;
        m_currentTextFormattingState.m_length = length;
    }
}

void NoteEditorPrivate::onTextCursorFontNameChanged(QString fontName)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorFontNameChanged: font name = " << fontName);
    emit textFontFamilyChanged(fontName);
}

void NoteEditorPrivate::onTextCursorFontSizeChanged(int fontSize)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorFontSizeChanged: font size = " << fontSize);
    emit textFontSizeChanged(fontSize);
}

void NoteEditorPrivate::onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId)
{
    if (requestId == m_writeNoteHtmlToFileRequestId)
    {
        QNDEBUG("Write note html to file completed: success = " << (success ? "true" : "false")
                << ", error description = " << errorDescription << ", request id = " << requestId);

        m_writeNoteHtmlToFileRequestId = QUuid();
        m_pendingIndexHtmlWritingToFile = false;

        if (!success) {
            clearEditorContent();
            errorDescription.prepend(QT_TR_NOOP("Could not write note html to file: "));
            emit notifyError(errorDescription);
            QNWARNING(errorDescription);
            return;
        }

        QUrl url = QUrl::fromLocalFile(m_noteEditorPagePath);

#ifdef USE_QT_WEB_ENGINE
        page()->setUrl(url);
        page()->load(url);
#else
        page()->mainFrame()->setUrl(url);
        page()->mainFrame()->load(url);
#endif
        QNTRACE("Loaded url: " << url);
        m_pendingNotePageLoad = true;
    }

    auto manualSaveResourceIt = m_manualSaveResourceToFileRequestIds.find(requestId);
    if (manualSaveResourceIt != m_manualSaveResourceToFileRequestIds.end())
    {
        if (success) {
            QNDEBUG("Successfully saved resource to file for request id " << requestId);
        }
        else {
            QNWARNING("Could not save resource to file: " << errorDescription);
        }

        Q_UNUSED(m_manualSaveResourceToFileRequestIds.erase(manualSaveResourceIt));
        return;
    }
}

void NoteEditorPrivate::onEncryptSelectedTextDelegateFinished()
{
    QNDEBUG("NoteEditorPrivate::onEncryptSelectedTextDelegateFinished");
    sender()->deleteLater();
}

void NoteEditorPrivate::onEncryptSelectedTextDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onEncryptSelectedTextDelegateError: " << error);
    emit notifyError(error);
    sender()->deleteLater();
}

void NoteEditorPrivate::timerEvent(QTimerEvent * event)
{
    QNDEBUG("NoteEditorPrivate::timerEvent: " << (event ? QString::number(event->timerId()) : "<null>"));

    if (!event) {
        return;
    }

    if (event->timerId() == m_pageToNoteContentPostponeTimerId)
    {
        if (m_contentChangedSinceWatchingStart)
        {
            QNTRACE("Note editor page's content has been changed lately, "
                    "the editing is most likely in progress now, postponing "
                    "the conversion to ENML");
            m_contentChangedSinceWatchingStart = false;
            return;
        }

        QNTRACE("Looks like the note editing has stopped for a while, "
                "will convert the note editor page's content to ENML");
        bool res = htmlToNoteContent(m_errorCachedMemory);
        if (!res) {
            emit notifyError(m_errorCachedMemory);
        }

        killTimer(m_pageToNoteContentPostponeTimerId);
        m_pageToNoteContentPostponeTimerId = 0;

        m_watchingForContentChange = false;
        m_contentChangedSinceWatchingStart = false;
    }
}

void NoteEditorPrivate::pushNoteContentEditUndoCommand()
{
    QNDEBUG("NoteEditorPrivate::pushNoteTextEditUndoCommand");

    QUTE_NOTE_CHECK_PTR(m_pPreliminaryUndoCommandQueue,
                        "Preliminaty undo command queue for note editor wasn't initialized");

    if (Q_UNLIKELY(!m_pNote)) {
        QNINFO("Ignoring the content changed signal as the note pointer is null");
        return;
    }

    QList<ResourceWrapper> resources;
    if (m_pNote->hasResources()) {
        resources = m_pNote->resources();
    }

    m_pPreliminaryUndoCommandQueue->push(new NoteEditorContentEditUndoCommand(*this, resources));
}

void NoteEditorPrivate::pushDecryptUndoCommand(const QString & cipher, const size_t keyLength,
                                               const QString & encryptedText, const QString & decryptedText,
                                               const QString & passphrase, const bool rememberForSession,
                                               const bool decryptPermanently)
{
    QNDEBUG("NoteEditorPrivate::pushDecryptUndoCommand");

    EncryptDecryptUndoCommandInfo info;
    info.m_cipher = cipher;
    info.m_keyLength = keyLength;
    info.m_rememberForSession = rememberForSession;
    info.m_decryptedText = decryptedText;
    info.m_decryptPermanently = decryptPermanently;
    info.m_encryptedText = encryptedText;
    info.m_passphrase = passphrase;

    m_pPreliminaryUndoCommandQueue->push(new DecryptUndoCommand(info, m_decryptedTextManager, *this));
    QNTRACE("Pushed DecryptUndoCommand to the undo stack");
}

void NoteEditorPrivate::switchEditorPage(const bool shouldConvertFromNote)
{
    QNDEBUG("NoteEditorPrivate::switchEditorPage: should convert from note = "
            << (shouldConvertFromNote ? "true" : "false"));

    GET_PAGE()

    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded), this, QNSLOT(NoteEditorPrivate,onJavaScriptLoaded));
#ifndef USE_QT_WEB_ENGINE
    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,microFocusChanged), this, QNSLOT(NoteEditorPrivate,onTextCursorPositionChange));

    QWebFrame * frame = page->mainFrame();
    QObject::disconnect(frame, QNSIGNAL(QWebFrame,loadFinished,bool), this, QNSLOT(NoteEditorPrivate,onNoteLoadFinished,bool));
#else
    QObject::disconnect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool), this, QNSLOT(NoteEditorPrivate,onNoteLoadFinished,bool));
#endif

    page->setView(Q_NULLPTR);
    page->setParent(Q_NULLPTR);
    m_pagesStack.push(page);

    setupNoteEditorPage();

    if (shouldConvertFromNote) {
        noteToEditorContent();
    }
}

void NoteEditorPrivate::popEditorPage()
{
    QNDEBUG("NoteEditorPrivate::popEditorPage");

    if (Q_UNLIKELY(m_pagesStack.isEmpty())) {
        m_errorCachedMemory = QT_TR_NOOP("The stack of note pages is unexpectedly empty after popping out the top page");
        QNWARNING(m_errorCachedMemory);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    NoteEditorPage * page = m_pagesStack.pop();
    if (Q_UNLIKELY(!page)) {
        m_errorCachedMemory = QT_TR_NOOP("Detected null pointer to note editor page in the pages stack");
        QNWARNING(m_errorCachedMemory);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    --m_lastNoteEditorPageFreeIndex;
    QNTRACE("Updated last note editor page free index to " << m_lastNoteEditorPageFreeIndex);

    page->setView(this);
    updateNoteEditorPagePath(page->index());
    setupNoteEditorPageConnections(page);

    setPage(page);

#ifdef USE_QT_WEB_ENGINE
    QNTRACE("Set note editor page with url: " << page->url());
#else
    QNTRACE("Set note editor page with url: " << page->mainFrame()->url());
#endif

    convertToNote();
}

void NoteEditorPrivate::skipPushingUndoCommandOnNextContentChange()
{
    QNDEBUG("NoteEditorPrivate::skipPushingUndoCommandOnNextContentChange");
    m_skipPushingUndoCommandOnNextContentChange = true;
}

void NoteEditorPrivate::setNotePageHtmlAfterEncryption(const QString & html)
{
    QNDEBUG("NoteEditorPrivate::setNotePageHtmlAfterEncryption");

    EncryptUndoCommand * pCommand = new EncryptUndoCommand(*this);
    pCommand->setHtmlWithEncryption(html);
    m_pPreliminaryUndoCommandQueue->push(pCommand);
    QNTRACE("Pushed EncryptUndoCommand to the undo stack");
}

void NoteEditorPrivate::undoLastEncryption()
{
    QNDEBUG("NoteEditorPrivate::undoLastEncryption");

    if (m_lastFreeEnCryptIdNumber == 1) {
        QNWARNING("Detected attempt to undo last encryption even though "
                  "there're no encrypted text nodes within the current node");
        return;
    }

    if (Q_UNLIKELY(m_lastEncryptedText.isEmpty())) {
        QNWARNING("Detected attempt to undo last encryption even though "
                  "last encrypted text is empty");
        return;
    }

    QString decryptedText;
    bool rememberForSession;
    bool found = m_decryptedTextManager.findDecryptedTextByEncryptedText(m_lastEncryptedText,
                                                                         decryptedText,
                                                                         rememberForSession);
    if (!found) {
        QString error = QT_TR_NOOP("Can't undo last encryption: can't find corresponding decrypted text");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_decryptedTextManager.removeEntry(m_lastEncryptedText);

    skipPushingUndoCommandOnNextContentChange();

    QString javascript = "decryptEncryptedTextPermanently('" + m_lastEncryptedText + "', '" +
                         decryptedText + "', " + QString::number(m_lastFreeEnCryptIdNumber - 1) + ");";
    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::changeFontSize(const bool increase)
{
    QNDEBUG("NoteEditorPrivate::changeFontSize: increase = " << (increase ? "true" : "false"));

    int fontSize = m_font.pointSize();
    if (fontSize < 0) {
        QNTRACE("Font size is negative which most likely means the font is not set yet, nothing to do. "
                "Current font: " << m_font);
        return;
    }

    QFontDatabase fontDatabase;
    QList<int> fontSizes = fontDatabase.pointSizes(m_font.family(), m_font.styleName());
    if (fontSizes.isEmpty()) {
        QNTRACE("Coulnd't find point sizes for font family " << m_font.family() << ", will use standard sizes instead");
        fontSizes = fontDatabase.standardSizes();
    }

    int fontSizeIndex = fontSizes.indexOf(fontSize);
    if (fontSizeIndex < 0)
    {
        QNTRACE("Couldn't find font size " << fontSize << " within the available sizes, will take the closest one instead");
        const int numFontSizes = fontSizes.size();
        int currentSmallestDiscrepancy = 1e5;
        int currentClosestIndex = -1;
        for(int i = 0; i < numFontSizes; ++i)
        {
            int value = fontSizes[i];

            int discrepancy = abs(value - fontSize);
            if (currentSmallestDiscrepancy > discrepancy) {
                currentSmallestDiscrepancy = discrepancy;
                currentClosestIndex = i;
                QNTRACE("Updated current closest index to " << i << ": font size = " << value);
            }
        }

        if (currentClosestIndex >= 0) {
            fontSizeIndex = currentClosestIndex;
        }
    }

    if (fontSizeIndex >= 0)
    {
        if (increase && (fontSizeIndex < (fontSizes.size() - 1))) {
            fontSize = fontSizes.at(fontSizeIndex + 1);
        }
        else if (!increase && (fontSizeIndex != 0)) {
            fontSize = fontSizes.at(fontSizeIndex - 1);
        }
        else {
            QNTRACE("Can't " << (increase ? "increase" : "decrease") << " the font size: hit the boundary of "
                    "available font sizes");
            return;
        }
    }
    else
    {
        QNTRACE("Wasn't able to find even the closest font size within the available ones, will simply "
                << (increase ? "increase" : "decrease") << " the given font size by 1 pt and see what happens");
        if (increase) {
            ++fontSize;
        }
        else {
            --fontSize;
            if (!fontSize) {
                fontSize = 1;
            }
        }
    }

    setFontHeight(fontSize);
}

void NoteEditorPrivate::changeIndentation(const bool increase)
{
    QNDEBUG("NoteEditorPrivate::changeIndentation: increase = " << (increase ? "true" : "false"));

    execJavascriptCommand((increase ? "indent" : "outdent"));
    setFocus();
}

void NoteEditorPrivate::replaceSelectedTextWithEncryptedOrDecryptedText(const QString & selectedText, const QString & encryptedText,
                                                                        const QString & hint, const bool rememberForSession)
{
    QNDEBUG("NoteEditorPrivate::replaceSelectedTextWithEncryptedOrDecryptedText: encrypted text = "
            << encryptedText << ", hint = " << hint << ", rememberForSession = " << (rememberForSession ? "true" : "false"));

    Q_UNUSED(selectedText);
    m_lastEncryptedText = encryptedText;

    QString encryptedTextHtmlObject = (rememberForSession
                                       ? ENMLConverter::decryptedTextHtml(selectedText, encryptedText, hint, "AES", 128, m_lastFreeEnDecryptedIdNumber++)
                                       : ENMLConverter::encryptedTextHtml(encryptedText, hint, "AES", 128, m_lastFreeEnCryptIdNumber++));
    QString javascript = QString("replaceSelectionWithHtml('%1');").arg(encryptedTextHtmlObject);

    QNTRACE("script: " << javascript);

    // Protect ourselves from the note's html update during the process of changing the selected text to encrypted/decrypted text
    m_pendingConversionToNote = false;

    GET_PAGE()

    m_pendingJavaScriptExecution = true;

#ifndef USE_QT_WEB_ENGINE
    page->executeJavaScript(javascript);
    onSelectedTextEncryptionDone(QVariant(), QVector<QPair<QString,QString> >());
#else
    QVector<QPair<QString,QString> > extraData;
    page->runJavaScript(javascript, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onSelectedTextEncryptionDone, extraData));
#endif
}

void NoteEditorPrivate::raiseEditUrlDialog(const QString & startupText, const QString & startupUrl,
                                           const quint64 idNumber)
{
    QNDEBUG("NoteEditorPrivate::raiseEditUrlDialog: startup text = " << startupText
            << ", startup url = " << startupUrl << ", id number = " << idNumber);

    QScopedPointer<EditUrlDialog> pEditUrlDialog(new EditUrlDialog(this, startupText, startupUrl));
    pEditUrlDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pEditUrlDialog.data(), QNSIGNAL(EditUrlDialog,accepted,QString,QUrl,quint64),
                     this, QNSLOT(NoteEditorPrivate,onUrlEditingFinished,QString,QUrl,quint64));
    QNTRACE("Will exec edit URL dialog now");
    pEditUrlDialog->exec();
    QNTRACE("Executed edit URL dialog");
}

void NoteEditorPrivate::clearEditorContent()
{
    QNDEBUG("NoteEditorPrivate::clearEditorContent");

    if (m_pageToNoteContentPostponeTimerId != 0) {
        killTimer(m_pageToNoteContentPostponeTimerId);
        m_pageToNoteContentPostponeTimerId = 0;
    }

    m_watchingForContentChange = false;
    m_contentChangedSinceWatchingStart = false;

    m_modified = false;

    m_contextMenuSequenceNumber = 1;
    m_lastContextMenuEventGlobalPos = QPoint();
    m_lastContextMenuEventPagePos = QPoint();

    m_lastFreeEnToDoIdNumber = 1;
    m_lastFreeHyperlinkIdNumber = 1;
    m_lastFreeEnCryptIdNumber = 1;
    m_lastFreeEnDecryptedIdNumber = 1;

    m_lastEncryptedText.resize(0);

    QString initialHtml = m_pagePrefix + "<body></body></html>";
    m_writeNoteHtmlToFileRequestId = QUuid::createUuid();
    emit writeNoteHtmlToFile(m_noteEditorPagePath, initialHtml.toLocal8Bit(),
                             m_writeNoteHtmlToFileRequestId);
    m_pendingIndexHtmlWritingToFile = true;
}

void NoteEditorPrivate::noteToEditorContent()
{
    QNDEBUG("NoteEditorPrivate::noteToEditorContent");

    if (!m_pNote) {
        QNDEBUG("No note has been set yet");
        clearEditorContent();
        return;
    }

    if (!m_pNote->hasContent()) {
        QNDEBUG("Note without content was inserted into NoteEditor");
        clearEditorContent();
        return;
    }

    m_htmlCachedMemory.resize(0);

    ENMLConverter::NoteContentToHtmlExtraData extraData;
    bool res = m_enmlConverter.noteContentToHtml(m_pNote->content(), m_htmlCachedMemory,
                                                 m_errorCachedMemory, m_decryptedTextManager,
                                                 extraData);
    if (!res) {
        QNWARNING("Can't convert note's content to HTML: " << m_errorCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearEditorContent();
        return;
    }

    m_lastFreeEnToDoIdNumber = extraData.m_numEnToDoNodes + 1;
    m_lastFreeHyperlinkIdNumber = extraData.m_numHyperlinkNodes + 1;
    m_lastFreeEnCryptIdNumber = extraData.m_numEnCryptNodes + 1;
    m_lastFreeEnDecryptedIdNumber = extraData.m_numEnDecryptedNodes + 1;

    m_lastEncryptedText.resize(0);

    int bodyTagIndex = m_htmlCachedMemory.indexOf("<body>");
    if (bodyTagIndex < 0) {
        m_errorCachedMemory = QT_TR_NOOP("can't find <body> tag in the result of note to HTML conversion");
        QNWARNING(m_errorCachedMemory << ", note content: " << m_pNote->content()
                  << ", html: " << m_htmlCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearEditorContent();
        return;
    }

    m_htmlCachedMemory.replace(0, bodyTagIndex, m_pagePrefix);

    int bodyClosingTagIndex = m_htmlCachedMemory.indexOf("</body>");
    if (bodyClosingTagIndex < 0) {
        m_errorCachedMemory = QT_TR_NOOP("Can't find </body> tag in the result of note to HTML conversion");
        QNWARNING(m_errorCachedMemory << ", note content: " << m_pNote->content()
                  << ", html: " << m_htmlCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearEditorContent();
        return;
    }

    m_htmlCachedMemory.insert(bodyClosingTagIndex + 7, "</html>");

    m_htmlCachedMemory.replace("<br></br>", "</br>");   // Webkit-specific fix

    bool readOnly = false;
    if (m_pNote->hasActive() && !m_pNote->active()) {
        QNDEBUG("Current note is not active, setting it to read-only state");
        setPageEditable(false);
        readOnly = true;
    }
    else if (m_pNotebook->hasRestrictions())
    {
        const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
        if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref()) {
            QNDEBUG("Notebook restrictions forbid the note modification, setting note's content to read-only state");
#ifndef USE_QT_WEB_ENGINE
            QWebPage * page = this->page();
            if (Q_LIKELY(page)) {
                page->setContentEditable(false);
            }
#else
            setPageEditable(false);
#endif
            readOnly = true;
        }
    }

    if (!readOnly) {
        QNDEBUG("Nothing prevents user to modify the note, allowing it in the editor");
        setPageEditable(true);
    }

    m_writeNoteHtmlToFileRequestId = QUuid::createUuid();
    emit writeNoteHtmlToFile(m_noteEditorPagePath, m_htmlCachedMemory.toLocal8Bit(),
                             m_writeNoteHtmlToFileRequestId);
    m_pendingIndexHtmlWritingToFile = true;
}

void NoteEditorPrivate::updateColResizableTableBindings()
{
    QNDEBUG("NoteEditorPrivate::updateColResizableTableBindings");

    bool readOnly = !isPageEditable();

    QString colResizable = "$(\"table\").colResizable({";
    if (readOnly) {
        colResizable += "disable:true});";
    }
    else {
        colResizable += "liveDrag:true, "
                        "gripInnerHtml:\"<div class=\\'grip\\'></div>\", "
                        "draggingClass:\"dragging\", "
                        "postbackSafe:true, "
                        "onResize:onFixedWidthTableResized"
                        "});";
    }

    QNTRACE("colResizable js code: " << colResizable);

    GET_PAGE()
    page->executeJavaScript(colResizable);
}

bool NoteEditorPrivate::htmlToNoteContent(QString & errorDescription)
{
    QNDEBUG("NoteEditorPrivate::htmlToNoteContent");

    if (!m_pNote) {
        errorDescription = QT_TR_NOOP("No note was set to note editor");
        emit cantConvertToNote(errorDescription);
        return false;
    }

    if (m_pNote->hasActive() && !m_pNote->active()) {
        errorDescription = QT_TR_NOOP("Current note is marked as read-only, the changes won't be saved");
        QNINFO(errorDescription << ", note: local guid = " << m_pNote->localGuid()
               << ", guid = " << (m_pNote->hasGuid() ? m_pNote->guid() : "<null>")
               << ", title = " << (m_pNote->hasTitle() ? m_pNote->title() : "<null>"));
        emit cantConvertToNote(errorDescription);
        return false;
    }

    if (m_pNotebook && m_pNotebook->hasRestrictions())
    {
        const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
        if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref()) {
            errorDescription = QT_TR_NOOP("The notebook the current note belongs to doesn't allow notes modification, "
                                          "the changes won't be saved");
            QNINFO(errorDescription << ", note: local guid = " << m_pNote->localGuid()
                   << ", guid = " << (m_pNote->hasGuid() ? m_pNote->guid() : "<null>")
                   << ", title = " << (m_pNote->hasTitle() ? m_pNote->title() : "<null>")
                   << ", notebook: local guid = " << m_pNotebook->localGuid() << ", guid = "
                   << (m_pNotebook->hasGuid() ? m_pNotebook->guid() : "<null>") << ", name = "
                   << (m_pNotebook->hasName() ? m_pNotebook->name() : "<null>"));
            emit cantConvertToNote(errorDescription);
            return false;
        }
    }

    m_pendingConversionToNote = true;

#ifndef USE_QT_WEB_ENGINE
    m_htmlCachedMemory = page()->mainFrame()->toHtml();
    onPageHtmlReceived(m_htmlCachedMemory);
#else
    page()->toHtml(NoteEditorCallbackFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
#endif

    return true;
}

void NoteEditorPrivate::saveNoteResourcesToLocalFiles()
{
    QNDEBUG("NoteEditorPrivate::saveNoteResourcesToLocalFiles");

    if (!m_pNote) {
        QNTRACE("No note is set for the editor");
        return;
    }

    QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
    if (resourceAdapters.isEmpty()) {
        QNTRACE("Note has no resources, nothing to do");
        return;
    }

    auto resourceAdaptersConstBegin = resourceAdapters.constBegin();
    auto resourceAdaptersConstEnd = resourceAdapters.constEnd();

    size_t numPendingResourceWritesToLocalFiles = 0;

    for(auto it = resourceAdaptersConstBegin; it != resourceAdaptersConstEnd; ++it)
    {
        const ResourceAdapter & resourceAdapter = *it;

        if (!resourceAdapter.hasDataBody() && !resourceAdapter.hasAlternateDataBody()) {
            QNINFO("Detected resource without data body: " << resourceAdapter);
            continue;
        }

        if (!resourceAdapter.hasDataHash() && !resourceAdapter.hasAlternateDataHash()) {
            QNINFO("Detected resource without data hash: " << resourceAdapter);
            continue;
        }

        if (!resourceAdapter.hasMime()) {
            QNINFO("Detected resource without mime type: " << resourceAdapter);
            continue;
        }

        const QByteArray & dataBody = (resourceAdapter.hasDataBody()
                                       ? resourceAdapter.dataBody()
                                       : resourceAdapter.alternateDataBody());

        const QByteArray & dataHash = (resourceAdapter.hasDataHash()
                                       ? resourceAdapter.dataHash()
                                       : resourceAdapter.alternateDataHash());

        QString dataHashStr = QString::fromLocal8Bit(dataHash.constData(), dataHash.size());

        QNTRACE("Found current note's resource corresponding to the data hash "
                << dataHashStr << ": " << resourceAdapter);

        if (!m_resourceInfo.contains(dataHashStr))
        {
            const QString resourceLocalGuid = resourceAdapter.localGuid();
            QUuid saveResourceRequestId = QUuid::createUuid();

            QString fileStoragePath;
            if (resourceAdapter.mime().startsWith("image/")) {
                fileStoragePath = m_noteEditorImageResourcesStoragePath;
            }
            else {
                fileStoragePath = m_resourceLocalFileStorageFolder;
            }

            fileStoragePath += "/" + resourceLocalGuid;
            QString preferredFileSuffix = resourceAdapter.preferredFileSuffix();
            if (!preferredFileSuffix.isEmpty()) {
                fileStoragePath += "." + preferredFileSuffix;
            }

            m_genericResourceLocalGuidBySaveToStorageRequestIds[saveResourceRequestId] = resourceLocalGuid;
            emit saveResourceToStorage(resourceLocalGuid, dataBody, dataHash, fileStoragePath, saveResourceRequestId);

            QNTRACE("Sent request to save resource to file storage: request id = " << saveResourceRequestId
                    << ", resource local guid = " << resourceLocalGuid << ", data hash = " << dataHash
                    << ", mime type = " << resourceAdapter.mime());
            ++numPendingResourceWritesToLocalFiles;
        }
    }

    if (numPendingResourceWritesToLocalFiles == 0)
    {
        QNTRACE("All current note's resources are written to local files and are actual. "
                "Will set filepaths to these local files to src attributes of img resource tags");
        if (!m_pendingNotePageLoad) {
            provideSrcForResourceImgTags();
        }
        return;
    }

    QNTRACE("Scheduled writing of " << numPendingResourceWritesToLocalFiles
            << " to local files, will wait until they are written "
            "and add the src attributes to img resources when the files are ready");
}

void NoteEditorPrivate::provideSrcForResourceImgTags()
{
    QNDEBUG("NoteEditorPrivate::provideSrcForResourceImgTags");

    GET_PAGE()
    page->executeJavaScript("provideSrcForResourceImgTags();");
}

void NoteEditorPrivate::manualSaveResourceToFile(const IResource & resource)
{
    QNDEBUG("NoteEditorPrivate::manualSaveResourceToFile");

    if (Q_UNLIKELY(!resource.hasDataBody() && !resource.hasAlternateDataBody())) {
        QString error = QT_TR_NOOP("Can't save resource: resource has neither data body nor alternate data body");
        QNINFO(error << ", resource: " << resource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!resource.hasMime())) {
        QString error = QT_TR_NOOP("Can't save resource: resource has no mime type");
        QNINFO(error << ", resource: " << resource);
        emit notifyError(error);
        return;
    }

    QString resourcePreferredSuffix = resource.preferredFileSuffix();
    QString resourcePreferredFilterString;
    if (!resourcePreferredSuffix.isEmpty()) {
        resourcePreferredFilterString = "(*." + resourcePreferredSuffix + ")";
    }

    const QString & mimeTypeName = resource.mime();

    auto preferredSuffixesIter = m_fileSuffixesForMimeType.find(mimeTypeName);
    auto fileFilterStringIter = m_fileFilterStringForMimeType.find(mimeTypeName);

    if ( (preferredSuffixesIter == m_fileSuffixesForMimeType.end()) ||
         (fileFilterStringIter == m_fileFilterStringForMimeType.end()) )
    {
        QMimeDatabase mimeDatabase;
        QMimeType mimeType = mimeDatabase.mimeTypeForName(mimeTypeName);
        if (Q_UNLIKELY(!mimeType.isValid())) {
            QString error = QT_TR_NOOP("Can't save resource: can't identify resource's mime type");
            QNINFO(error << ", mime type name: " << mimeTypeName);
            emit notifyError(error);
            return;
        }

        bool shouldSkipResourcePreferredSuffix = false;
        QStringList suffixes = mimeType.suffixes();
        if (!resourcePreferredSuffix.isEmpty() && !suffixes.contains(resourcePreferredSuffix))
        {
            const int numSuffixes = suffixes.size();
            for(int i = 0; i < numSuffixes; ++i)
            {
                const QString & currentSuffix = suffixes[i];
                if (resourcePreferredSuffix.contains(currentSuffix)) {
                    shouldSkipResourcePreferredSuffix = true;
                    break;
                }
            }

            if (!shouldSkipResourcePreferredSuffix) {
                suffixes.prepend(resourcePreferredSuffix);
            }
        }

        QString filterString = mimeType.filterString();
        if (!shouldSkipResourcePreferredSuffix && !resourcePreferredFilterString.isEmpty()) {
            filterString += ";;" + resourcePreferredFilterString;
        }

        if (preferredSuffixesIter == m_fileSuffixesForMimeType.end()) {
            preferredSuffixesIter = m_fileSuffixesForMimeType.insert(mimeTypeName, suffixes);
        }

        if (fileFilterStringIter == m_fileFilterStringForMimeType.end()) {
            fileFilterStringIter = m_fileFilterStringForMimeType.insert(mimeTypeName, filterString);
        }
    }

    QString preferredSuffix;
    QString preferredFolderPath;

    const QStringList & preferredSuffixes = preferredSuffixesIter.value();
    if (!preferredSuffixes.isEmpty())
    {
        ApplicationSettings appSettings;
        QStringList childGroups = appSettings.childGroups();
        int attachmentsSaveLocGroupIndex = childGroups.indexOf("AttachmentSaveLocations");
        if (attachmentsSaveLocGroupIndex >= 0)
        {
            QNTRACE("Found cached attachment save location group within application settings");

            appSettings.beginGroup("AttachmentSaveLocations");
            QStringList cachedFileSuffixes = appSettings.childKeys();
            const int numPreferredSuffixes = preferredSuffixes.size();
            for(int i = 0; i < numPreferredSuffixes; ++i)
            {
                preferredSuffix = preferredSuffixes[i];
                int indexInCache = cachedFileSuffixes.indexOf(preferredSuffix);
                if (indexInCache < 0) {
                    QNTRACE("Haven't found cached attachment save directory for file suffix " << preferredSuffix);
                    continue;
                }

                QVariant dirValue = appSettings.value(preferredSuffix);
                if (dirValue.isNull() || !dirValue.isValid()) {
                    QNTRACE("Found inappropriate attachment save directory for file suffix " << preferredSuffix);
                    continue;
                }

                QFileInfo dirInfo(dirValue.toString());
                if (!dirInfo.exists()) {
                    QNTRACE("Cached attachment save directory for file suffix " << preferredSuffix
                            << " does not exist: " << dirInfo.absolutePath());
                    continue;
                }
                else if (!dirInfo.isDir()) {
                    QNTRACE("Cached attachment save directory for file suffix " << preferredSuffix
                            << " is not a directory: " << dirInfo.absolutePath());
                    continue;
                }
                else if (!dirInfo.isWritable()) {
                    QNTRACE("Cached attachment save directory for file suffix " << preferredSuffix
                            << " is not writable: " << dirInfo.absolutePath());
                    continue;
                }

                preferredFolderPath = dirInfo.absolutePath();
                break;
            }

            appSettings.endGroup();
        }
    }

    const QString & filterString = fileFilterStringIter.value();

    QString * pSelectedFilter = (filterString.contains(resourcePreferredFilterString)
                                 ? &resourcePreferredFilterString
                                 : Q_NULLPTR);

    QString absoluteFilePath = QFileDialog::getSaveFileName(this, QObject::tr("Save as..."),
                                                            preferredFolderPath, filterString,
                                                            pSelectedFilter);
    if (absoluteFilePath.isEmpty()) {
        QNINFO("User cancelled saving resource to file");
        return;
    }

    bool foundSuffix = false;
    const int numPreferredSuffixes = preferredSuffixes.size();
    for(int i = 0; i < numPreferredSuffixes; ++i)
    {
        const QString & currentSuffix = preferredSuffixes[i];
        if (absoluteFilePath.endsWith(currentSuffix, Qt::CaseInsensitive)) {
            foundSuffix = true;
            break;
        }
    }

    if (!foundSuffix) {
        absoluteFilePath += "." + preferredSuffix;
    }

    QUuid saveResourceToFileRequestId = QUuid::createUuid();

    const QByteArray & data = (resource.hasDataBody()
                               ? resource.dataBody()
                               : resource.alternateDataBody());

    Q_UNUSED(m_manualSaveResourceToFileRequestIds.insert(saveResourceToFileRequestId));
    emit saveResourceToFile(absoluteFilePath, data, saveResourceToFileRequestId);
    QNDEBUG("Sent request to manually save resource to file, request id = " << saveResourceToFileRequestId
            << ", resource local guid = " << resource.localGuid());
}

void NoteEditorPrivate::openResource(const QString & resourceAbsoluteFilePath)
{
    QNDEBUG("NoteEditorPrivate::openResource: " << resourceAbsoluteFilePath);
    QDesktopServices::openUrl(QUrl("file://" + resourceAbsoluteFilePath));
}

#ifdef USE_QT_WEB_ENGINE
void NoteEditorPrivate::provideSrcAndOnClickScriptForImgEnCryptTags()
{
    QNDEBUG("NoteEditorPrivate::provideSrcAndOnClickScriptForImgEnCryptTags");

    if (!m_pNote) {
        QNTRACE("No note is set for the editor");
        return;
    }

    if (!m_pNote->containsEncryption()) {
        QNTRACE("Current note doesn't contain any encryption, nothing to do");
        return;
    }

    QString iconPath = "qrc:/encrypted_area_icons/en-crypt/en-crypt.png";
    QString javascript = "provideSrcAndOnClickScriptForEnCryptImgTags(\"" + iconPath + "\")";

    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::setupGenericResourceImages()
{
    QNDEBUG("NoteEditorPrivate::setupGenericResourceImages");

    if (!m_pNote) {
        QNDEBUG("No note to build generic resource images for");
        return;
    }

    if (!m_pNote->hasResources()) {
        QNDEBUG("Note has no resources, nothing to do");
        return;
    }

    QString mimeTypeName;
    size_t resourceImagesCounter = 0;
    bool shouldWaitForResourceImagesToSave = false;

    QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
    const int numResources = resourceAdapters.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceAdapter & resourceAdapter = resourceAdapters[i];

        if (resourceAdapter.hasMime())
        {
            mimeTypeName = resourceAdapter.mime();
            if (mimeTypeName.startsWith("image/")) {
                QNTRACE("Skipping image resource " << resourceAdapter);
                continue;
            }
        }

        shouldWaitForResourceImagesToSave |= findOrBuildGenericResourceImage(resourceAdapter);
        ++resourceImagesCounter;
    }

    if (resourceImagesCounter == 0) {
        QNDEBUG("No generic resources requiring building custom images were found");
        return;
    }

    if (shouldWaitForResourceImagesToSave) {
        QNTRACE("Some generic resource images are being saved to files, waiting");
        return;
    }

    QNTRACE("All generic resource images are ready");
    provideSrcForGenericResourceImages();
    setupGenericResourceOnClickHandler();
}

bool NoteEditorPrivate::findOrBuildGenericResourceImage(const IResource & resource)
{
    QNDEBUG("NoteEditorPrivate::findOrBuildGenericResourceImage: " << resource);

    if (!resource.hasDataHash() && !resource.hasAlternateDataHash()) {
        QString errorDescription = QT_TR_NOOP("Found resource without either data hash or alternate data hash");
        QNWARNING(errorDescription << ": " << resource);
        emit notifyError(errorDescription);
        return true;
    }

    const QString localGuid = resource.localGuid();

    const QByteArray & resourceHash = (resource.hasDataHash()
                                       ? resource.dataHash()
                                       : resource.alternateDataHash());

    auto it = m_genericResourceImageFilePathsByResourceHash.find(resourceHash);
    if (it != m_genericResourceImageFilePathsByResourceHash.end()) {
        QNTRACE("Found generic resource image file path for resource with hash " << resourceHash << " and local guid "
                << localGuid << ": " << it.value());
        return false;
    }

    QImage img = buildGenericResourceImage(resource);
    if (img.isNull()) {
        QNDEBUG("Can't build generic resource image");
        return true;
    }

    saveGenericResourceImage(resource, img);
    return true;
}

QImage NoteEditorPrivate::buildGenericResourceImage(const IResource & resource)
{
    QNDEBUG("NoteEditorPrivate::buildGenericResourceImage");

    QString resourceDisplayName = resource.displayName();
    if (Q_UNLIKELY(resourceDisplayName.isEmpty())) {
        resourceDisplayName = QObject::tr("Attachment");
    }

    QFont font = m_font;
    font.setPointSize(10);

    const int maxResourceDisplayNameWidth = 146;
    QFontMetrics fontMetrics(font);
    int width = fontMetrics.width(resourceDisplayName);
    while(width > maxResourceDisplayNameWidth)
    {
        int widthOverflow = width - maxResourceDisplayNameWidth - fontMetrics.width("...");
        int singleCharWidth = fontMetrics.width("n");
        int numCharsToSkip = widthOverflow / singleCharWidth;

        int dotIndex = resourceDisplayName.indexOf(".");
        if (dotIndex != 0 && (dotIndex > resourceDisplayName.size() / 2))
        {
            // Try to shorten the name while preserving the file extension. Need to skip some chars before the dot index
            int startSkipPos = dotIndex - numCharsToSkip;
            if (startSkipPos >= 0) {
                resourceDisplayName.replace(startSkipPos, numCharsToSkip, "...");
                width = fontMetrics.width(resourceDisplayName);
                continue;
            }
        }

        // Either no file extension or name contains a dot, skip some chars without attempt to preserve the file extension
        resourceDisplayName.replace(resourceDisplayName.size() - numCharsToSkip, numCharsToSkip, "...");
        width = fontMetrics.width(resourceDisplayName);
    }

    QNTRACE("(possibly) shortened resource display name: " << resourceDisplayName);

    QString resourceHumanReadableSize;
    if (resource.hasDataSize() || resource.hasAlternateDataSize()) {
        resourceHumanReadableSize = humanReadableSize(resource.hasDataSize() ? resource.dataSize() : resource.alternateDataSize());
    }

    QIcon resourceIcon;
    bool useFallbackGenericResourceIcon = false;

    if (resource.hasMime())
    {
        const QString & resourceMimeTypeName = resource.mime();
        QMimeDatabase mimeDatabase;
        QMimeType mimeType = mimeDatabase.mimeTypeForName(resourceMimeTypeName);
        if (mimeType.isValid())
        {
            resourceIcon = QIcon::fromTheme(mimeType.genericIconName());
            if (resourceIcon.isNull()) {
                QNTRACE("Can't get icon from theme by name " << mimeType.genericIconName());
                useFallbackGenericResourceIcon = true;
            }
        }
        else
        {
            QNTRACE("Can't get valid mime type for name " << resourceMimeTypeName
                    << ", will use fallback generic resource icon");
            useFallbackGenericResourceIcon = true;
        }
    }
    else
    {
        QNINFO("Found resource without mime type set: " << resource);
        QNTRACE("Will use fallback generic resource icon");
        useFallbackGenericResourceIcon = true;
    }

    if (useFallbackGenericResourceIcon) {
        resourceIcon = QIcon(":/generic_resource_icons/png/attachment.png");
    }

    QPixmap pixmap(230, 32);
    pixmap.fill();

    QPainter painter;
    painter.begin(&pixmap);
    painter.setFont(font);

    // Draw resource icon
    painter.drawPixmap(QPoint(2,4), resourceIcon.pixmap(QSize(24,24)));

    // Draw resource display name
    painter.drawText(QPoint(28, 14), resourceDisplayName);

    // Draw resource display size
    painter.drawText(QPoint(28, 28), resourceHumanReadableSize);

    // Draw open resource icon
    QIcon openResourceIcon = QIcon::fromTheme("document-open", QIcon(":/generic_resource_icons/png/open_with.png"));
    painter.drawPixmap(QPoint(174,4), openResourceIcon.pixmap(QSize(24,24)));

    // Draw save resource icon
    QIcon saveResourceIcon = QIcon::fromTheme("document-save", QIcon(":/generic_resource_icons/png/save.png"));
    painter.drawPixmap(QPoint(202,4), saveResourceIcon.pixmap(QSize(24,24)));

    painter.end();
    return pixmap.toImage();
}

void NoteEditorPrivate::saveGenericResourceImage(const IResource & resource, const QImage & image)
{
    QNDEBUG("NoteEditorPrivate::saveGenericResourceImage: resource local guid = " << resource.localGuid());

    if (Q_UNLIKELY(!resource.hasDataHash() && !resource.hasAlternateDataHash())) {
        QString error = QT_TR_NOOP("Can't save generic resource image: resource has neither data hash nor alternate data hash");
        QNWARNING(error << ", resource: " << resource);
        emit notifyError(error);
        return;
    }

    QByteArray imageData;
    QBuffer buffer(&imageData);
    Q_UNUSED(buffer.open(QIODevice::WriteOnly));
    image.save(&buffer, "PNG");

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_saveGenericResourceImageToFileRequestIds.insert(requestId));

    QNDEBUG("Emitting request to write generic resource image for resource with local guid "
            << resource.localGuid() << ", request id " << requestId);
    emit saveGenericResourceImageToFile(resource.localGuid(), imageData, "png",
                                        (resource.hasDataHash() ? resource.dataHash() : resource.alternateDataHash()),
                                        resource.displayName(), requestId);
}

void NoteEditorPrivate::provideSrcForGenericResourceImages()
{
    QNDEBUG("NoteEditorPrivate::provideSrcForGenericResourceImages");

    GET_PAGE()
    page->executeJavaScript("provideSrcForGenericResourceImages();");
}

void NoteEditorPrivate::setupGenericResourceOnClickHandler()
{
    QNDEBUG("NoteEditorPrivate::setupGenericResourceOnClickHandler");

    GET_PAGE()
    page->executeJavaScript("setupGenericResourceOnClickHandler();");
}

void NoteEditorPrivate::setupWebSocketServer()
{
    QNDEBUG("NoteEditorPrivate::setupWebSocketServer");

    if (!m_pWebSocketServer->listen(QHostAddress::LocalHost, 0)) {
        QNFATAL("Cannot open web socket server: " << m_pWebSocketServer->errorString());
        // TODO: throw appropriate exception
        return;
    }

    m_webSocketServerPort = m_pWebSocketServer->serverPort();
    QNDEBUG("Using automatically selected websocket server port " << m_webSocketServerPort);

    QObject::connect(m_pWebSocketClientWrapper, &WebSocketClientWrapper::clientConnected,
                     m_pWebChannel, &QWebChannel::connectTo);
}

void NoteEditorPrivate::setupJavaScriptObjects()
{
    QNDEBUG("NoteEditorPrivate::setupJavaScriptObjects");

    QObject::connect(m_pEnCryptElementClickHandler, &EnCryptElementOnClickHandler::decrypt,
                     this, &NoteEditorPrivate::onEnCryptElementClicked);

    QObject::connect(m_pGenericResourceOpenAndSaveButtonsOnClickHandler,
                     &GenericResourceOpenAndSaveButtonsOnClickHandler::saveResourceRequest,
                     this, &NoteEditorPrivate::onSaveResourceRequest);

    QObject::connect(m_pGenericResourceOpenAndSaveButtonsOnClickHandler,
                     &GenericResourceOpenAndSaveButtonsOnClickHandler::openResourceRequest,
                     this, &NoteEditorPrivate::onOpenResourceRequest);

    QObject::connect(m_pTextCursorPositionJavaScriptHandler,
                     &TextCursorPositionJavaScriptHandler::textCursorPositionChanged,
                     this, &NoteEditorPrivate::onTextCursorPositionChange);

    QObject::connect(m_pContextMenuEventJavaScriptHandler,
                     &ContextMenuEventJavaScriptHandler::contextMenuEventReply,
                     this, &NoteEditorPrivate::onContextMenuEventReply);

    QObject::connect(m_pHyperlinkClickJavaScriptHandler,
                     &HyperlinkClickJavaScriptHandler::hyperlinkClicked,
                     this, &NoteEditorPrivate::onHyperlinkClicked);

    m_pWebChannel->registerObject("resourceCache", m_pResourceInfoJavaScriptHandler);
    m_pWebChannel->registerObject("enCryptElementClickHandler", m_pEnCryptElementClickHandler);
    m_pWebChannel->registerObject("pageMutationObserver", m_pPageMutationHandler);
    m_pWebChannel->registerObject("openAndSaveResourceButtonsHandler", m_pGenericResourceOpenAndSaveButtonsOnClickHandler);
    m_pWebChannel->registerObject("textCursorPositionHandler", m_pTextCursorPositionJavaScriptHandler);
    m_pWebChannel->registerObject("contextMenuEventHandler", m_pContextMenuEventJavaScriptHandler);
    m_pWebChannel->registerObject("genericResourceImageHandler", m_pGenericResoureImageJavaScriptHandler);
    m_pWebChannel->registerObject("hyperlinkClickHandler", m_pHyperlinkClickJavaScriptHandler);
    QNDEBUG("Registered objects exposed to JavaScript");
}

void NoteEditorPrivate::setupTextCursorPositionTracking()
{
    QNDEBUG("NoteEditorPrivate::setupTextCursorPositionTracking");

    QString javascript = "setupTextCursorPositionTracking();";

    GET_PAGE()
    page->executeJavaScript(javascript);
}

#endif

void NoteEditorPrivate::setupGenericTextContextMenu(const QString & selectedHtml, bool insideDecryptedTextFragment)
{
    QNDEBUG("NoteEditorPrivate::setupGenericTextContextMenu: selected html = " << selectedHtml
            << "; inside decrypted text fragment = " << (insideDecryptedTextFragment ? "true" : "false"));

    m_lastSelectedHtml = selectedHtml;

    delete m_pGenericTextContextMenu;
    m_pGenericTextContextMenu = new QMenu(this);

#define ADD_ACTION_WITH_SHORTCUT(key, name, menu, slot, ...) \
    { \
        QAction * action = new QAction(QObject::tr(name), menu); \
        setupActionShortcut(key, QString(#__VA_ARGS__), *action); \
        QObject::connect(action, QNSIGNAL(QAction,triggered), this, QNSLOT(NoteEditorPrivate,slot)); \
        menu->addAction(action); \
    }

    if (!selectedHtml.isEmpty()) {
        ADD_ACTION_WITH_SHORTCUT(QKeySequence::Cut, "Cut", m_pGenericTextContextMenu, cut);
        ADD_ACTION_WITH_SHORTCUT(QKeySequence::Copy, "Copy", m_pGenericTextContextMenu, copy);
    }

    bool clipboardHasHtml = false;
    bool clipboardHasText = false;
    bool clipboardHasImage = false;
    bool clipboardHasUrls = false;

    QClipboard * pClipboard = QApplication::clipboard();
    const QMimeData * pClipboardMimeData = (pClipboard ? pClipboard->mimeData(QClipboard::Clipboard) : Q_NULLPTR);
    if (pClipboardMimeData)
    {
        if (pClipboardMimeData->hasHtml()) {
            clipboardHasHtml = !pClipboardMimeData->html().isEmpty();
        }
        else if (pClipboardMimeData->hasText()) {
            clipboardHasText = !pClipboardMimeData->text().isEmpty();
        }
        else if (pClipboardMimeData->hasImage()) {
            clipboardHasImage = true;
        }
        else if (pClipboardMimeData->hasUrls()) {
            clipboardHasUrls = true;
        }
    }

    if (clipboardHasHtml || clipboardHasText || clipboardHasImage || clipboardHasUrls) {
        QNTRACE("Clipboard buffer has something, adding paste action");
        ADD_ACTION_WITH_SHORTCUT(QKeySequence::Paste, "Paste", m_pGenericTextContextMenu, paste);
    }

    if (clipboardHasHtml) {
        QNTRACE("Clipboard buffer has html, adding paste unformatted action");
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::PasteUnformatted, "Paste as unformatted text",
                                 m_pGenericTextContextMenu, pasteUnformatted);
    }

    Q_UNUSED(m_pGenericTextContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Font, "Font...", m_pGenericTextContextMenu, fontMenu);

    QMenu * pParagraphSubMenu = m_pGenericTextContextMenu->addMenu(QObject::tr("Paragraph"));
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::AlignLeft, "Align left", pParagraphSubMenu, alignLeft);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::AlignCenter, "Center text", pParagraphSubMenu, alignCenter);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::AlignRight, "Align right", pParagraphSubMenu, alignRight);
    Q_UNUSED(pParagraphSubMenu->addSeparator());
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::IncreaseIndentation, "Increase indentation",
                             pParagraphSubMenu, increaseIndentation);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::DecreaseIndentation, "Decrease indentation",
                             pParagraphSubMenu, decreaseIndentation);
    Q_UNUSED(pParagraphSubMenu->addSeparator());

    if (!selectedHtml.isEmpty()) {
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::IncreaseFontSize, "Increase font size",
                                 pParagraphSubMenu, increaseFontSize);
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::DecreaseFontSize, "Decrease font size",
                                 pParagraphSubMenu, decreaseFontSize);
        Q_UNUSED(pParagraphSubMenu->addSeparator());
    }

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertNumberedList, "Numbered list",
                             pParagraphSubMenu, insertNumberedList);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertBulletedList, "Bulleted list",
                             pParagraphSubMenu, insertBulletedList);

    QMenu * pStyleSubMenu = m_pGenericTextContextMenu->addMenu(QObject::tr("Style"));
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Bold, "Bold", pStyleSubMenu, textBold);
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Italic, "Italic", pStyleSubMenu, textItalic);
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Underline, "Underline", pStyleSubMenu, textUnderline);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Strikethrough, "Strikethrough", pStyleSubMenu, textStrikethrough);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Highlight, "Highlight", pStyleSubMenu, textHighlight);

    Q_UNUSED(m_pGenericTextContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertTable, "Insert table...",
                             m_pGenericTextContextMenu, insertTableDialog);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertHorizontalLine, "Insert horizontal line",
                             m_pGenericTextContextMenu, insertHorizontalLine);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::AddAttachment, "Add attachment...",
                             m_pGenericTextContextMenu, addAttachmentDialog);

    Q_UNUSED(m_pGenericTextContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertToDoTag, "Insert ToDo tag",
                             m_pGenericTextContextMenu, insertToDoCheckbox);

    QMenu * pHyperlinkMenu = m_pGenericTextContextMenu->addMenu(QObject::tr("Hyperlink"));
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::EditHyperlink, "Add/edit...", pHyperlinkMenu, editHyperlinkDialog);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::CopyHyperlink, "Copy", pHyperlinkMenu, copyHyperlink);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveHyperlink, "Remove", pHyperlinkMenu, removeHyperlink);

    if (!insideDecryptedTextFragment && !selectedHtml.isEmpty()) {
        Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Encrypt, "Encrypt selected fragment...",
                                 m_pGenericTextContextMenu, encryptSelectedTextDialog);
    }

    m_pGenericTextContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupImageResourceContextMenu(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::setupImageResourceContextMenu: resource hash = " << resourceHash);

    m_currentContextMenuExtraData.m_contentType = "ImageResource";
    m_currentContextMenuExtraData.m_resourceHash = resourceHash;

    delete m_pImageResourceContextMenu;
    m_pImageResourceContextMenu = new QMenu(this);

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::CopyAttachment, "Copy", m_pImageResourceContextMenu,
                             copyAttachmentUnderCursor);

    Q_UNUSED(m_pImageResourceContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::OpenAttachment, "Open", m_pImageResourceContextMenu,
                             openAttachmentUnderCursor);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::SaveAttachment, "Save as...", m_pImageResourceContextMenu,
                             saveAttachmentUnderCursor);

    // TODO: continue filling the menu items

    m_pImageResourceContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupNonImageResourceContextMenu()
{
    QNDEBUG("NoteEditorPrivate::setupNonImageResourceContextMenu");

    delete m_pNonImageResourceContextMenu;
    m_pNonImageResourceContextMenu = new QMenu(this);

    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Cut, "Cut", m_pNonImageResourceContextMenu, cut);
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Copy, "Copy", m_pNonImageResourceContextMenu, copy);

    QClipboard * pClipboard = QApplication::clipboard();
    if (pClipboard && pClipboard->mimeData(QClipboard::Clipboard)) {
        QNTRACE("Clipboard buffer has something, adding paste action");
        ADD_ACTION_WITH_SHORTCUT(QKeySequence::Paste, "Paste", m_pNonImageResourceContextMenu, paste);
    }

    // TODO: continue filling the menu items

    m_pNonImageResourceContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupEncryptedTextContextMenu(const QString & cipher, const QString & keyLength,
                                                      const QString & encryptedText, const QString & hint)
{
    QNDEBUG("NoteEditorPrivate::setupEncryptedTextContextMenu: cipher = " << cipher << ", key length = " << keyLength
            << ", encrypted text = " << encryptedText << ", hint = " << hint);

    m_currentContextMenuExtraData.m_contentType = "EncryptedText";
    m_currentContextMenuExtraData.m_encryptedText = encryptedText;
    m_currentContextMenuExtraData.m_keyLength = keyLength;
    m_currentContextMenuExtraData.m_cipher = cipher;
    m_currentContextMenuExtraData.m_hint = hint;

    delete m_pEncryptedTextContextMenu;
    m_pEncryptedTextContextMenu = new QMenu(this);

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Decrypt, "Decrypt...", m_pEncryptedTextContextMenu, decryptEncryptedTextUnderCursor);

    m_pEncryptedTextContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupActionShortcut(const int key, const QString & context,
                                            QAction & action)
{
    ShortcutManager shortcutManager;
    QKeySequence shortcut = shortcutManager.shortcut(key, context);
    if (!shortcut.isEmpty()) {
        QNTRACE("Setting shortcut " << shortcut << " for action " << action.objectName());
        action.setShortcut(shortcut);
    }
}

void NoteEditorPrivate::setupFileIO()
{
    QNDEBUG("NoteEditorPrivate::setupFileIO");

    m_pIOThread = new QThread;
    QObject::connect(m_pIOThread, QNSIGNAL(QThread,finished), m_pIOThread, QNSLOT(QThread,deleteLater));
    m_pIOThread->start(QThread::LowPriority);

    m_pFileIOThreadWorker = new FileIOThreadWorker;
    m_pFileIOThreadWorker->moveToThread(m_pIOThread);

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,readDroppedFileData,QString,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onReadFileRequest,QString,QUuid));
    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,readFileRequestProcessed,bool,QString,QByteArray,QUuid),
                     this, QNSLOT(NoteEditorPrivate,onDroppedFileRead,bool,QString,QByteArray,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,writeNoteHtmlToFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,writeImageResourceToFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,saveResourceToFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));
    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(NoteEditorPrivate,onWriteFileRequestProcessed,bool,QString,QUuid));

    m_pResourceFileStorageManager = new ResourceFileStorageManager;
    m_pResourceFileStorageManager->moveToThread(m_pIOThread);

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,saveResourceToStorage,QString,QByteArray,QByteArray,QString,QUuid),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onWriteResourceToFileRequest,QString,QByteArray,QByteArray,QString,QUuid));
    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QString),
                     this, QNSLOT(NoteEditorPrivate,onResourceSavedToStorage,QUuid,QByteArray,QString,int,QString));

#ifdef USE_QT_WEB_ENGINE
    m_pGenericResourceImageWriter = new GenericResourceImageWriter;
    m_pGenericResourceImageWriter->setStorageFolderPath(m_noteEditorPageFolderPath + "/GenericResourceImages");
    m_pGenericResourceImageWriter->moveToThread(m_pIOThread);

    QObject::connect(this, &NoteEditorPrivate::saveGenericResourceImageToFile,
                     m_pGenericResourceImageWriter, &GenericResourceImageWriter::onGenericResourceImageWriteRequest);
    QObject::connect(m_pGenericResourceImageWriter, &GenericResourceImageWriter::genericResourceImageWriteReply,
                     this, &NoteEditorPrivate::onGenericResourceImageSaved);
#endif
}

void NoteEditorPrivate::setupScripts()
{
    QNDEBUG("NoteEditorPrivate::setupScripts");

    __initNoteEditorResources();

    QFile file;

#define SETUP_SCRIPT(scriptPathPart, scriptVarName) \
    file.setFileName(":/" scriptPathPart); \
    file.open(QIODevice::ReadOnly); \
    scriptVarName = file.readAll(); \
    file.close()

    SETUP_SCRIPT("javascript/jquery/jquery-2.1.3.min.js", m_jQueryJs);
    SETUP_SCRIPT("javascript/scripts/pageMutationObserver.js", m_pageMutationObserverJs);
    SETUP_SCRIPT("javascript/colResizable/colResizable-1.5.min.js", m_resizableTableColumnsJs);
    SETUP_SCRIPT("javascript/debounce/jquery.debounce-1.0.5.js", m_debounceJs);
    SETUP_SCRIPT("javascript/scripts/onFixedWidthTableResize.js", m_onFixedWidthTableResizeJs);
    SETUP_SCRIPT("javascript/scripts/getSelectionHtml.js", m_getSelectionHtmlJs);
    SETUP_SCRIPT("javascript/scripts/snapSelectionToWord.js", m_snapSelectionToWordJs);
    SETUP_SCRIPT("javascript/scripts/replaceSelectionWithHtml.js", m_replaceSelectionWithHtmlJs);
    SETUP_SCRIPT("javascript/scripts/findSelectedHyperlinkElement.js", m_findSelectedHyperlinkElementJs);
    SETUP_SCRIPT("javascript/scripts/setHyperlinkToSelection.js", m_setHyperlinkToSelectionJs);
    SETUP_SCRIPT("javascript/scripts/getHyperlinkFromSelection.js", m_getHyperlinkFromSelectionJs);
    SETUP_SCRIPT("javascript/scripts/removeHyperlinkFromSelection.js", m_removeHyperlinkFromSelectionJs);
    SETUP_SCRIPT("javascript/scripts/provideSrcForResourceImgTags.js", m_provideSrcForResourceImgTagsJs);
    SETUP_SCRIPT("javascript/scripts/enToDoTagsSetup.js", m_setupEnToDoTagsJs);
    SETUP_SCRIPT("javascript/scripts/flipEnToDoCheckboxState.js", m_flipEnToDoCheckboxStateJs);
    SETUP_SCRIPT("javascript/scripts/onResourceInfoReceived.js", m_onResourceInfoReceivedJs);
    SETUP_SCRIPT("javascript/scripts/determineStatesForCurrentTextCursorPosition.js", m_determineStatesForCurrentTextCursorPositionJs);
    SETUP_SCRIPT("javascript/scripts/determineContextMenuEventTarget.js", m_determineContextMenuEventTargetJs);
    SETUP_SCRIPT("javascript/scripts/changeFontSizeForSelection.js", m_changeFontSizeForSelectionJs);
    SETUP_SCRIPT("javascript/scripts/decryptEncryptedTextPermanently.js", m_decryptEncryptedTextPermanentlyJs);

#ifndef USE_QT_WEB_ENGINE
    SETUP_SCRIPT("javascript/scripts/qWebKitSetup.js", m_qWebKitSetupJs);
#else
    SETUP_SCRIPT("qtwebchannel/qwebchannel.js", m_qWebChannelJs);
    SETUP_SCRIPT("javascript/scripts/qWebChannelSetup.js", m_qWebChannelSetupJs);
    SETUP_SCRIPT("javascript/scripts/provideSrcAndOnClickScriptForEnCryptImgTags.js", m_provideSrcAndOnClickScriptForEnCryptImgTagsJs);
    SETUP_SCRIPT("javascript/scripts/provideSrcForGenericResourceImages.js", m_provideSrcForGenericResourceImagesJs);
    SETUP_SCRIPT("javascript/scripts/onGenericResourceImageReceived.js", m_onGenericResourceImageReceivedJs);
    SETUP_SCRIPT("javascript/scripts/genericResourceOnClickHandler.js", m_genericResourceOnClickHandlerJs);
    SETUP_SCRIPT("javascript/scripts/setupGenericResourceOnClickHandler.js", m_setupGenericResourceOnClickHandlerJs);
    SETUP_SCRIPT("javascript/scripts/clickInterceptor.js", m_clickInterceptorJs);
    SETUP_SCRIPT("javascript/scripts/notifyTextCursorPositionChanged.js", m_notifyTextCursorPositionChangedJs);
    SETUP_SCRIPT("javascript/scripts/setupTextCursorPositionTracking.js", m_setupTextCursorPositionTrackingJs);
#endif

#undef SETUP_SCRIPT
}

void NoteEditorPrivate::setupGeneralSignalSlotConnections()
{
    QNDEBUG("NoteEditorPrivate::setupGeneralSignalSlotConnections");

    QObject::connect(m_pPageMutationHandler, QNSIGNAL(PageMutationHandler,contentsChanged),
                     this, QNSIGNAL(NoteEditorPrivate,contentChanged));
    QObject::connect(m_pPageMutationHandler, QNSIGNAL(PageMutationHandler,contentsChanged),
                     this, QNSLOT(NoteEditorPrivate,onContentChanged));
    QObject::connect(m_pContextMenuEventJavaScriptHandler, QNSIGNAL(ContextMenuEventJavaScriptHandler,contextMenuEventReply,QString,QString,bool,QStringList,quint64),
                     this, QNSLOT(NoteEditorPrivate,onContextMenuEventReply,QString,QString,bool,QStringList,quint64));

    Q_Q(NoteEditor);
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,notifyError,QString), q, QNSIGNAL(NoteEditor,notifyError,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note), q, QNSIGNAL(NoteEditor,convertedToNote,Note));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,cantConvertToNote,QString), q, QNSIGNAL(NoteEditor,cantConvertToNote,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,noteEditorHtmlUpdated,QString), q, QNSIGNAL(NoteEditor,noteEditorHtmlUpdated,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,insertTableDialogRequested), q, QNSIGNAL(NoteEditor,insertTableDialogRequested));
}

void NoteEditorPrivate::setupNoteEditorPage()
{
    QNDEBUG("NoteEditorPrivate::setupNoteEditorPage");

    NoteEditorPage * page = new NoteEditorPage(*this, m_lastNoteEditorPageFreeIndex++);
    QNTRACE("Updated last note editor page free index to " << m_lastNoteEditorPageFreeIndex);

    page->settings()->setAttribute(WebSettings::LocalContentCanAccessFileUrls, true);
    page->settings()->setAttribute(WebSettings::LocalContentCanAccessRemoteUrls, false);

    updateNoteEditorPagePath(page->index());
    QUrl url = QUrl::fromLocalFile(m_noteEditorPagePath);

#ifdef USE_QT_WEB_ENGINE
    page->setUrl(url);
#else
    page->mainFrame()->setUrl(url);

    page->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    page->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    page->setContentEditable(true);

    page->mainFrame()->addToJavaScriptWindowObject("pageMutationObserver", m_pPageMutationHandler,
                                                   QScriptEngine::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject("resourceCache", m_pResourceInfoJavaScriptHandler,
                                                   QScriptEngine::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject("textCursorPositionHandler", m_pTextCursorPositionJavaScriptHandler,
                                                   QScriptEngine::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject("contextMenuEventHandler", m_pContextMenuEventJavaScriptHandler,
                                                   QScriptEngine::QtOwnership);

    m_errorCachedMemory.resize(0);
#endif

    setupNoteEditorPageConnections(page);
    setPage(page);

#ifdef USE_QT_WEB_ENGINE
    QNTRACE("Set note editor page with url: " << page->url());
#else
    EncryptedAreaPlugin * pEncryptedAreaPlugin = new EncryptedAreaPlugin(*this, m_encryptionManager, m_decryptedTextManager);
    m_pluginFactory = new NoteEditorPluginFactory(*this, *m_pResourceFileStorageManager, *m_pFileIOThreadWorker, pEncryptedAreaPlugin, page);
    if (Q_LIKELY(m_pNote)) {
        m_pluginFactory->setNote(*m_pNote);
    }

    page->setPluginFactory(m_pluginFactory);

    QNTRACE("Set note editor page with url: " << page->mainFrame()->url());
#endif
}

void NoteEditorPrivate::setupNoteEditorPageConnections(NoteEditorPage * page)
{
    QNDEBUG("NoteEditorPrivate::setupNoteEditorPageConnections");

    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded), this, QNSLOT(NoteEditorPrivate,onJavaScriptLoaded));
#ifndef USE_QT_WEB_ENGINE
    QObject::connect(page, QNSIGNAL(NoteEditorPage,microFocusChanged), this, QNSLOT(NoteEditorPrivate,onTextCursorPositionChange));

    QWebFrame * frame = page->mainFrame();
    QObject::connect(frame, QNSIGNAL(QWebFrame,loadFinished,bool), this, QNSLOT(NoteEditorPrivate,onNoteLoadFinished,bool));

    QObject::connect(page, QNSIGNAL(QWebPage,linkClicked,QUrl), this, QNSLOT(NoteEditorPrivate,onHyperlinkClicked,QUrl));
#else
    QObject::connect(page, QNSIGNAL(NoteEditorPage,loadFinished,bool), this, QNSLOT(NoteEditorPrivate,onNoteLoadFinished,bool));
#endif
}

void NoteEditorPrivate::setupTextCursorPositionJavaScriptHandlerConnections()
{
    QNDEBUG("NoteEditorPrivate::setupTextCursorPositionJavaScriptHandlerConnections");

    // Connect JavaScript glue object's signals to slots
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionBoldState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorBoldStateChanged,bool));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionItalicState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorItalicStateChanged,bool));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionUnderlineState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorUnderlineStateChanged,bool));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionStrikethroughState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorStrikethgouthStateChanged,bool));

    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionAlignLeftState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorAlignLeftStateChanged,bool));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionAlignCenterState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorAlignCenterStateChanged,bool));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionAlignRightState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorAlignRightStateChanged,bool));

    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionInsideOrderedListState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorInsideOrderedListStateChanged,bool));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionInsideUnorderedListState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorInsideUnorderedListStateChanged,bool));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionInsideTableState,bool),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorInsideTableStateChanged,bool));

    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionOnImageResourceState,bool,QString),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorOnImageResourceStateChanged,bool,QString));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionOnNonImageResourceState,bool,QString),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorOnNonImageResourceStateChanged,bool,QString));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionOnEnCryptTagState,bool,QString,QString,QString),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorOnEnCryptTagStateChanged,bool,QString,QString,QString));

    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionFontName,QString),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorFontNameChanged,QString));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionFontSize,int),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorFontSizeChanged,int));

    // Connect signals to signals of public class
    Q_Q(NoteEditor);

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textBoldState,bool), q, QNSIGNAL(NoteEditor,textBoldState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textItalicState,bool), q, QNSIGNAL(NoteEditor,textItalicState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textUnderlineState,bool), q, QNSIGNAL(NoteEditor,textUnderlineState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textStrikethroughState,bool), q, QNSIGNAL(NoteEditor,textStrikethroughState,bool));

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textAlignLeftState,bool), q, QNSIGNAL(NoteEditor,textAlignLeftState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textAlignCenterState,bool), q, QNSIGNAL(NoteEditor,textAlignCenterState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textAlignRightState,bool), q, QNSIGNAL(NoteEditor,textAlignRightState,bool));

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textInsideOrderedListState,bool), q, QNSIGNAL(NoteEditor,textInsideOrderedListState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textInsideUnorderedListState,bool), q, QNSIGNAL(NoteEditor,textInsideUnorderedListState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textInsideTableState,bool), q, QNSIGNAL(NoteEditor,textInsideTableState,bool));

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textFontFamilyChanged,QString), q, QNSIGNAL(NoteEditor,textFontFamilyChanged,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textFontSizeChanged,int), q, QNSIGNAL(NoteEditor,textFontSizeChanged,int));

}

void NoteEditorPrivate::setupSkipRulesForHtmlToEnmlConversion()
{
    QNDEBUG("NoteEditorPrivate::setupSkipRulesForHtmlToEnmlConversion");

    ENMLConverter::SkipHtmlElementRule skipRule;
    skipRule.m_attributeValueToSkip = "JCLRgrip";
    skipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::StartsWith;
    skipRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;

    m_skipRulesForHtmlToEnmlConversion << skipRule;
}

void NoteEditorPrivate::determineStatesForCurrentTextCursorPosition()
{
    QNDEBUG("NoteEditorPrivate::determineStatesForCurrentTextCursorPosition");

    QString javascript = "determineStatesForCurrentTextCursorPosition();";

    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::determineContextMenuEventTarget()
{
    QNDEBUG("NoteEditorPrivate::determineContextMenuEventTarget");

    QString javascript = "determineContextMenuEventTarget(" + QString::number(m_contextMenuSequenceNumber) +
                         ", " + QString::number(m_lastContextMenuEventPagePos.x()) + ", " +
                         QString::number(m_lastContextMenuEventPagePos.y()) + ");";

    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::setPageEditable(const bool editable)
{
    QNTRACE("NoteEditorPrivate::setPageEditable: " << (editable ? "true" : "false"));

    GET_PAGE()

#ifndef USE_QT_WEB_ENGINE
    page->setContentEditable(editable);
#else
    QString javascript = QString("document.body.contentEditable='") + QString(editable ? "true" : "false") + QString("'; ") +
                         QString("document.designMode='") + QString(editable ? "on" : "off") + QString("'; void 0;");
    page->executeJavaScript(javascript);
    QNINFO("Queued javascript to make page " << (editable ? "editable" : "non-editable") << ": " << javascript);
#endif

    m_isPageEditable = editable;
}

bool NoteEditorPrivate::checkContextMenuSequenceNumber(const quint64 sequenceNumber) const
{
    return m_contextMenuSequenceNumber == sequenceNumber;
}

void NoteEditorPrivate::onPageHtmlReceived(const QString & html,
                                           const QVector<QPair<QString, QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onPageHtmlReceived");
    Q_UNUSED(extraData)

    emit noteEditorHtmlUpdated(html);

    if (!m_pendingConversionToNote) {
        return;
    }

    if (!m_pNote) {
        m_pendingConversionToNote = false;
        m_errorCachedMemory = QT_TR_NOOP("No current note is set to note editor");
        emit cantConvertToNote(m_errorCachedMemory);
        return;
    }

    m_lastSelectedHtml.resize(0);
    m_htmlCachedMemory = html;
    m_enmlCachedMemory.resize(0);
    m_errorCachedMemory.resize(0);
    bool res = m_enmlConverter.htmlToNoteContent(m_htmlCachedMemory, m_enmlCachedMemory,
                                                 m_decryptedTextManager, m_errorCachedMemory,
                                                 m_skipRulesForHtmlToEnmlConversion);
    if (!res)
    {
        m_errorCachedMemory = QT_TR_NOOP("Can't convert note editor page's content to ENML: ") + m_errorCachedMemory;
        QNWARNING(m_errorCachedMemory)
        emit notifyError(m_errorCachedMemory);

        m_pendingConversionToNote = false;
        emit cantConvertToNote(m_errorCachedMemory);

        return;
    }

    m_pNote->setContent(m_enmlCachedMemory);
    m_pendingConversionToNote = false;
    emit convertedToNote(*m_pNote);
}

void NoteEditorPrivate::onSelectedTextEncryptionDone(const QVariant & dummy, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onSelectedTextEncryptionDone");

    Q_UNUSED(dummy);
    Q_UNUSED(extraData);

    m_pendingConversionToNote = true;

#ifndef USE_QT_WEB_ENGINE
    m_htmlCachedMemory = page()->mainFrame()->toHtml();
    onPageHtmlReceived(m_htmlCachedMemory);
#else
    page()->toHtml(NoteEditorCallbackFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif
}

int NoteEditorPrivate::resourceIndexByHash(const QList<ResourceAdapter> & resourceAdapters,
                                           const QString & resourceHash) const
{
    QNDEBUG("NoteEditorPrivate::resourceIndexByHash: hash = " << resourceHash);

    const int numResources = resourceAdapters.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceAdapter & resourceAdapter = resourceAdapters[i];
        if (resourceAdapter.hasDataHash() && (resourceAdapter.dataHash() == resourceHash)) {
            return i;
        }
    }

    return -1;
}

void NoteEditorPrivate::updateNoteEditorPagePath(const quint32 index)
{
    m_noteEditorPagePath = m_noteEditorPageFolderPath + "/index_" + QString::number(index) + ".html";
}

#define COMMAND_TO_JS(command) \
    QString javascript = QString("document.execCommand(\"%1\", false, null)").arg(command)

#define COMMAND_WITH_ARGS_TO_JS(command, args) \
    QString javascript = QString("document.execCommand('%1', false, '%2')").arg(command,args)

#ifndef USE_QT_WEB_ENGINE
QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command)
{
    COMMAND_TO_JS(command);
    QWebFrame * frame = page()->mainFrame();
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript << ", result = " << result.toString());
    return result;
}

QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command, const QString & args)
{
    COMMAND_WITH_ARGS_TO_JS(command, args);
    QWebFrame * frame = page()->mainFrame();
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript << ", result = " << result.toString());
    return result;
}
#endif

void NoteEditorPrivate::execJavascriptCommand(const QString & command)
{
    COMMAND_TO_JS(command);

    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::execJavascriptCommand(const QString & command, const QString & args)
{
    COMMAND_WITH_ARGS_TO_JS(command, args);

    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::setUndoStack(QUndoStack * pUndoStack)
{
    QNDEBUG("NoteEditorPrivate::setUndoStack");

    QUTE_NOTE_CHECK_PTR(pUndoStack, "null undo stack passed to note editor");
    m_pUndoStack = pUndoStack;

    if (Q_UNLIKELY(!m_pPreliminaryUndoCommandQueue)) {
        m_pPreliminaryUndoCommandQueue = new PreliminaryUndoCommandQueue(pUndoStack, this);
    }
    else {
        m_pPreliminaryUndoCommandQueue->setUndoStack(pUndoStack);
    }
}

void NoteEditorPrivate::setNoteAndNotebook(const Note & note, const Notebook & notebook)
{
    QNDEBUG("NoteEditorPrivate::setNoteAndNotebook: note: local guid = " << note.localGuid()
            << ", guid = " << (note.hasGuid() ? note.guid() : "<null>") << ", title: "
            << (note.hasTitle() ? note.title() : "<null>") << "; notebook: local guid = "
            << notebook.localGuid() << ", guid = " << (notebook.hasGuid() ? notebook.guid() : "<null>")
            << ", name = " << (notebook.hasName() ? notebook.name() : "<null>"));

    if (!m_pNotebook) {
        m_pNotebook = new Notebook(notebook);
    }
    else {
        *m_pNotebook = notebook;
    }

    if (!m_pNote)
    {
        m_pNote = new Note(note);
    }
    else
    {
        if ( (m_pNote->localGuid() == note.localGuid()) &&
             (m_pNote->hasContent() == note.hasContent()) &&
             (!m_pNote->hasContent() || (m_pNote->content() == note.content())) )
        {
            QNDEBUG("This note has already been set for the editor and its content hasn't changed");
            return;
        }
        else
        {
            *m_pNote = note;

            m_decryptedTextManager.clearNonRememberedForSessionEntries();
            QNTRACE("Removed non-per-session saved passphrases from decrypted text manager");
        }
    }

    // Clear the caches from previous note
    m_genericResourceLocalGuidBySaveToStorageRequestIds.clear();
    m_resourceFileStoragePathsByResourceLocalGuid.clear();
#ifdef USE_QT_WEB_ENGINE
    m_localGuidsOfResourcesWantedToBeSaved.clear();
    if (m_genericResourceImageFilePathsByResourceHash.size() > 30) {
        m_genericResourceImageFilePathsByResourceHash.clear();
    }
    m_saveGenericResourceImageToFileRequestIds.clear();
#else
    m_pluginFactory->setNote(*m_pNote);
#endif

    noteToEditorContent();

    QNTRACE("Done setting the current note and notebook");
}

void NoteEditorPrivate::convertToNote()
{
    QNDEBUG("NoteEditorPrivate::convertToNote");
    m_pendingConversionToNote = true;

    m_errorCachedMemory.resize(0);
    bool res = htmlToNoteContent(m_errorCachedMemory);
    if (!res) {
        m_pendingConversionToNote = false;
        return;
    }
}

void NoteEditorPrivate::updateFromNote()
{
    QNDEBUG("NoteEditorPrivate::updateFromNote");
    noteToEditorContent();
}

void NoteEditorPrivate::setNoteHtml(const QString & html)
{
    QNDEBUG("NoteEditorPrivate::setNoteHtml");
    m_pendingConversionToNote = true;
    onPageHtmlReceived(html, QVector<QPair<QString,QString> >());
}

void NoteEditorPrivate::addResourceToNote(const ResourceWrapper & resource)
{
    QNDEBUG("NoteEditorPrivate::addResourceToNote");
    QNTRACE(resource);

    if (Q_UNLIKELY(!m_pNote)) {
        m_errorCachedMemory = QT_TR_NOOP("Can't add resource to note: no note is set to the editor");
        QNWARNING(m_errorCachedMemory << ", resource to add: " << resource);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    m_pNote->addResource(resource);
}

void NoteEditorPrivate::removeResourceFromNote(const ResourceWrapper & resource)
{
    QNDEBUG("NoteEditorPrivate::removeResourceFromNote");
    QNTRACE(resource);

    if (Q_UNLIKELY(!m_pNote)) {
        m_errorCachedMemory = QT_TR_NOOP("Can't remove resource from note: no note is set to the editor");
        QNWARNING(m_errorCachedMemory << ", resource to remove: " << resource);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    m_pNote->removeResource(resource);
}

void NoteEditorPrivate::replaceResourceInNote(const ResourceWrapper & resource)
{
    QNDEBUG("NoteEditorPrivate::replaceResourceInNote");
    QNTRACE(resource);

    if (Q_UNLIKELY(!m_pNote)) {
        m_errorCachedMemory = QT_TR_NOOP("Can't replace resource in note: no note is set to the editor");
        QNWARNING(m_errorCachedMemory << ", replacement resource: " << resource);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    if (Q_UNLIKELY(!m_pNote->hasResources())) {
        m_errorCachedMemory = QT_TR_NOOP("Can't replace resource in note: note has no resources");
        QNWARNING(m_errorCachedMemory << ", replacement resource: " << resource);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    QList<ResourceWrapper> resources = m_pNote->resources();
    int resourceIndex = -1;
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceWrapper & currentResource = resources[i];
        if (currentResource.localGuid() == resource.localGuid()) {
            resourceIndex = i;
            break;
        }
    }

    if (Q_UNLIKELY(resourceIndex < 0)) {
        m_errorCachedMemory = QT_TR_NOOP("Can't replace resource in note: can't find resource to be replaced in note");
        QNWARNING(m_errorCachedMemory << ", replacement resource: " << resource);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    resources[resourceIndex] = resource;
}

void NoteEditorPrivate::setNoteResources(const QList<ResourceWrapper> & resources)
{
    QNDEBUG("NoteEditorPrivate::setNoteResources");

    if (Q_UNLIKELY(!m_pNote)) {
        m_errorCachedMemory = QT_TR_NOOP("Can't set resources to the note: no note is set to the editor");
        QNWARNING(m_errorCachedMemory);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    m_pNote->setResources(resources);
}

bool NoteEditorPrivate::isModified() const
{
    return m_modified;
}

#ifndef USE_QT_WEB_ENGINE
const NoteEditorPluginFactory & NoteEditorPrivate::pluginFactory() const
{
    return *m_pluginFactory;
}

NoteEditorPluginFactory & NoteEditorPrivate::pluginFactory()
{
    return *m_pluginFactory;
}
#endif

void NoteEditorPrivate::onDropEvent(QDropEvent * pEvent)
{
    QNDEBUG("NoteEditorPrivate::onDropEvent");

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

QString NoteEditorPrivate::attachResourceToNote(const QByteArray & data, const QByteArray & dataHash,
                                                const QMimeType & mimeType, const QString & filename)
{
    QNDEBUG("NoteEditorPrivate::attachResourceToNote: hash = " << dataHash
            << ", mime type = " << mimeType.name());

    if (!m_pNote) {
        QNINFO("Can't attach resource to note editor: no actual note was selected");
        return QString();
    }

    ResourceWrapper resource;
    resource.setDataBody(data);

    if (!dataHash.isEmpty()) {
        resource.setDataHash(dataHash);
    }

    resource.setDataSize(data.size());
    resource.setMime(mimeType.name());
    resource.setDirty(true);

    if (!filename.isEmpty()) {
        qevercloud::ResourceAttributes attributes;
        attributes.fileName = filename;
        resource.setResourceAttributes(attributes);
    }

    m_pNote->addResource(resource);
    return resource.localGuid();
}

template <typename T>
QString NoteEditorPrivate::composeHtmlTable(const T width, const T singleColumnWidth,
                                            const int rows, const int columns,
                                            const bool relative)
{
    // Table header
    QString htmlTable = "<div><table style=\"border-collapse: collapse; margin-left: 0px; table-layout: fixed; width: ";
    htmlTable += QString::number(width);
    if (relative) {
        htmlTable += "%";
    }
    else {
        htmlTable += "px";
    }
    htmlTable += ";\" ><tbody>";

    for(int i = 0; i < rows; ++i)
    {
        // Row header
        htmlTable += "<tr>";

        for(int j = 0; j < columns; ++j)
        {
            // Column header
            htmlTable += "<td style=\"border: 1px solid rgb(219, 219, 219); padding: 10 px; margin: 0px; width: ";
            htmlTable += QString::number(singleColumnWidth);
            if (relative) {
                htmlTable += "%";
            }
            else {
                htmlTable += "px";
            }
            htmlTable += ";\">";

            // Blank line to preserve the size
            htmlTable += "<div><br></div>";

            // End column
            htmlTable += "</td>";
        }

        // End row
        htmlTable += "</tr>";
    }

    // End table
    htmlTable += "</tbody></table></div>";
    return htmlTable;
}

#define HANDLE_ACTION(name, item) \
    QNDEBUG("NoteEditorPrivate::" #name); \
    GET_PAGE() \
    page->triggerAction(WebPage::item); \
    setFocus()

void NoteEditorPrivate::undo()
{
    QNDEBUG("NoteEditorPrivate::undo");
    m_pUndoStack->undo();
}

void NoteEditorPrivate::redo()
{
    QNDEBUG("NoteEditorPrivate::redo");
    m_pUndoStack->redo();
}

void NoteEditorPrivate::undoPageAction()
{
    skipPushingUndoCommandOnNextContentChange();
    HANDLE_ACTION(undoPageAction, Undo);
}

void NoteEditorPrivate::redoPageAction()
{
    skipPushingUndoCommandOnNextContentChange();
    HANDLE_ACTION(redoPageAction, Redo);
}

void NoteEditorPrivate::flipEnToDoCheckboxState(const quint64 enToDoIdNumber)
{
    QNDEBUG("NoteEditorPrivate::flipEnToDoCheckboxState: " << enToDoIdNumber);

    GET_PAGE()
    QString javascript = QString("flipEnToDoCheckboxState(%1);").arg(QString::number(enToDoIdNumber));
    page->executeJavaScript(javascript);
    setFocus();
}

void NoteEditorPrivate::cut()
{
    HANDLE_ACTION(cut, Cut);
}

void NoteEditorPrivate::copy()
{
    HANDLE_ACTION(copy, Copy);
}

void NoteEditorPrivate::paste()
{
    HANDLE_ACTION(paste, Paste);
}

void NoteEditorPrivate::pasteUnformatted()
{
    HANDLE_ACTION(pasteUnformatted, PasteAndMatchStyle);
}

void NoteEditorPrivate::selectAll()
{
    HANDLE_ACTION(selectAll, SelectAll);
}

void NoteEditorPrivate::fontMenu()
{
    QNDEBUG("NoteEditorPrivate::fontMenu");

    bool fontWasChosen = false;
    QFont chosenFont = QFontDialog::getFont(&fontWasChosen, m_font, this);
    if (!fontWasChosen) {
        setFocus();
        return;
    }

    setFont(chosenFont);

    textBold();
    emit textBoldState(chosenFont.bold());

    textItalic();
    emit textItalicState(chosenFont.italic());

    textUnderline();
    emit textUnderlineState(chosenFont.underline());

    textStrikethrough();
    emit textStrikethroughState(chosenFont.strikeOut());
}

#undef HANDLE_ACTION

#ifndef USE_QT_WEB_ENGINE
#define HANDLE_ACTION(method, item, command) \
    QNDEBUG("NoteEditorPrivate::" #method); \
    GET_PAGE() \
    page->triggerAction(QWebPage::item); \
    setFocus()
#else
#define HANDLE_ACTION(method, item, command) \
    execJavascriptCommand(#command); \
    setFocus()
#endif

void NoteEditorPrivate::textBold()
{
    HANDLE_ACTION(textBold, ToggleBold, bold);
}

void NoteEditorPrivate::textItalic()
{
    HANDLE_ACTION(textItalic, ToggleItalic, italic);
}

void NoteEditorPrivate::textUnderline()
{
    HANDLE_ACTION(textUnderline, ToggleUnderline, underline);
}

void NoteEditorPrivate::textStrikethrough()
{
    HANDLE_ACTION(textStrikethrough, ToggleStrikethrough, strikethrough);
}

void NoteEditorPrivate::textHighlight()
{
    QNDEBUG("NoteEditorPrivate::textHighlight");
    setBackgroundColor(QColor(255, 255, 127));
}

void NoteEditorPrivate::alignLeft()
{
    HANDLE_ACTION(alignLeft, AlignLeft, justifyleft);
}

void NoteEditorPrivate::alignCenter()
{
    HANDLE_ACTION(alignCenter, AlignCenter, justifycenter);
}

void NoteEditorPrivate::alignRight()
{
    HANDLE_ACTION(alignRight, AlignRight, justifyright);
}

#undef HANDLE_ACTION

void NoteEditorPrivate::insertToDoCheckbox()
{
    QNDEBUG("NoteEditorPrivate::insertToDoCheckbox");

    QString html = ENMLConverter::toDoCheckboxHtml(/* checked = */ false, m_lastFreeEnToDoIdNumber++);
    QString javascript = QString("document.execCommand('insertHtml', false, '%1'); ").arg(html);
    javascript += m_setupEnToDoTagsJs;

    GET_PAGE()
    page->executeJavaScript(javascript);
    setFocus();
}

void NoteEditorPrivate::setSpellcheck(bool enabled)
{
    QNDEBUG("stub: NoteEditorPrivate::setSpellcheck: enabled = "
            << (enabled ? "true" : "false"));
    // TODO: implement

    setFocus();
}

void NoteEditorPrivate::setFont(const QFont & font)
{
    QNDEBUG("NoteEditorPrivate::setFont: " << font.family()
            << ", point size = " << font.pointSize());

    if (m_font.family() == font.family()) {
        QNTRACE("Font family hasn't changed, nothing to to do");
        setFocus();
        return;
    }

    m_font = font;
    QString fontName = font.family();
    execJavascriptCommand("fontName", fontName);
    emit textFontFamilyChanged(font.family());
    setFocus();
}

void NoteEditorPrivate::setFontHeight(const int height)
{
    QNDEBUG("NoteEditorPrivate::setFontHeight: " << height);

    if (height > 0) {
        m_font.setPointSize(height);
        GET_PAGE()
        page->executeJavaScript("changeFontSizeForSelection(" + QString::number(height) + ");");
        emit textFontSizeChanged(height);
    }
    else {
        QString error = QT_TR_NOOP("Detected incorrect font size: " + QString::number(height));
        QNINFO(error);
        emit notifyError(error);
    }

    setFocus();
}

void NoteEditorPrivate::setFontColor(const QColor & color)
{
    QNDEBUG("NoteEditorPrivate::setFontColor: " << color.name()
            << ", rgb: " << QString::number(color.rgb(), 16));

    if (color.isValid()) {
        execJavascriptCommand("foreColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid font color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }

    setFocus();
}

void NoteEditorPrivate::setBackgroundColor(const QColor & color)
{
    QNDEBUG("NoteEditorPrivate::setBackgroundColor: " << color.name()
            << ", rgb: " << QString::number(color.rgb(), 16));

    if (color.isValid()) {
        execJavascriptCommand("hiliteColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid background color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }

    setFocus();
}

void NoteEditorPrivate::insertHorizontalLine()
{
    QNDEBUG("NoteEditorPrivate::insertHorizontalLine");

    execJavascriptCommand("insertHorizontalRule");
    setFocus();
}

void NoteEditorPrivate::increaseFontSize()
{
    changeFontSize(/* increase = */ true);
}

void NoteEditorPrivate::decreaseFontSize()
{
    changeFontSize(/* decrease = */ false);
}

void NoteEditorPrivate::increaseIndentation()
{
    changeIndentation(/* increase = */ true);
}

void NoteEditorPrivate::decreaseIndentation()
{
    changeIndentation(/* increase = */ false);
}

void NoteEditorPrivate::insertBulletedList()
{
    QNDEBUG("NoteEditorPrivate::insertBulletedList");

    execJavascriptCommand("insertUnorderedList");
    setFocus();
}

void NoteEditorPrivate::insertNumberedList()
{
    QNDEBUG("NoteEditorPrivate::insertNumberedList");

    execJavascriptCommand("insertOrderedList");
    setFocus();
}

void NoteEditorPrivate::insertTableDialog()
{
    QNDEBUG("NoteEditorPrivate::insertTableDialog");
    emit insertTableDialogRequested();
}

#define CHECK_NUM_COLUMNS() \
    if (columns <= 0) { \
        QString error = QT_TR_NOOP("Detected attempt to insert table with bad number of columns: ") + QString::number(columns); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

#define CHECK_NUM_ROWS() \
    if (rows <= 0) { \
        QString error = QT_TR_NOOP("Detected attempt to insert table with bad number of rows: ") + QString::number(rows); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

void NoteEditorPrivate::insertFixedWidthTable(const int rows, const int columns, const int widthInPixels)
{
    QNDEBUG("NoteEditorPrivate::insertFixedWidthTable: rows = " << rows
            << ", columns = " << columns << ", width in pixels = "
            << widthInPixels);

    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    int pageWidth = geometry().width();
    if (widthInPixels > 2 * pageWidth) {
        QString error = QT_TR_NOOP("Can't insert table, width is too large (more than twice the page width): ") +
                        QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (widthInPixels <= 0) {
        QString error = QT_TR_NOOP("Can't insert table, bad width: ") + QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int singleColumnWidth = widthInPixels / columns;
    if (singleColumnWidth == 0) {
        QString error = QT_TR_NOOP("Can't insert table, bad width for specified number of columns "
                                   "(single column width is zero): width = ") +
                        QString::number(widthInPixels) + QT_TR_NOOP(", number of columns = ") +
                        QString::number(columns);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString htmlTable = composeHtmlTable(widthInPixels, singleColumnWidth, rows, columns,
                                         /* relative = */ false);
    execJavascriptCommand("insertHTML", htmlTable);
    updateColResizableTableBindings();
    setFocus();
}

void NoteEditorPrivate::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    QNDEBUG("NoteEditorPrivate::insertRelativeWidthTable: rows = " << rows
            << ", columns = " << columns << ", relative width = " << relativeWidth);

    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    if (relativeWidth <= 0.01) {
        QString error = QT_TR_NOOP("Can't insert table, relative width is too small: ") +
                        QString::number(relativeWidth) + QString("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (relativeWidth > 100.0 +1.0e-9) {
        QString error = QT_TR_NOOP("Can't insert table, relative width is too large: ") +
                        QString::number(relativeWidth) + QString("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    double singleColumnWidth = relativeWidth / columns;
    QString htmlTable = composeHtmlTable(relativeWidth, singleColumnWidth, rows, columns,
                                         /* relative = */ true);
    execJavascriptCommand("insertHTML", htmlTable);
    updateColResizableTableBindings();
    setFocus();
}

void NoteEditorPrivate::addAttachmentDialog()
{
    QNDEBUG("NoteEditorPrivate::addAttachmentDialog");

    // TODO: implement

    setFocus();
}

void NoteEditorPrivate::saveAttachmentDialog(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::saveAttachmentDialog");
    onSaveResourceRequest(resourceHash);
}

void NoteEditorPrivate::saveAttachmentUnderCursor()
{
    QNDEBUG("NoteEditorPrivate::saveAttachmentUnderCursor");

    if ((m_currentContextMenuExtraData.m_contentType != "ImageResource") &&
        (m_currentContextMenuExtraData.m_contentType != "NonImageResource"))
    {
        QString error = QT_TR_NOOP("Can't save attachment under cursor: wrong current context menu extra data's content type");
        QNWARNING(error << ": content type = " << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    saveAttachmentDialog(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::openAttachment(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::openAttachment");
    onOpenResourceRequest(resourceHash);
}

void NoteEditorPrivate::openAttachmentUnderCursor()
{
    QNDEBUG("NoteEditorPrivate::openAttachmentUnderCursor");

    if ((m_currentContextMenuExtraData.m_contentType != "ImageResource") &&
        (m_currentContextMenuExtraData.m_contentType != "NonImageResource"))
    {
        QString error = QT_TR_NOOP("Can't open attachment under cursor: wrong current context menu extra data's' content type");
        QNWARNING(error << ": content type = " << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    openAttachment(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::copyAttachment(const QString & resourceHash)
{
    if (Q_UNLIKELY(!m_pNote)) {
        QString error = QT_TR_NOOP("Can't copy attachment: no note is set to the editor");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
    int resourceIndex = resourceIndexByHash(resourceAdapters, resourceHash);
    if (Q_UNLIKELY(resourceIndex < 0)) {
        QString error = QT_TR_NOOP("Attachment to be copied was not found in the note");
        QNWARNING(error << ", resource hash = " << resourceHash);
        emit notifyError(error);
        return;
    }

    const ResourceAdapter & resource = resourceAdapters[resourceIndex];

    if (Q_UNLIKELY(!resource.hasDataBody() && !resource.hasAlternateDataBody())) {
        QString error = QT_TR_NOOP("Can't copy attachment as it has neither data body nor alternate data body");
        QNWARNING(error << ", resource hash = " << resourceHash);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!resource.hasMime())) {
        QString error = QT_TR_NOOP("Can't copy attachment as it has no mime type");
        QNWARNING(error << ", resource hash = " << resourceHash);
        emit notifyError(error);
        return;
    }

    const QByteArray & data = (resource.hasDataBody() ? resource.dataBody() : resource.alternateDataBody());
    const QString & mimeType = resource.mime();

    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        QString error = QT_TR_NOOP("Can't copy attachment: can't get access to clipboard");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QMimeData * pMimeData = new QMimeData;
    pMimeData->setData(mimeType, data);
    pClipboard->setMimeData(pMimeData);
}

void NoteEditorPrivate::copyAttachmentUnderCursor()
{
    QNDEBUG("NoteEditorPrivate::copyAttachmentUnderCursor");

    if ((m_currentContextMenuExtraData.m_contentType != "ImageResource") &&
        (m_currentContextMenuExtraData.m_contentType != "NonImageResource"))
    {
        QString error = QT_TR_NOOP("Can't copy attachment under cursor: wrong current context menu extra data's content type");
        QNWARNING(error << ": content type = " << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    copyAttachment(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::encryptSelectedTextDialog()
{
    QNDEBUG("NoteEditorPrivate::encryptSelectedTextDialog");

    if (m_lastSelectedHtml.isEmpty()) {
        QString error = QT_TR_NOOP("Requested encrypt selected text dialog "
                                   "but last selected html is empty");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_lastSelectedHtmlForEncryption = m_lastSelectedHtml;

    GET_PAGE()

    EncryptSelectedTextDelegate * delegate = new EncryptSelectedTextDelegate(*this, page, m_pFileIOThreadWorker);
    QObject::connect(delegate, QNSIGNAL(EncryptSelectedTextDelegate,finished),
                     this, QNSLOT(NoteEditorPrivate,onEncryptSelectedTextDelegateFinished));
    QObject::connect(delegate, QNSIGNAL(EncryptSelectedTextDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onEncryptSelectedTextDelegateError,QString));
    delegate->start();
}

void NoteEditorPrivate::doEncryptSelectedTextDialog(bool *pCancelled)
{
    QNDEBUG("NoteEditorPrivate::doEncryptSelectedTextDialog");

    if (m_lastSelectedHtmlForEncryption.isEmpty()) {
        QString error = QT_TR_NOOP("Requested encrypt selected text dialog but last selected html is empty");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QScopedPointer<EncryptionDialog> pEncryptionDialog(new EncryptionDialog(m_lastSelectedHtmlForEncryption,
                                                                            m_encryptionManager,
                                                                            m_decryptedTextManager, this));
    pEncryptionDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pEncryptionDialog.data(), QNSIGNAL(EncryptionDialog,accepted,QString,QString,QString,QString,size_t,QString,bool),
                     this, QNSLOT(NoteEditorPrivate,onSelectedTextEncryption,QString,QString,QString,QString,size_t,QString,bool));
    QNTRACE("Will exec encryption dialog now");
    int res = pEncryptionDialog->exec();
    if (pCancelled) {
        *pCancelled = (res == QDialog::Rejected);
    }

    m_lastSelectedHtmlForEncryption.resize(0);
    QNTRACE("Executed encryption dialog: " << (res == QDialog::Accepted ? "accepted" : "rejected"));
}

void NoteEditorPrivate::decryptEncryptedTextUnderCursor()
{
    QNDEBUG("NoteEditorPrivate::decryptEncryptedTextUnderCursor");

    if (m_currentContextMenuExtraData.m_contentType != "EncryptedText") {
        QString error = QT_TR_NOOP("Can't decrypt the encrypted text under cursor: wrong current context menu extra data's content type");
        QNWARNING(error << ": content type = " << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    onEnCryptElementClicked(m_currentContextMenuExtraData.m_encryptedText, m_currentContextMenuExtraData.m_cipher,
                            m_currentContextMenuExtraData.m_keyLength, m_currentContextMenuExtraData.m_hint);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::editHyperlinkDialog()
{
    QNDEBUG("NoteEditorPrivate::editHyperlinkDialog");

    QVector<QPair<QString,QString> > extraData;

#ifndef USE_QT_WEB_ENGINE
    Q_UNUSED(extraData);

    if (!page()->hasSelection()) {
        QNDEBUG("Note editor page has no selected text, hence no hyperlink to edit is available");
        raiseEditUrlDialog();
        return;
    }

    QStringList hyperlinkData = page()->mainFrame()->evaluateJavaScript("getHyperlinkFromSelection();").toStringList();
    if (hyperlinkData.size() != 3) {
        QString error = QT_TR_NOOP("Can't edit hyperlink: can't get text, hyperlink and id number from JavaScript");
        QNWARNING(error << "; hyperlink data: " << hyperlinkData.join(","));
        emit notifyError(error);
        return;
    }

    raiseEditUrlDialog(hyperlinkData[0], hyperlinkData[1]);
#else
    GET_PAGE()
    page->runJavaScript("getHyperlinkFromSelection();", NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onFoundHyperlinkToEdit, extraData));
#endif
}

void NoteEditorPrivate::copyHyperlink()
{
    QNDEBUG("NoteEditorPrivate::copyHyperlink");

    QVector<QPair<QString,QString> > extraData;

#ifndef USE_QT_WEB_ENGINE
    if (!page()->hasSelection()) {
        QNDEBUG("Note editor page has no selected text, hence no hyperlink to copy is available");
        return;
    }

    QVariant hyperlink = page()->mainFrame()->evaluateJavaScript("getHyperlinkFromSelection();");
    onFoundHyperlinkToCopy(hyperlink, extraData);
#else
    GET_PAGE()
    page->runJavaScript("getHyperlinkFromSelection();", NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onFoundHyperlinkToCopy, extraData));
#endif
}

void NoteEditorPrivate::removeHyperlink()
{
    QNDEBUG("NoteEditorPrivate::removeHyperlink");

    GET_PAGE()
    page->executeJavaScript("removeHyperlinkFromSelection();");
    setFocus();
}

void NoteEditorPrivate::onEncryptedAreaDecryption(QString cipher, size_t keyLength,
                                                  QString encryptedText, QString passphrase,
                                                  QString decryptedText, bool rememberForSession,
                                                  bool decryptPermanently, bool createDecryptUndoCommand)
{
    QNDEBUG("NoteEditorPrivate::onEncryptedAreaDecryption: encrypted text = " << encryptedText
            << "; remember for session = " << (rememberForSession ? "true" : "false")
            << "; decrypt permanently = " << (decryptPermanently ? "true" : "false")
            << "; create decrypt undo command = " << (createDecryptUndoCommand ? "true" : "false"));

    if (createDecryptUndoCommand) {
        switchEditorPage();
        pushDecryptUndoCommand(cipher, keyLength, encryptedText, decryptedText, passphrase,
                               rememberForSession, decryptPermanently);
    }

    if (decryptPermanently)
    {
        ENMLConverter::escapeString(decryptedText);

        GET_PAGE();
        QString javascript = "decryptEncryptedTextPermanently('" + encryptedText +
                             "', '" + decryptedText + "', 0);";
        QNTRACE("script: " << javascript);
        page->executeJavaScript(javascript);

        if (decryptPermanently) {
            // The default scheme with contentChanged signal seems to be working but its auto-postponing by timer
            // may prevent the note's ENML from updating in time, so enforce the conversion from ENML to note content
            m_errorCachedMemory.resize(0);
            Q_UNUSED(htmlToNoteContent(m_errorCachedMemory));
        }

        return;
    }

    // TODO: optimize this, it can be done way more efficiently
    noteToEditorContent();
}

void NoteEditorPrivate::onSelectedTextEncryption(QString selectedText, QString encryptedText,
                                                 QString passphrase, QString cipher,
                                                 size_t keyLength, QString hint,
                                                 bool rememberForSession)
{
    QNDEBUG("NoteEditorPrivate::onSelectedTextEncryption: "
            << "encrypted text = " << encryptedText << ", hint = " << hint
            << ", remember for session = " << (rememberForSession ? "true" : "false"));

    Q_UNUSED(passphrase)
    Q_UNUSED(cipher)
    Q_UNUSED(keyLength)

    replaceSelectedTextWithEncryptedOrDecryptedText(selectedText, encryptedText, hint, rememberForSession);
}

void NoteEditorPrivate::onNoteLoadCancelled()
{
    stop();
    QNINFO("Note load has been cancelled");
    // TODO: add some overlay widget for NoteEditor to properly indicate visually that the note load has been cancelled
}

void NoteEditorPrivate::onFoundHyperlinkToEdit(const QVariant & hyperlinkData,
                                               const QVector<QPair<QString, QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onFoundHyperlinkToEdit: " << hyperlinkData);
    Q_UNUSED(extraData);

    QStringList hyperlinkDataList = hyperlinkData.toStringList();
    if (hyperlinkDataList.isEmpty()) {
        QNTRACE("Hyperlink data was not found, starting with empty startup text and url");
        raiseEditUrlDialog();
        return;
    }

    if (hyperlinkDataList.size() != 3) {
        m_errorCachedMemory = QT_TR_NOOP("Can't edit hyperlink: can't get text, hyperlink and hyperlink id number from JavaScript");
        QNWARNING(m_errorCachedMemory << "; hyperlink data: " << hyperlinkDataList.join(","));
        emit notifyError(m_errorCachedMemory);
        return;
    }

    bool conversionResult = false;
    quint64 idNumber = hyperlinkDataList[2].toULongLong(&conversionResult);
    if (!conversionResult) {
        m_errorCachedMemory = QT_TR_NOOP("Can't edit hyperlink: can't cinvert hyperlink id number to unsigned int");
        QNWARNING(m_errorCachedMemory << "; hyperlink data: " << hyperlinkDataList.join(","));
        emit notifyError(m_errorCachedMemory);
        return;
    }

    raiseEditUrlDialog(hyperlinkDataList[0], hyperlinkDataList[1], idNumber);
}

void NoteEditorPrivate::onFoundHyperlinkToCopy(const QVariant & hyperlinkData,
                                               const QVector<QPair<QString, QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onFoundHyperlinkToCopy: " << hyperlinkData);
    Q_UNUSED(extraData);

    QStringList hyperlinkDataList = hyperlinkData.toStringList();
    if (hyperlinkDataList.isEmpty()) {
        QNTRACE("Hyperlink data to copy was not found");
        return;
    }

    if (hyperlinkDataList.size() != 2) {
        QString error = QT_TR_NOOP("Can't copy hyperlink: can't get text and hyperlink from JavaScript");
        QNWARNING(error << "; hyperlink data: " << hyperlinkDataList.join(","));
        emit notifyError(error);
        return;
    }

    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        QNWARNING("Unable to get window system clipboard");
    }
    else {
        pClipboard->setText(hyperlinkDataList[1]);
    }
}

void NoteEditorPrivate::onUrlEditingFinished(QString text, QUrl url, quint64 hyperlinkIdNumber)
{
    QNDEBUG("NoteEditorPrivate::onUrlEditingFinished: text = " << text << "; url = "
            << url << ", hyperlink id number = " << hyperlinkIdNumber);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QString urlString = url.toString(QUrl::FullyEncoded);
#else
    QString urlString = url.toString(QUrl::None);
#endif

    if (hyperlinkIdNumber == 0) {
        hyperlinkIdNumber = m_lastFreeHyperlinkIdNumber++;
    }

    GET_PAGE()
    page->executeJavaScript("setHyperlinkToSelection('" + text + "', '" + urlString +
                            "', " + QString::number(hyperlinkIdNumber) + ");");
    setFocus();
}

void NoteEditorPrivate::dropFile(QString & filepath)
{
    QNDEBUG("NoteEditorPrivate::dropFile: " << filepath);

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

    QUuid readDroppedFileRequestId = QUuid::createUuid();
    auto & pair = m_droppedFilePathsAndMimeTypesByReadRequestIds[readDroppedFileRequestId];
    pair.first = fileInfo.filePath();
    pair.second = mimeType;
    emit readDroppedFileData(filepath, readDroppedFileRequestId);
}

} // namespace qute_note

void __initNoteEditorResources()
{
    Q_INIT_RESOURCE(css);
    Q_INIT_RESOURCE(checkbox_icons);
    Q_INIT_RESOURCE(encrypted_area_icons);
    Q_INIT_RESOURCE(generic_resource_icons);
    Q_INIT_RESOURCE(jquery);
    Q_INIT_RESOURCE(colResizable);
    Q_INIT_RESOURCE(debounce);
    Q_INIT_RESOURCE(scripts);

    QNDEBUG("Initialized NoteEditor's resources");
}
