#include "NoteEditor_p.h"
#include "SpellChecker.h"
#include "GenericResourceImageWriter.h"
#include "dialogs/DecryptionDialog.h"
#include "delegates/AddResourceDelegate.h"
#include "delegates/RemoveResourceDelegate.h"
#include "delegates/RenameResourceDelegate.h"
#include "delegates/ImageResourceRotationDelegate.h"
#include "delegates/EncryptSelectedTextDelegate.h"
#include "delegates/DecryptEncryptedTextDelegate.h"
#include "delegates/AddHyperlinkToSelectedTextDelegate.h"
#include "delegates/EditHyperlinkDelegate.h"
#include "delegates/RemoveHyperlinkDelegate.h"
#include "javascript_glue/ResourceInfoJavaScriptHandler.h"
#include "javascript_glue/TextCursorPositionJavaScriptHandler.h"
#include "javascript_glue/ContextMenuEventJavaScriptHandler.h"
#include "javascript_glue/PageMutationHandler.h"
#include "javascript_glue/ToDoCheckboxOnClickHandler.h"
#include "javascript_glue/TableResizeJavaScriptHandler.h"
#include "javascript_glue/SpellCheckerDynamicHelper.h"
#include "undo_stack/NoteEditorContentEditUndoCommand.h"
#include "undo_stack/EncryptUndoCommand.h"
#include "undo_stack/DecryptUndoCommand.h"
#include "undo_stack/HideDecryptedTextUndoCommand.h"
#include "undo_stack/AddHyperlinkUndoCommand.h"
#include "undo_stack/EditHyperlinkUndoCommand.h"
#include "undo_stack/RemoveHyperlinkUndoCommand.h"
#include "undo_stack/ToDoCheckboxUndoCommand.h"
#include "undo_stack/AddResourceUndoCommand.h"
#include "undo_stack/RemoveResourceUndoCommand.h"
#include "undo_stack/RenameResourceUndoCommand.h"
#include "undo_stack/ImageResourceRotationUndoCommand.h"
#include "undo_stack/ReplaceUndoCommand.h"
#include "undo_stack/ReplaceAllUndoCommand.h"
#include "undo_stack/SpellCorrectionUndoCommand.h"
#include "undo_stack/SpellCheckIgnoreWordUndoCommand.h"
#include "undo_stack/SpellCheckAddToUserWordListUndoCommand.h"
#include "undo_stack/TableActionUndoCommand.h"

#ifndef USE_QT_WEB_ENGINE
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
#include <qute_note/utility/ApplicationSettings.h>
#include <qute_note/utility/DesktopServices.h>
#include <QPainter>
#include <QIcon>
#include <QFontMetrics>
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
#include <qute_note/types/ResourceRecognitionIndexItem.h>
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
#include <QDesktopWidget>
#include <QFontDialog>
#include <QFontDatabase>
#include <QCryptographicHash>
#include <QPixmap>
#include <QBuffer>
#include <QImage>
#include <QTransform>

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
    m_onTableResizeJs(),
    m_getSelectionHtmlJs(),
    m_snapSelectionToWordJs(),
    m_replaceSelectionWithHtmlJs(),
    m_replaceHyperlinkContentJs(),
    m_updateResourceHashJs(),
    m_updateImageResourceSrcJs(),
    m_provideSrcForResourceImgTagsJs(),
    m_setupEnToDoTagsJs(),
    m_flipEnToDoCheckboxStateJs(),
    m_onResourceInfoReceivedJs(),
    m_findInnermostElementJs(),
    m_determineStatesForCurrentTextCursorPositionJs(),
    m_determineContextMenuEventTargetJs(),
    m_changeFontSizeForSelectionJs(),
    m_pageMutationObserverJs(),
    m_tableManagerJs(),
    m_resourceManagerJs(),
    m_hyperlinkManagerJs(),
    m_encryptDecryptManagerJs(),
    m_hilitorJs(),
    m_imageAreasHilitorJs(),
    m_findReplaceManagerJs(),
    m_spellCheckerJs(),
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
    m_pHyperlinkClickJavaScriptHandler(new HyperlinkClickJavaScriptHandler(this)),
    m_webSocketServerPort(0),
#endif
    m_pSpellCheckerDynamicHandler(new SpellCheckerDynamicHelper(this)),
    m_pTableResizeJavaScriptHandler(new TableResizeJavaScriptHandler(this)),
    m_pGenericResourceImageWriter(Q_NULLPTR),
    m_pToDoCheckboxClickHandler(new ToDoCheckboxOnClickHandler(this)),
    m_pPageMutationHandler(new PageMutationHandler(this)),
    m_pUndoStack(Q_NULLPTR),
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
    m_decryptedTextManager(new DecryptedTextManager),
    m_enmlConverter(),
#ifndef USE_QT_WEB_ENGINE
    m_pluginFactory(Q_NULLPTR),
#endif
    m_pGenericTextContextMenu(Q_NULLPTR),
    m_pImageResourceContextMenu(Q_NULLPTR),
    m_pNonImageResourceContextMenu(Q_NULLPTR),
    m_pEncryptedTextContextMenu(Q_NULLPTR),
    m_pSpellChecker(Q_NULLPTR),
    m_spellCheckerEnabled(false),
    m_currentNoteMisSpelledWords(),
    m_stringUtils(),
    m_pagePrefix("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
                 "<html><head>"
                 "<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"UTF-8\" />"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-crypt.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/hover.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-decrypted.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-media-generic.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-media-image.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/image-area-hilitor.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-todo.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/link.css\">"
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/misspell.css\">"
                 "<title></title></head>"),
    m_lastSelectedHtml(),
    m_lastSelectedHtmlForEncryption(),
    m_lastSelectedHtmlForHyperlink(),
    m_lastMisSpelledWord(),
    m_lastSearchHighlightedText(),
    m_lastSearchHighlightedTextCaseSensitivity(false),
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
    m_imageResourceSaveToStorageRequestIds(),
    m_resourceFileStoragePathsByResourceLocalGuid(),
    m_localGuidsOfResourcesWantedToBeSaved(),
    m_localGuidsOfResourcesWantedToBeOpened(),
    m_manualSaveResourceToFileRequestIds(),
    m_fileSuffixesForMimeType(),
    m_fileFilterStringForMimeType(),
    m_genericResourceImageFilePathsByResourceHash(),
#ifdef USE_QT_WEB_ENGINE
    m_pGenericResoureImageJavaScriptHandler(new GenericResourceImageJavaScriptHandler(m_genericResourceImageFilePathsByResourceHash, this)),
#endif
    m_saveGenericResourceImageToFileRequestIds(),
    m_recognitionIndicesByResourceHash(),
    m_currentContextMenuExtraData(),
    m_resourceLocalGuidAndFileStoragePathByReadResourceRequestIds(),
    m_lastFreeEnToDoIdNumber(1),
    m_lastFreeHyperlinkIdNumber(1),
    m_lastFreeEnCryptIdNumber(1),
    m_lastFreeEnDecryptedIdNumber(1),
    q_ptr(&noteEditor)
{
    QString initialHtml = m_pagePrefix + "<body></body></html>";
    m_noteEditorPageFolderPath = applicationPersistentStoragePath() + "/NoteEditorPage";
    m_noteEditorPagePath = m_noteEditorPageFolderPath + "/index.html";
    m_noteEditorImageResourcesStoragePath = m_noteEditorPageFolderPath + "/imageResources";

    setupSkipRulesForHtmlToEnmlConversion();
    setupFileIO();

    m_pSpellChecker = new SpellChecker(m_pFileIOThreadWorker, this);

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
    m_pendingIndexHtmlWritingToFile = true;
    emit writeNoteHtmlToFile(m_noteEditorPagePath, initialHtml.toLocal8Bit(),
                             m_writeNoteHtmlToFileRequestId, /* append = */ false);
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

    if (!ok) {
        QNDEBUG("Note page was not loaded successfully");
        return;
    }

    m_pendingNotePageLoad = false;
    m_pendingJavaScriptExecution = true;

    GET_PAGE()
    page->stopJavaScriptAutoExecution();

    page->executeJavaScript(m_jQueryJs);
    page->executeJavaScript(m_getSelectionHtmlJs);
    page->executeJavaScript(m_replaceSelectionWithHtmlJs);
    page->executeJavaScript(m_findReplaceManagerJs);

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
    frame->addToJavaScriptWindowObject("toDoCheckboxClickHandler", m_pToDoCheckboxClickHandler,
                                       QScriptEngine::QtOwnership);
    frame->addToJavaScriptWindowObject("tableResizeHandler", m_pTableResizeJavaScriptHandler,
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

    page->executeJavaScript(m_findInnermostElementJs);
    page->executeJavaScript(m_resizableTableColumnsJs);
    page->executeJavaScript(m_debounceJs);
    page->executeJavaScript(m_onTableResizeJs);
    page->executeJavaScript(m_snapSelectionToWordJs);
    page->executeJavaScript(m_replaceHyperlinkContentJs);
    page->executeJavaScript(m_updateResourceHashJs);
    page->executeJavaScript(m_updateImageResourceSrcJs);
    page->executeJavaScript(m_provideSrcForResourceImgTagsJs);
    page->executeJavaScript(m_setupEnToDoTagsJs);
    page->executeJavaScript(m_flipEnToDoCheckboxStateJs);
    page->executeJavaScript(m_determineStatesForCurrentTextCursorPositionJs);
    page->executeJavaScript(m_determineContextMenuEventTargetJs);
    page->executeJavaScript(m_changeFontSizeForSelectionJs);
    page->executeJavaScript(m_tableManagerJs);
    page->executeJavaScript(m_resourceManagerJs);
    page->executeJavaScript(m_hyperlinkManagerJs);
    page->executeJavaScript(m_encryptDecryptManagerJs);
    page->executeJavaScript(m_hilitorJs);
    page->executeJavaScript(m_imageAreasHilitorJs);
    page->executeJavaScript(m_spellCheckerJs);

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

    if (m_spellCheckerEnabled) {
        applySpellCheck();
    }

    QNTRACE("Sent commands to execute all the page's necessary scripts");
    page->startJavaScriptAutoExecution();
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
        QNWARNING(errorDescription + ", error code = " << errorCode);
        emit notifyError(errorDescription);
        return;
    }

    const QString localGuid = it.value();
    QNDEBUG("Local guid of the resource updated in the local storage: " << localGuid);

    bool shouldCreateLinkToTheResourceFile = false;
    auto imageResourceIt = m_imageResourceSaveToStorageRequestIds.find(requestId);
    shouldCreateLinkToTheResourceFile = (imageResourceIt != m_imageResourceSaveToStorageRequestIds.end());
    if (shouldCreateLinkToTheResourceFile)
    {
        QNDEBUG("The just saved resource is an image for which we need to create a link "
                "in order to fool the web engine's cache to ensure it would reload the image "
                "if its previous version has already been displayed on the page");

        Q_UNUSED(m_imageResourceSaveToStorageRequestIds.erase(imageResourceIt));

        QString linkFileName = createLinkToImageResourceFile(fileStoragePath, localGuid, errorDescription);
        if (linkFileName.isEmpty()) {
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
        }
        else {
            fileStoragePath = linkFileName;
        }
    }

    m_resourceFileStoragePathsByResourceLocalGuid[localGuid] = fileStoragePath;

    QString resourceDisplayName;
    QString resourceDisplaySize;

    QString oldResourceHash;
    if (m_pNote)
    {
        bool shouldUpdateNoteResources = false;
        QList<ResourceWrapper> resources = m_pNote->resources();
        const int numResources = resources.size();
        for(int i = 0; i < numResources; ++i)
        {
            ResourceWrapper & resource = resources[i];

            if (resource.localGuid() != localGuid) {
                continue;
            }

            shouldUpdateNoteResources = true;
            if (resource.hasDataHash()) {
                oldResourceHash = QString::fromLocal8Bit(resource.dataHash());
            }

            resource.setDataHash(dataHash);

            if (resource.hasRecognitionDataBody())
            {
                ResourceRecognitionIndices recoIndices(resource.recognitionDataBody());
                if (!recoIndices.isNull() && recoIndices.isValid()) {
                    m_recognitionIndicesByResourceHash[dataHash] = recoIndices;
                    QNDEBUG("Updated recognition indices for resource with hash " << dataHash);
                }
            }

            resourceDisplayName = resource.displayName();
            if (resource.hasDataSize()) {
                resourceDisplaySize = humanReadableSize(resource.dataSize());
            }

            break;
        }

        if (shouldUpdateNoteResources) {
            m_pNote->setResources(resources);
        }
    }

    QString dataHashStr = QString::fromLocal8Bit(dataHash);

    m_resourceInfo.cacheResourceInfo(dataHashStr, resourceDisplayName,
                                     resourceDisplaySize, fileStoragePath);

    Q_UNUSED(m_genericResourceLocalGuidBySaveToStorageRequestIds.erase(it));

    if (!oldResourceHash.isEmpty() && (oldResourceHash != dataHashStr)) {
        updateHashForResourceTag(oldResourceHash, dataHashStr);
        highlightRecognizedImageAreas(m_lastSearchHighlightedText, m_lastSearchHighlightedTextCaseSensitivity);
    }

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

#ifdef USE_QT_WEB_ENGINE
    setupGenericResourceImages();
#endif
}

void NoteEditorPrivate::onResourceFileChanged(QString resourceLocalGuid, QString fileStoragePath)
{
    QNDEBUG("NoteEditorPrivate::onResourceFileChanged: resource local guid = " << resourceLocalGuid
            << ", file storage path = " << fileStoragePath);

    if (Q_UNLIKELY(!m_pNote)) {
        QNDEBUG("Can't process resource file change: no note is set to the editor");
        return;
    }

    QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
    const int numResources = resourceAdapters.size();
    int targetResourceIndex = -1;
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceAdapter & resourceAdapter = resourceAdapters[i];
        if (resourceAdapter.localGuid() == resourceLocalGuid) {
            targetResourceIndex = i;
            break;
        }
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        QNDEBUG("Can't process resource file change: can't find the resource by local guid within note's resources");
        return;
    }

    QUuid requestId = QUuid::createUuid();
    m_resourceLocalGuidAndFileStoragePathByReadResourceRequestIds[requestId] = QPair<QString,QString>(resourceLocalGuid, fileStoragePath);
    QNTRACE("Emitting request to read the resource file from storage: file storage path = "
            << fileStoragePath << ", local guid = " << resourceLocalGuid << ", request id = " << requestId);
    emit readResourceFromStorage(fileStoragePath, resourceLocalGuid, requestId);
}

void NoteEditorPrivate::onResourceFileReadFromStorage(QUuid requestId, QByteArray data, QByteArray dataHash,
                                                      int errorCode, QString errorDescription)
{
    auto it = m_resourceLocalGuidAndFileStoragePathByReadResourceRequestIds.find(requestId);
    if (it == m_resourceLocalGuidAndFileStoragePathByReadResourceRequestIds.end()) {
        return;
    }

    QNDEBUG("NoteEditorPrivate::onResourceFileReadFromStorage: request id = " << requestId
            << ", data hash = " << dataHash << ", errorCode = " << errorCode << ", error description: "
            << errorDescription);

    auto pair = it.value();
    QString resourceLocalGuid = pair.first;
    QString fileStoragePath = pair.second;

    Q_UNUSED(m_resourceLocalGuidAndFileStoragePathByReadResourceRequestIds.erase(it));

    if (Q_UNLIKELY(errorCode != 0)) {
        errorDescription = QT_TR_NOOP("Can't process the update of the resource file data: can't read the data from file") + QString(": ") + errorDescription;
        QNWARNING(errorDescription << ", resource local guid = " << resourceLocalGuid << ", error code = " << errorCode);
        return;
    }

    if (Q_UNLIKELY(!m_pNote)) {
        QNDEBUG("Can't process the update of the resource file data: no note is set to the editor");
        return;
    }

    QString oldResourceHash;
    QString resourceDisplayName;
    QString resourceDisplaySize;
    QString resourceMimeTypeName;

    QList<ResourceWrapper> resources = m_pNote->resources();
    int targetResourceIndex = -1;
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        ResourceWrapper & resource = resources[i];
        if (resource.localGuid() != resourceLocalGuid) {
            continue;
        }

        if (resource.hasDataHash()) {
            oldResourceHash = QString::fromLocal8Bit(resource.dataHash());
        }

        resource.setDataBody(data);
        resource.setDataHash(dataHash);
        resource.setDataSize(data.size());

        if (Q_LIKELY(resource.hasMime())) {
            resourceMimeTypeName = resource.mime();
        }

        resourceDisplayName = resource.displayName();
        resourceDisplaySize = humanReadableSize(resource.dataSize());

        targetResourceIndex = i;
        break;
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        QNDEBUG("Can't process the update of the resource file data: can't find the corresponding resource within the note's resources");
        return;
    }

    const ResourceWrapper & resource = resources[targetResourceIndex];
    bool updated = m_pNote->updateResource(resource);
    if (Q_UNLIKELY(!updated)) {
        QString errorDescription = QT_TR_NOOP("unexpectedly failed to update the resource within the note");
        QNWARNING(errorDescription << ", resource: " << resource << "\nNote: " << *m_pNote);
        emit notifyError(errorDescription);
        return;
    }

    QString dataHashStr = QString::fromLocal8Bit(dataHash);
    if (!oldResourceHash.isEmpty() && (oldResourceHash != dataHashStr))
    {
        m_resourceInfo.removeResourceInfo(oldResourceHash);

        m_resourceInfo.cacheResourceInfo(dataHashStr, resourceDisplayName,
                                         resourceDisplaySize, fileStoragePath);

        updateHashForResourceTag(oldResourceHash, dataHashStr);
    }

    if (resourceMimeTypeName.startsWith("image/"))
    {
        QString linkFileName = createLinkToImageResourceFile(fileStoragePath, resourceLocalGuid, errorDescription);
        if (linkFileName.isEmpty()) {
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return;
        }

        m_resourceFileStoragePathsByResourceLocalGuid[resourceLocalGuid] = linkFileName;

        m_resourceInfo.cacheResourceInfo(dataHashStr, resourceDisplayName,
                                         resourceDisplaySize, linkFileName);

        if (!m_pendingNotePageLoad) {
            GET_PAGE()
            page->executeJavaScript("updateImageResourceSrc('" + dataHashStr + "', '" + linkFileName + "');");
        }
    }
    else
    {
        QImage image = buildGenericResourceImage(resource);
        saveGenericResourceImage(resource, image);
    }
}

#ifdef USE_QT_WEB_ENGINE
void NoteEditorPrivate::onGenericResourceImageSaved(bool success, QByteArray resourceActualHash,
                                                    QString filePath, QString errorDescription,
                                                    QUuid requestId)
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

void NoteEditorPrivate::onToDoCheckboxClicked(quint64 enToDoCheckboxId)
{
    QNDEBUG("NoteEditorPrivate::onToDoCheckboxClicked: " << enToDoCheckboxId);

    ToDoCheckboxUndoCommand * pCommand = new ToDoCheckboxUndoCommand(enToDoCheckboxId, *this);
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::onToDoCheckboxClickHandlerError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onToDoCheckboxClickHandlerError: " << error);
    emit notifyError(error);
}

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

    m_currentContextMenuExtraData.m_contentType = contentType;
    m_currentContextMenuExtraData.m_insideDecryptedText = insideDecryptedTextFragment;

    if (contentType == "GenericText")
    {
        setupGenericTextContextMenu(extraData, selectedHtml, insideDecryptedTextFragment);
    }
    else if ((contentType == "ImageResource") || (contentType == "NonImageResource"))
    {
        if (Q_UNLIKELY(extraData.empty())) {
            QString error = QT_TR_NOOP("Can't display context menu for resource: "
                                       "extra data from JavaScript is empty");
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        if (Q_UNLIKELY(extraData.size() != 1)) {
            QString error = QT_TR_NOOP("Can't display context menu for resource: "
                                       "extra data from JavaScript has wrong size");
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        const QString & resourceHash = extraData[0];

        if (contentType == "ImageResource") {
            setupImageResourceContextMenu(resourceHash);
        }
        else {
            setupNonImageResourceContextMenu(resourceHash);
        }
    }
    else if (contentType == "EncryptedText")
    {
        QString cipher, keyLength, encryptedText, hint, id, error;
        bool res = parseEncryptedTextContextMenuExtraData(extraData, encryptedText, cipher, keyLength, hint, id, error);
        if (Q_UNLIKELY(!res)) {
            error = QT_TR_NOOP("Can't display context menu for encrypted text: ") + error;
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        setupEncryptedTextContextMenu(cipher, keyLength, encryptedText, hint, id);
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

void NoteEditorPrivate::onAddResourceDelegateFinished(ResourceWrapper addedResource, QString resourceFileStoragePath)
{
    QNDEBUG("NoteEditorPrivate::onAddResourceDelegateFinished: resource file storage path = " << resourceFileStoragePath);

    QNTRACE(addedResource);

    if (Q_UNLIKELY(!addedResource.hasDataHash())) {
        QString error = QT_TR_NOOP("The added resource doesn't contain the data hash");
        QNWARNING(error);
        removeResourceFromNote(addedResource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!addedResource.hasDataSize())) {
        QString error = QT_TR_NOOP("The added resource doesn't contain the data size");
        QNWARNING(error);
        removeResourceFromNote(addedResource);
        emit notifyError(error);
        return;
    }

    m_resourceFileStoragePathsByResourceLocalGuid[addedResource.localGuid()] = resourceFileStoragePath;

    m_resourceInfo.cacheResourceInfo(addedResource.dataHash(), addedResource.displayName(),
                                     humanReadableSize(addedResource.dataSize()), resourceFileStoragePath);

#ifdef USE_QT_WEB_ENGINE
    setupGenericResourceImages();
#endif

    provideSrcForResourceImgTags();

    AddResourceUndoCommand * pCommand = new AddResourceUndoCommand(addedResource,
                                                                   NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onAddResourceUndoRedoFinished),
                                                                   *this);
    m_pUndoStack->push(pCommand);

    AddResourceDelegate * delegate = qobject_cast<AddResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    convertToNote();
}

void NoteEditorPrivate::onAddResourceDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onAddResourceDelegateError: " << error);
    emit notifyError(error);

    AddResourceDelegate * delegate = qobject_cast<AddResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onAddResourceUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onAddResourceUndoRedoFinished: " << data);

    Q_UNUSED(extraData);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of new resource html insertion from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of new resource html insertion from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't insert resource html into the note editor: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::onRemoveResourceDelegateFinished(ResourceWrapper removedResource)
{
    QNDEBUG("onRemoveResourceDelegateFinished: removed resource = " << removedResource);

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onRemoveResourceUndoRedoFinished);
    RemoveResourceUndoCommand * pCommand = new RemoveResourceUndoCommand(removedResource, callback, *this);
    m_pUndoStack->push(pCommand);

    RemoveResourceDelegate * delegate = qobject_cast<RemoveResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRemoveResourceDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onRemoveResourceDelegateError: " << error);
    emit notifyError(error);

    RemoveResourceDelegate * delegate = qobject_cast<RemoveResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRemoveResourceUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onRemoveResourceUndoRedoFinished: " << data);

    Q_UNUSED(extraData)

    if (!m_lastSearchHighlightedText.isEmpty()) {
        highlightRecognizedImageAreas(m_lastSearchHighlightedText, m_lastSearchHighlightedTextCaseSensitivity);
    }
}

void NoteEditorPrivate::onRenameResourceDelegateFinished(QString oldResourceName, QString newResourceName,
                                                         ResourceWrapper resource, bool performingUndo,
                                                         QString newResourceImageFilePath)
{
    QNDEBUG("NoteEditorPrivate::onRenameResourceDelegateFinished: old resource name = " << oldResourceName
            << ", new resource name = " << newResourceName << ", performing undo = "
            << (performingUndo ? "true" : "false") << ", resource: " << resource
            << "\nnew resource image file path = " << newResourceImageFilePath);

#ifndef USE_QT_WEB_ENGINE
    if (m_pluginFactory) {
        m_pluginFactory->updateResource(resource);
    }
#endif

    if (!performingUndo) {
        RenameResourceUndoCommand * pCommand = new RenameResourceUndoCommand(resource, oldResourceName, *this, m_pGenericResourceImageWriter);
        m_pUndoStack->push(pCommand);
    }

    RenameResourceDelegate * delegate = qobject_cast<RenameResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRenameResourceDelegateCancelled()
{
    QNDEBUG("NoteEditorPrivate::onRenameResourceDelegateCancelled");

    RenameResourceDelegate * delegate = qobject_cast<RenameResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRenameResourceDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onRenameResourceDelegateError: " << error);
    emit notifyError(error);

    RenameResourceDelegate * delegate = qobject_cast<RenameResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onImageResourceRotationDelegateFinished(QByteArray resourceDataBefore, QString resourceHashBefore,
                                                                QByteArray resourceRecognitionDataBefore, QByteArray resourceRecognitionDataHashBefore,
                                                                ResourceWrapper resourceAfter, INoteEditorBackend::Rotation::type rotationDirection)
{
    QNDEBUG("NoteEditorPrivate::onImageResourceRotationDelegateFinished: previous resource hash = " << resourceHashBefore
            << ", resource local guid = " << resourceAfter.localGuid() << ", rotation direction = " << rotationDirection);

    ImageResourceRotationUndoCommand * pCommand = new ImageResourceRotationUndoCommand(resourceDataBefore, resourceHashBefore,
                                                                                       resourceRecognitionDataBefore,
                                                                                       resourceRecognitionDataHashBefore,
                                                                                       resourceAfter, rotationDirection, *this);
    m_pUndoStack->push(pCommand);

    ImageResourceRotationDelegate * delegate = qobject_cast<ImageResourceRotationDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    highlightRecognizedImageAreas(m_lastSearchHighlightedText, m_lastSearchHighlightedTextCaseSensitivity);
}

void NoteEditorPrivate::onImageResourceRotationDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onImageResourceRotationDelegateError");
    emit notifyError(error);

    ImageResourceRotationDelegate * delegate = qobject_cast<ImageResourceRotationDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onHideDecryptedTextFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onHideDecryptedTextFinished: " << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of decrypted text hiding from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of decrypted text hiding from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't hide the decrypted text: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

#ifdef USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif

    HideDecryptedTextUndoCommand * pCommand = new HideDecryptedTextUndoCommand(*this, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onHideDecryptedTextUndoRedoFinished));
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::onHideDecryptedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onHideDecryptedTextUndoRedoFinished: " << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of decrypted text hiding undo/redo from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of decrypted text hiding undo/redo from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't undo/redo the decrypted text hiding: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

#ifdef USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif
}

void NoteEditorPrivate::onEncryptSelectedTextDelegateFinished()
{
    QNDEBUG("NoteEditorPrivate::onEncryptSelectedTextDelegateFinished");

    EncryptUndoCommand * pCommand = new EncryptUndoCommand(*this, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onEncryptSelectedTextUndoRedoFinished));
    m_pUndoStack->push(pCommand);

    EncryptSelectedTextDelegate * delegate = qobject_cast<EncryptSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    convertToNote();

#ifdef USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif
}

void NoteEditorPrivate::onEncryptSelectedTextDelegateCancelled()
{
    QNDEBUG("NoteEditorPrivate::onEncryptSelectedTextDelegateCancelled");

    EncryptSelectedTextDelegate * delegate = qobject_cast<EncryptSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onEncryptSelectedTextDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onEncryptSelectedTextDelegateError: " << error);
    emit notifyError(error);

    EncryptSelectedTextDelegate * delegate = qobject_cast<EncryptSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onEncryptSelectedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onEncryptSelectedTextUndoRedoFinished: " << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of encryption undo/redo from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of encryption undo/redo from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't undo/redo the selected text encryption: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();

#ifdef USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif
}

void NoteEditorPrivate::onDecryptEncryptedTextDelegateFinished(QString encryptedText, QString cipher, size_t length, QString hint,
                                                               QString decryptedText, QString passphrase, bool rememberForSession,
                                                               bool decryptPermanently)
{
    QNDEBUG("NoteEditorPrivate::onDecryptEncryptedTextDelegateFinished");

    EncryptDecryptUndoCommandInfo info;
    info.m_encryptedText = encryptedText;
    info.m_decryptedText = decryptedText;
    info.m_passphrase = passphrase;
    info.m_cipher = cipher;
    info.m_hint = hint;
    info.m_keyLength = length;
    info.m_rememberForSession = rememberForSession;
    info.m_decryptPermanently = decryptPermanently;

    QVector<QPair<QString, QString> > extraData;
    extraData << QPair<QString, QString>("decryptPermanently", (decryptPermanently ? "true" : "false"));

    DecryptUndoCommand * pCommand = new DecryptUndoCommand(info, m_decryptedTextManager, *this,
                                                           NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onDecryptEncryptedTextUndoRedoFinished, extraData));
    m_pUndoStack->push(pCommand);

    DecryptEncryptedTextDelegate * delegate = qobject_cast<DecryptEncryptedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    if (decryptPermanently) {
        convertToNote();
    }
}

void NoteEditorPrivate::onDecryptEncryptedTextDelegateCancelled()
{
    QNDEBUG("NoteEditorPrivate::onDecryptEncryptedTextDelegateCancelled");

    DecryptEncryptedTextDelegate * delegate = qobject_cast<DecryptEncryptedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onDecryptEncryptedTextDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onDecryptEncryptedTextDelegateError: " << error);

    emit notifyError(error);

    DecryptEncryptedTextDelegate * delegate = qobject_cast<DecryptEncryptedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onDecryptEncryptedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onDecryptEncryptedTextUndoRedoFinished: " << data);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of encrypted text decryption undo/redo from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of encrypted text decryption undo/redo from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't undo/redo the encrypted text decryption: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool shouldConvertToNote = true;
    if (!extraData.isEmpty())
    {
        const QPair<QString, QString> & pair = extraData[0];
        if (pair.second == "false") {
            shouldConvertToNote = false;
        }
    }

    if (shouldConvertToNote) {
        convertToNote();
    }
}

void NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateFinished()
{
    QNDEBUG("NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateFinished");

    AddHyperlinkUndoCommand * pCommand = new AddHyperlinkUndoCommand(*this, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onAddHyperlinkToSelectedTextUndoRedoFinished));
    m_pUndoStack->push(pCommand);

    AddHyperlinkToSelectedTextDelegate * delegate = qobject_cast<AddHyperlinkToSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    convertToNote();
}

void NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateCancelled()
{
    QNDEBUG("NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateCancelled");

    AddHyperlinkToSelectedTextDelegate * delegate = qobject_cast<AddHyperlinkToSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateError");
    emit notifyError(error);

    AddHyperlinkToSelectedTextDelegate * delegate = qobject_cast<AddHyperlinkToSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onAddHyperlinkToSelectedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onAddHyperlinkToSelectedTextUndoRedoFinished: " << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of hyperlink addition undo/redo from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of hyperlink addition undo/redo from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't undo/redo the hyperlink addition: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::onEditHyperlinkDelegateFinished()
{
    QNDEBUG("NoteEditorPrivate::onEditHyperlinkDelegateFinished");

    EditHyperlinkUndoCommand * pCommand = new EditHyperlinkUndoCommand(*this, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onEditHyperlinkUndoRedoFinished));
    m_pUndoStack->push(pCommand);

    EditHyperlinkDelegate * delegate = qobject_cast<EditHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    convertToNote();
}

void NoteEditorPrivate::onEditHyperlinkDelegateCancelled()
{
    QNDEBUG("NoteEditorPrivate::onEditHyperlinkDelegateCancelled");

    EditHyperlinkDelegate * delegate = qobject_cast<EditHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onEditHyperlinkDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onEditHyperlinkDelegateError: " << error);
    emit notifyError(error);

    EditHyperlinkDelegate * delegate = qobject_cast<EditHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onEditHyperlinkUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onEditHyperlinkUndoRedoFinished: " << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of hyperlink edit undo/redo from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of hyperlink edit undo/redo from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't undo/redo the hyperlink edit: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::onRemoveHyperlinkDelegateFinished()
{
    QNDEBUG("NoteEditorPrivate::onRemoveHyperlinkDelegateFinished");

    RemoveHyperlinkUndoCommand * pCommand = new RemoveHyperlinkUndoCommand(*this, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onRemoveHyperlinkUndoRedoFinished));
    m_pUndoStack->push(pCommand);

    RemoveHyperlinkDelegate * delegate = qobject_cast<RemoveHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    convertToNote();
}

void NoteEditorPrivate::onRemoveHyperlinkDelegateError(QString error)
{
    QNDEBUG("NoteEditorPrivate::onRemoveHyperlinkDelegateError: " << error);
    emit notifyError(error);

    RemoveHyperlinkDelegate * delegate = qobject_cast<RemoveHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRemoveHyperlinkUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onRemoveHyperlinkUndoRedoFinished: " << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of hyperlink removal undo/redo from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of hyperlink removal undo/redo from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't undo/redo the hyperlink removal: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::timerEvent(QTimerEvent * pEvent)
{
    QNDEBUG("NoteEditorPrivate::timerEvent: " << (pEvent ? QString::number(pEvent->timerId()) : "<null>"));

    if (Q_UNLIKELY(!pEvent)) {
        QNINFO("Detected null pointer to timer event");
        return;
    }

    if (pEvent->timerId() == m_pageToNoteContentPostponeTimerId)
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

void NoteEditorPrivate::dragMoveEvent(QDragMoveEvent * pEvent)
{
    if (Q_UNLIKELY(!pEvent)) {
        QNINFO("Detected null pointer to drag move event");
        return;
    }

    const QMimeData * pMimeData = pEvent->mimeData();
    if (Q_UNLIKELY(!pMimeData)) {
        QNWARNING("Null pointer to mime data from drag move event was detected");
        return;
    }

    QList<QUrl> urls = pMimeData->urls();
    if (urls.isEmpty()) {
        return;
    }

    pEvent->acceptProposedAction();
}

void NoteEditorPrivate::dropEvent(QDropEvent * pEvent)
{
    onDropEvent(pEvent);
}

void NoteEditorPrivate::pushNoteContentEditUndoCommand()
{
    QNDEBUG("NoteEditorPrivate::pushNoteTextEditUndoCommand");

    QUTE_NOTE_CHECK_PTR(m_pUndoStack, "Undo stack for note editor wasn't initialized");

    if (Q_UNLIKELY(!m_pNote)) {
        QNINFO("Ignoring the content changed signal as the note pointer is null");
        return;
    }

    QList<ResourceWrapper> resources;
    if (m_pNote->hasResources()) {
        resources = m_pNote->resources();
    }

    m_pUndoStack->push(new NoteEditorContentEditUndoCommand(*this, resources));
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

void NoteEditorPrivate::findText(const QString & textToFind, const bool matchCase, const bool searchBackward,
                                 NoteEditorPage::Callback callback) const
{
    QNDEBUG("NoteEditorPrivate::findText: " << textToFind << "; match case = " << (matchCase ? "true" : "false")
            << ", search backward = " << (searchBackward ? "true" : "false"));

    GET_PAGE()

#ifdef USE_QT_WEB_ENGINE
    QString escapedTextToFind = textToFind;
    ENMLConverter::escapeString(escapedTextToFind);

    // The order of used parameters to window.find: text to find, match case (bool), search backwards (bool), wrap the search around (bool)
    QString javascript = "window.find('" + escapedTextToFind + "', " + (matchCase ? "true" : "false") + ", " +
                         (searchBackward ? "true" : "false") + ", true);";

    page->executeJavaScript(javascript, callback);
#else
    WebPage::FindFlags flags;
    flags |= QWebPage::FindWrapsAroundDocument;

    if (matchCase) {
        flags |= WebPage::FindCaseSensitively;
    }

    if (searchBackward) {
        flags |= WebPage::FindBackward;
    }

    bool res = page->findText(textToFind, flags);
    if (callback != 0) {
        callback(QVariant(res));
    }
#endif

    setSearchHighlight(textToFind, matchCase);
}

bool NoteEditorPrivate::searchHighlightEnabled() const
{
    return !m_lastSearchHighlightedText.isEmpty();
}

void NoteEditorPrivate::setSearchHighlight(const QString & textToFind, const bool matchCase, const bool force) const
{
    QNDEBUG("NoteEditorPrivate::setSearchHighlight: " << textToFind << "; match case = " << (matchCase ? "true" : "false")
            << "; force = " << (force ? "true" : "false"));

    if ( !force && (textToFind.compare(m_lastSearchHighlightedText, (matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive)) == 0) &&
         (m_lastSearchHighlightedTextCaseSensitivity == matchCase) )
    {
        QNTRACE("The text to find matches the one highlighted the last time as well as its case sensitivity");
        return;
    }

    m_lastSearchHighlightedText = textToFind;
    m_lastSearchHighlightedTextCaseSensitivity = matchCase;

    QString escapedTextToFind = textToFind;
    ENMLConverter::escapeString(escapedTextToFind, /* simplify = */ false);

    GET_PAGE()
    page->executeJavaScript("findReplaceManager.setSearchHighlight('" + escapedTextToFind + "', " +
                            (matchCase ? "true" : "false") + ");");

    highlightRecognizedImageAreas(textToFind, matchCase);
}

void NoteEditorPrivate::highlightRecognizedImageAreas(const QString & textToFind, const bool matchCase) const
{
    QNDEBUG("NoteEditorPrivate::highlightRecognizedImageAreas");

    GET_PAGE()
    page->executeJavaScript("imageAreasHilitor.clearImageHilitors();");

    if (m_lastSearchHighlightedText.isEmpty()) {
        QNTRACE("Last search highlighted text is empty");
        return;
    }

    QString escapedTextToFind = m_lastSearchHighlightedText;
    ENMLConverter::escapeString(escapedTextToFind);

    if (escapedTextToFind.isEmpty()) {
        QNTRACE("Escaped search highlighted text is empty");
        return;
    }

    for(auto it = m_recognitionIndicesByResourceHash.begin(), end = m_recognitionIndicesByResourceHash.end(); it != end; ++it)
    {
        const QByteArray & resourceHash = it.key();
        const ResourceRecognitionIndices & recoIndices = it.value();
        QNTRACE("Processing recognition data for resource hash " << resourceHash);

        QVector<ResourceRecognitionIndexItem> recoIndexItems = recoIndices.items();
        const int numIndexItems = recoIndexItems.size();
        for(int j = 0; j < numIndexItems; ++j)
        {
            const ResourceRecognitionIndexItem & recoIndexItem = recoIndexItems[j];
            QVector<ResourceRecognitionIndexItem::TextItem> textItems = recoIndexItem.textItems();
            const int numTextItems = textItems.size();

            bool matchFound = false;
            for(int k = 0; k < numTextItems; ++k)
            {
                const ResourceRecognitionIndexItem::TextItem & textItem = textItems[k];
                if (textItem.m_text.contains(textToFind, (matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive))) {
                    QNTRACE("Found text item matching with the text to find: " << textItem.m_text);
                    matchFound = true;
                }
            }

            if (matchFound) {
                page->executeJavaScript("imageAreasHilitor.hiliteImageArea('" + resourceHash + "', " +
                                        QString::number(recoIndexItem.x()) + ", " + QString::number(recoIndexItem.y()) + ", " +
                                        QString::number(recoIndexItem.h()) + ", " + QString::number(recoIndexItem.w()) + ");");
            }
        }
    }
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

    m_lastSearchHighlightedText.resize(0);
    m_lastSearchHighlightedTextCaseSensitivity = false;

    QString initialHtml = m_pagePrefix + "<body></body></html>";
    m_writeNoteHtmlToFileRequestId = QUuid::createUuid();
    m_pendingIndexHtmlWritingToFile = true;
    emit writeNoteHtmlToFile(m_noteEditorPagePath, initialHtml.toLocal8Bit(),
                             m_writeNoteHtmlToFileRequestId, /* append = */ false);
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
                                                 m_errorCachedMemory, *m_decryptedTextManager,
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
    m_pendingIndexHtmlWritingToFile = true;
    emit writeNoteHtmlToFile(m_noteEditorPagePath, m_htmlCachedMemory.toLocal8Bit(),
                             m_writeNoteHtmlToFileRequestId, /* append = */ false);
    QNTRACE("Emitted the request to write the note html to file: id = " << m_writeNoteHtmlToFileRequestId);
}

void NoteEditorPrivate::updateColResizableTableBindings()
{
    QNDEBUG("NoteEditorPrivate::updateColResizableTableBindings");

    bool readOnly = !isPageEditable();

    QString javascript;
    if (readOnly) {
        javascript = "tableManager.disableColumnHandles(\"table\");";
    }
    else {
        javascript = "tableManager.updateColumnHandles(\"table\");";
    }

    GET_PAGE()
    page->executeJavaScript(javascript);
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

void NoteEditorPrivate::updateHashForResourceTag(const QString & oldResourceHash, const QString & newResourceHash)
{
    QNDEBUG("NoteEditorPrivate::updateHashForResourceTag");

    GET_PAGE()
    page->executeJavaScript("updateResourceHash('" + oldResourceHash + "', '" + newResourceHash + "');");
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

    QString absoluteFilePath = QFileDialog::getSaveFileName(this, tr("Save as..."),
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
    emit saveResourceToFile(absoluteFilePath, data, saveResourceToFileRequestId, /* append = */ false);
    QNDEBUG("Sent request to manually save resource to file, request id = " << saveResourceToFileRequestId
            << ", resource local guid = " << resource.localGuid());
}

void NoteEditorPrivate::openResource(const QString & resourceAbsoluteFilePath)
{
    QNDEBUG("NoteEditorPrivate::openResource: " << resourceAbsoluteFilePath);

    QFile resourceFile(resourceAbsoluteFilePath);
    if (Q_UNLIKELY(!resourceFile.exists())) {
        QString errorDescription = QT_TR_NOOP("Can't open attachment: the attachment file does not exist");
        QNWARNING(errorDescription << ", supposed file path: " << resourceAbsoluteFilePath);
        emit notifyError(errorDescription);
        return;
    }

    QString symLinkTarget = resourceFile.symLinkTarget();
    if (!symLinkTarget.isEmpty()) {
        QNDEBUG("The resource file is actually a symlink, substituting the file path to open to the real file: " << symLinkTarget);
        emit openResourceFile(symLinkTarget);
    }
    else {
        emit openResourceFile(resourceAbsoluteFilePath);
    }
}

QImage NoteEditorPrivate::buildGenericResourceImage(const IResource & resource)
{
    QNDEBUG("NoteEditorPrivate::buildGenericResourceImage");

    QString resourceDisplayName = resource.displayName();
    if (Q_UNLIKELY(resourceDisplayName.isEmpty())) {
        resourceDisplayName = tr("Attachment");
    }

    QNTRACE("Resource display name = " << resourceDisplayName);

    QFont font = m_font;
    font.setPointSize(10);

    QString originalResourceDisplayName = resourceDisplayName;

    const int maxResourceDisplayNameWidth = 146;
    QFontMetrics fontMetrics(font);
    int width = fontMetrics.width(resourceDisplayName);
    int singleCharWidth = fontMetrics.width("n");
    int ellipsisWidth = fontMetrics.width("...");

    bool smartReplaceWorked = true;
    int previousWidth = width + 1;

    while(width > maxResourceDisplayNameWidth)
    {
        if (width >= previousWidth) {
            smartReplaceWorked = false;
            break;
        }

        previousWidth = width;

        int widthOverflow = width - maxResourceDisplayNameWidth;
        int numCharsToSkip = (widthOverflow + ellipsisWidth) / singleCharWidth + 1;

        int dotIndex = resourceDisplayName.lastIndexOf(".");
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

    if (!smartReplaceWorked)
    {
        QNTRACE("Wasn't able to shorten the resource name nicely, will try to shorten it just somehow");
        width = fontMetrics.width(originalResourceDisplayName);
        int widthOverflow = width - maxResourceDisplayNameWidth;
        int numCharsToSkip = (widthOverflow + ellipsisWidth) / singleCharWidth + 1;
        resourceDisplayName = originalResourceDisplayName;

        if (resourceDisplayName.size() > numCharsToSkip) {
            resourceDisplayName.replace(resourceDisplayName.size() - numCharsToSkip, numCharsToSkip, "...");
        }
        else {
            resourceDisplayName = "Attachment...";
        }
    }

    QNTRACE("(possibly) shortened resource display name: " << resourceDisplayName
            << ", width = " << fontMetrics.width(resourceDisplayName));

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

    QNTRACE("Looking for existing generic resource image file for resource with hash " << resourceHash);
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
                     this, &NoteEditorPrivate::decryptEncryptedText);

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
    m_pWebChannel->registerObject("toDoCheckboxClickHandler", m_pToDoCheckboxClickHandler);
    m_pWebChannel->registerObject("tableResizeHandler", m_pTableResizeJavaScriptHandler);
    m_pWebChannel->registerObject("spellCheckerDynamicHelper", m_pSpellCheckerDynamicHandler);
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

void NoteEditorPrivate::updateResource(const QString & resourceLocalGuid, const QString & previousResourceHash,
                                       ResourceWrapper updatedResource, const QString & resourceFileStoragePath)
{
    QNDEBUG("NoteEditorPrivate::updateResource: resource local guid = " << resourceLocalGuid << ", previous hash = "
            << previousResourceHash << ", updated resource: " << updatedResource << "\nResource file storage path = "
            << resourceFileStoragePath);

    if (Q_UNLIKELY(!m_pNote)) {
        QString error = QT_TR_NOOP("Can't update resource: no note is set to the editor");
        QNWARNING(error << ", updated resource: " << updatedResource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!updatedResource.hasDataBody())) {
        QString error = QT_TR_NOOP("Can't update resource: the updated resource contains no data body");
        QNWARNING(error << ", updated resource: " << updatedResource);
        emit notifyError(error);
        return;
    }

    QByteArray key = previousResourceHash.toLocal8Bit();

    // NOTE: intentionally set the "wrong", "stale" hash value here, it is required for proper update procedure
    // once the resource is saved in the local file storage; it's kinda hacky but it seems the simplest option
    updatedResource.setDataHash(key);

    bool res = m_pNote->updateResource(updatedResource);
    if (Q_UNLIKELY(!res)) {
        QString error = QT_TR_NOOP("Can't update resource: resource to be updated was not found in the note");
        QNWARNING(error << ", updated resource: " << updatedResource << "\nNote: " << *m_pNote);
        emit notifyError(error);
        return;
    }

    m_resourceInfo.removeResourceInfo(previousResourceHash);

    auto recoIt = m_recognitionIndicesByResourceHash.find(key);
    if (recoIt != m_recognitionIndicesByResourceHash.end()) {
        Q_UNUSED(m_recognitionIndicesByResourceHash.erase(recoIt));
    }

    QUuid saveResourceRequestId = QUuid::createUuid();
    m_genericResourceLocalGuidBySaveToStorageRequestIds[saveResourceRequestId] = updatedResource.localGuid();
    Q_UNUSED(m_imageResourceSaveToStorageRequestIds.insert(saveResourceRequestId))
    emit saveResourceToStorage(updatedResource.localGuid(), updatedResource.dataBody(), QByteArray(), resourceFileStoragePath, saveResourceRequestId);
}

void NoteEditorPrivate::setupGenericTextContextMenu(const QStringList & extraData, const QString & selectedHtml, bool insideDecryptedTextFragment)
{
    QNDEBUG("NoteEditorPrivate::setupGenericTextContextMenu: selected html = " << selectedHtml
            << "; inside decrypted text fragment = " << (insideDecryptedTextFragment ? "true" : "false"));

    m_lastSelectedHtml = selectedHtml;

    delete m_pGenericTextContextMenu;
    m_pGenericTextContextMenu = new QMenu(this);

#define ADD_ACTION_WITH_SHORTCUT(key, name, menu, slot, ...) \
    { \
        QAction * action = new QAction(tr(name), menu); \
        setupActionShortcut(key, QString(#__VA_ARGS__), *action); \
        QObject::connect(action, QNSIGNAL(QAction,triggered), this, QNSLOT(NoteEditorPrivate,slot)); \
        menu->addAction(action); \
    }

    // See if extraData contains the misspelled word
    QString misSpelledWord;
    const int extraDataSize = extraData.size();
    for(int i = 0; i < extraDataSize; ++i)
    {
        const QString & item = extraData[i];
        if (!item.startsWith("MisSpelledWord_")) {
            continue;
        }

        misSpelledWord = item.mid(15);
    }

    if (!misSpelledWord.isEmpty())
    {
        m_lastMisSpelledWord = misSpelledWord;

        QStringList correctionSuggestions = m_pSpellChecker->spellCorrectionSuggestions(misSpelledWord);
        if (!correctionSuggestions.isEmpty())
        {
            const int numCorrectionSuggestions = correctionSuggestions.size();
            for(int i = 0; i < numCorrectionSuggestions; ++i)
            {
                const QString & correctionSuggestion = correctionSuggestions[i];
                if (Q_UNLIKELY(correctionSuggestion.isEmpty())) {
                    continue;
                }

                QAction * action = new QAction(correctionSuggestion, m_pGenericTextContextMenu);
                action->setText(correctionSuggestion);
                action->setToolTip(tr("Correct the misspelled word"));
                QObject::connect(action, QNSIGNAL(QAction,triggered), this, QNSLOT(NoteEditorPrivate,onSpellCheckCorrectionAction));
                m_pGenericTextContextMenu->addAction(action);
            }

            Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
            ADD_ACTION_WITH_SHORTCUT(ShortcutManager::SpellCheckIgnoreWord, "Ignore word",
                                     m_pGenericTextContextMenu, onSpellCheckIgnoreWordAction);
            ADD_ACTION_WITH_SHORTCUT(ShortcutManager::SpellCheckAddWordToUserDictionary, "Add word to user dictionary",
                                     m_pGenericTextContextMenu, onSpellCheckAddWordToUserDictionaryAction);
            Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
        }
    }

    if (insideDecryptedTextFragment)
    {
        QString cipher, keyLength, encryptedText, hint, id, error;
        bool res = parseEncryptedTextContextMenuExtraData(extraData, encryptedText, cipher, keyLength, hint, id, error);
        if (Q_UNLIKELY(!res)) {
            error = QT_TR_NOOP("Can't display context menu for encrypted text: ") + error;
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        m_currentContextMenuExtraData.m_encryptedText = encryptedText;
        m_currentContextMenuExtraData.m_keyLength = keyLength;
        m_currentContextMenuExtraData.m_cipher = cipher;
        m_currentContextMenuExtraData.m_hint = hint;
        m_currentContextMenuExtraData.m_id = id;
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

    QMenu * pParagraphSubMenu = m_pGenericTextContextMenu->addMenu(tr("Paragraph"));
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

    QMenu * pStyleSubMenu = m_pGenericTextContextMenu->addMenu(tr("Style"));
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Bold, "Bold", pStyleSubMenu, textBold);
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Italic, "Italic", pStyleSubMenu, textItalic);
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Underline, "Underline", pStyleSubMenu, textUnderline);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Strikethrough, "Strikethrough", pStyleSubMenu, textStrikethrough);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Highlight, "Highlight", pStyleSubMenu, textHighlight);

    Q_UNUSED(m_pGenericTextContextMenu->addSeparator());

    if (extraData.contains("InsideTable")) {
        QMenu * pTableMenu = m_pGenericTextContextMenu->addMenu(tr("Table"));
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertRow, "Insert row", pTableMenu, insertTableRow);
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertColumn, "Insert column", pTableMenu, insertTableColumn);
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveRow, "Remove row", pTableMenu, removeTableRow);
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveColumn, "Remove column", pTableMenu, removeTableColumn);
        Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
    }
    else {
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertTable, "Insert table...",
                                 m_pGenericTextContextMenu, insertTableDialog);
    }

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertHorizontalLine, "Insert horizontal line",
                             m_pGenericTextContextMenu, insertHorizontalLine);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::AddAttachment, "Add attachment...",
                             m_pGenericTextContextMenu, addAttachmentDialog);

    Q_UNUSED(m_pGenericTextContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertToDoTag, "Insert ToDo tag",
                             m_pGenericTextContextMenu, insertToDoCheckbox);

    QMenu * pHyperlinkMenu = m_pGenericTextContextMenu->addMenu(tr("Hyperlink"));
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::EditHyperlink, "Add/edit...", pHyperlinkMenu, editHyperlinkDialog);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::CopyHyperlink, "Copy", pHyperlinkMenu, copyHyperlink);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveHyperlink, "Remove", pHyperlinkMenu, removeHyperlink);

    if (!insideDecryptedTextFragment && !selectedHtml.isEmpty()) {
        Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Encrypt, "Encrypt selected fragment...",
                                 m_pGenericTextContextMenu, encryptSelectedText);
    }
    else if (insideDecryptedTextFragment) {
        Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Encrypt, "Encrypt back",
                                 m_pGenericTextContextMenu, hideDecryptedTextUnderCursor);
    }

    m_pGenericTextContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupImageResourceContextMenu(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::setupImageResourceContextMenu: resource hash = " << resourceHash);

    m_currentContextMenuExtraData.m_resourceHash = resourceHash;

    delete m_pImageResourceContextMenu;
    m_pImageResourceContextMenu = new QMenu(this);

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::CopyAttachment, "Copy", m_pImageResourceContextMenu,
                             copyAttachmentUnderCursor);

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveAttachment, "Remove", m_pImageResourceContextMenu,
                             removeAttachmentUnderCursor);

    Q_UNUSED(m_pImageResourceContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::ImageRotateClockwise, "Rotate clockwise", m_pImageResourceContextMenu,
                             rotateImageAttachmentUnderCursorClockwise);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::ImageRotateCounterClockwise, "Rotate countercloskwise", m_pImageResourceContextMenu,
                             rotateImageAttachmentUnderCursorCounterclockwise);

    Q_UNUSED(m_pImageResourceContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::OpenAttachment, "Open", m_pImageResourceContextMenu,
                             openAttachmentUnderCursor);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::SaveAttachment, "Save as...", m_pImageResourceContextMenu,
                             saveAttachmentUnderCursor);

    m_pImageResourceContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupNonImageResourceContextMenu(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::setupNonImageResourceContextMenu");

    m_currentContextMenuExtraData.m_resourceHash = resourceHash;

    delete m_pNonImageResourceContextMenu;
    m_pNonImageResourceContextMenu = new QMenu(this);

    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Cut, "Cut", m_pNonImageResourceContextMenu, cut);
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Copy, "Copy", m_pNonImageResourceContextMenu, copy);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveAttachment, "Remove", m_pNonImageResourceContextMenu, removeAttachmentUnderCursor);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RenameAttachment, "Rename", m_pNonImageResourceContextMenu, renameAttachmentUnderCursor);

    QClipboard * pClipboard = QApplication::clipboard();
    if (pClipboard && pClipboard->mimeData(QClipboard::Clipboard)) {
        QNTRACE("Clipboard buffer has something, adding paste action");
        ADD_ACTION_WITH_SHORTCUT(QKeySequence::Paste, "Paste", m_pNonImageResourceContextMenu, paste);
    }

    m_pNonImageResourceContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupEncryptedTextContextMenu(const QString & cipher, const QString & keyLength,
                                                      const QString & encryptedText, const QString & hint,
                                                      const QString & id)
{
    QNDEBUG("NoteEditorPrivate::setupEncryptedTextContextMenu: cipher = " << cipher << ", key length = " << keyLength
            << ", encrypted text = " << encryptedText << ", hint = " << hint << ", en-crypt-id = " << id);

    m_currentContextMenuExtraData.m_encryptedText = encryptedText;
    m_currentContextMenuExtraData.m_keyLength = keyLength;
    m_currentContextMenuExtraData.m_cipher = cipher;
    m_currentContextMenuExtraData.m_hint = hint;
    m_currentContextMenuExtraData.m_id = id;

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

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,writeNoteHtmlToFile,QString,QByteArray,QUuid,bool),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,saveResourceToFile,QString,QByteArray,QUuid,bool),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));
    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(NoteEditorPrivate,onWriteFileRequestProcessed,bool,QString,QUuid));

    m_pResourceFileStorageManager = new ResourceFileStorageManager;
    m_pResourceFileStorageManager->moveToThread(m_pIOThread);

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,currentNoteChanged,Note),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onCurrentNoteChanged,Note));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,readResourceFromStorage,QString,QString,QUuid),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onReadResourceFromFileRequest,QString,QString,QUuid));
    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,readResourceFromFileCompleted,QUuid,QByteArray,QByteArray,int,QString),
                     this, QNSLOT(NoteEditorPrivate,onResourceFileReadFromStorage,QUuid,QByteArray,QByteArray,int,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,openResourceFile,QString),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onOpenResourceRequest,QString));
    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,resourceFileChanged,QString,QString),
                     this, QNSLOT(NoteEditorPrivate,onResourceFileChanged,QString,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,saveResourceToStorage,QString,QByteArray,QByteArray,QString,QUuid),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onWriteResourceToFileRequest,QString,QByteArray,QByteArray,QString,QUuid));
    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QString),
                     this, QNSLOT(NoteEditorPrivate,onResourceSavedToStorage,QUuid,QByteArray,QString,int,QString));

    m_pGenericResourceImageWriter = new GenericResourceImageWriter;
    m_pGenericResourceImageWriter->setStorageFolderPath(m_noteEditorPageFolderPath + "/GenericResourceImages");
    m_pGenericResourceImageWriter->moveToThread(m_pIOThread);

#ifdef USE_QT_WEB_ENGINE
    QObject::connect(this,
                     QNSIGNAL(NoteEditorPrivate,saveGenericResourceImageToFile,QString,QByteArray,QString,QByteArray,QString,QUuid),
                     m_pGenericResourceImageWriter,
                     QNSLOT(GenericResourceImageWriter,onGenericResourceImageWriteRequest,QString,QByteArray,QString,QByteArray,QString,QUuid));
    QObject::connect(m_pGenericResourceImageWriter,
                     QNSIGNAL(GenericResourceImageWriter,genericResourceImageWriteReply,bool,QByteArray,QString,QString,QUuid),
                     this, QNSLOT(NoteEditorPrivate,onGenericResourceImageSaved,bool,QByteArray,QString,QString,QUuid));
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
    SETUP_SCRIPT("javascript/hilitor/hilitor-utf8.js", m_hilitorJs);
    SETUP_SCRIPT("javascript/scripts/imageAreasHilitor.js", m_imageAreasHilitorJs);
    SETUP_SCRIPT("javascript/scripts/onTableResize.js", m_onTableResizeJs);
    SETUP_SCRIPT("javascript/scripts/getSelectionHtml.js", m_getSelectionHtmlJs);
    SETUP_SCRIPT("javascript/scripts/snapSelectionToWord.js", m_snapSelectionToWordJs);
    SETUP_SCRIPT("javascript/scripts/replaceSelectionWithHtml.js", m_replaceSelectionWithHtmlJs);
    SETUP_SCRIPT("javascript/scripts/findReplaceManager.js", m_findReplaceManagerJs);
    SETUP_SCRIPT("javascript/scripts/spellChecker.js", m_spellCheckerJs);
    SETUP_SCRIPT("javascript/scripts/replaceHyperlinkContent.js", m_replaceHyperlinkContentJs);
    SETUP_SCRIPT("javascript/scripts/updateResourceHash.js", m_updateResourceHashJs);
    SETUP_SCRIPT("javascript/scripts/updateImageResourceSrc.js", m_updateImageResourceSrcJs);
    SETUP_SCRIPT("javascript/scripts/provideSrcForResourceImgTags.js", m_provideSrcForResourceImgTagsJs);
    SETUP_SCRIPT("javascript/scripts/onResourceInfoReceived.js", m_onResourceInfoReceivedJs);
    SETUP_SCRIPT("javascript/scripts/findInnermostElement.js", m_findInnermostElementJs);
    SETUP_SCRIPT("javascript/scripts/determineStatesForCurrentTextCursorPosition.js", m_determineStatesForCurrentTextCursorPositionJs);
    SETUP_SCRIPT("javascript/scripts/determineContextMenuEventTarget.js", m_determineContextMenuEventTargetJs);
    SETUP_SCRIPT("javascript/scripts/changeFontSizeForSelection.js", m_changeFontSizeForSelectionJs);
    SETUP_SCRIPT("javascript/scripts/tableManager.js", m_tableManagerJs);
    SETUP_SCRIPT("javascript/scripts/resourceManager.js", m_resourceManagerJs);
    SETUP_SCRIPT("javascript/scripts/hyperlinkManager.js", m_hyperlinkManagerJs);
    SETUP_SCRIPT("javascript/scripts/encryptDecryptManager.js", m_encryptDecryptManagerJs);

#ifndef USE_QT_WEB_ENGINE
    SETUP_SCRIPT("javascript/scripts/qWebKitSetup.js", m_qWebKitSetupJs);
#else
    SETUP_SCRIPT("qtwebchannel/qwebchannel.js", m_qWebChannelJs);
    SETUP_SCRIPT("javascript/scripts/qWebChannelSetup.js", m_qWebChannelSetupJs);
#endif

    SETUP_SCRIPT("javascript/scripts/enToDoTagsSetup.js", m_setupEnToDoTagsJs);
    SETUP_SCRIPT("javascript/scripts/flipEnToDoCheckboxState.js", m_flipEnToDoCheckboxStateJs);
#ifdef USE_QT_WEB_ENGINE
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

    QObject::connect(m_pTableResizeJavaScriptHandler, QNSIGNAL(TableResizeJavaScriptHandler,tableResized),
                     this, QNSLOT(NoteEditorPrivate,onTableResized));
    QObject::connect(m_pSpellCheckerDynamicHandler, QNSIGNAL(SpellCheckerDynamicHelper,lastEnteredWords,QStringList),
                     this, QNSLOT(NoteEditorPrivate,onSpellCheckerDynamicHelperUpdate,QStringList));
    QObject::connect(m_pToDoCheckboxClickHandler, QNSIGNAL(ToDoCheckboxOnClickHandler,toDoCheckboxClicked,quint64),
                     this, QNSLOT(NoteEditorPrivate,onToDoCheckboxClicked,quint64));
    QObject::connect(m_pToDoCheckboxClickHandler, QNSIGNAL(ToDoCheckboxOnClickHandler,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onToDoCheckboxClickHandlerError,QString));
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

    NoteEditorPage * page = new NoteEditorPage(*this);

    page->settings()->setAttribute(WebSettings::LocalContentCanAccessFileUrls, true);
    page->settings()->setAttribute(WebSettings::LocalContentCanAccessRemoteUrls, false);

#ifndef USE_QT_WEB_ENGINE
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
    page->mainFrame()->addToJavaScriptWindowObject("toDoCheckboxClickHandler", m_pToDoCheckboxClickHandler,
                                                   QScriptEngine::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject("tableResizeHandler", m_pTableResizeJavaScriptHandler,
                                                   QScriptEngine::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject("spellCheckerDynamicHelper", m_pSpellCheckerDynamicHandler,
                                                   QScriptEngine::QtOwnership);

    m_pluginFactory = new NoteEditorPluginFactory(*this, *m_pResourceFileStorageManager, *m_pFileIOThreadWorker, page);
    if (Q_LIKELY(m_pNote)) {
        m_pluginFactory->setNote(*m_pNote);
    }

    page->setPluginFactory(m_pluginFactory);
#endif

    setupNoteEditorPageConnections(page);
    setPage(page);

    QNTRACE("Done setting up new note editor page");
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

    m_skipRulesForHtmlToEnmlConversion.reserve(4);

    ENMLConverter::SkipHtmlElementRule tableSkipRule;
    tableSkipRule.m_attributeValueToSkip = "JCLRgrip";
    tableSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::StartsWith;
    tableSkipRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;
    m_skipRulesForHtmlToEnmlConversion << tableSkipRule;

    ENMLConverter::SkipHtmlElementRule hilitorSkipRule;
    hilitorSkipRule.m_includeElementContents = true;
    hilitorSkipRule.m_attributeValueToSkip = "hilitorHelper";
    hilitorSkipRule.m_attributeValueCaseSensitivity = Qt::CaseInsensitive;
    hilitorSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::Contains;
    m_skipRulesForHtmlToEnmlConversion << hilitorSkipRule;

    ENMLConverter::SkipHtmlElementRule imageAreaHilitorSkipRule;
    imageAreaHilitorSkipRule.m_includeElementContents = false;
    imageAreaHilitorSkipRule.m_attributeValueToSkip = "image-area-hilitor";
    imageAreaHilitorSkipRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;
    imageAreaHilitorSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::Contains;
    m_skipRulesForHtmlToEnmlConversion << imageAreaHilitorSkipRule;

    ENMLConverter::SkipHtmlElementRule spellCheckerHelperSkipRule;
    spellCheckerHelperSkipRule.m_includeElementContents = true;
    spellCheckerHelperSkipRule.m_attributeValueToSkip = "misspell";
    spellCheckerHelperSkipRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;
    spellCheckerHelperSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::Contains;
    m_skipRulesForHtmlToEnmlConversion << spellCheckerHelperSkipRule;
}

void NoteEditorPrivate::determineStatesForCurrentTextCursorPosition()
{
    QNDEBUG("NoteEditorPrivate::determineStatesForCurrentTextCursorPosition");

    QString javascript = "if (typeof window[\"determineStatesForCurrentTextCursorPosition\"] !== 'undefined')"
                         "{ determineStatesForCurrentTextCursorPosition(); }";

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
    QNDEBUG("NoteEditorPrivate::setPageEditable: " << (editable ? "true" : "false"));

    GET_PAGE()

#ifndef USE_QT_WEB_ENGINE
    page->setContentEditable(editable);
#else
    QString javascript = QString("document.body.contentEditable='") + QString(editable ? "true" : "false") + QString("'; ") +
                         QString("document.designMode='") + QString(editable ? "on" : "off") + QString("'; void 0;");
    page->executeJavaScript(javascript);
    QNTRACE("Queued javascript to make page " << (editable ? "editable" : "non-editable") << ": " << javascript);
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
                                                 *m_decryptedTextManager, m_errorCachedMemory,
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

void NoteEditorPrivate::onTableActionDone(const QVariant & dummy, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onTableActionDone");

    Q_UNUSED(dummy);
    Q_UNUSED(extraData);

    convertToNote();
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

bool NoteEditorPrivate::parseEncryptedTextContextMenuExtraData(const QStringList & extraData, QString & encryptedText,
                                                               QString & cipher, QString & keyLength, QString & hint,
                                                               QString & id, QString & errorDescription) const
{
    if (Q_UNLIKELY(extraData.empty())) {
        errorDescription = QT_TR_NOOP("extra data from JavaScript is empty");
        return false;
    }

    if (Q_UNLIKELY(extraData.size() != 5)) {
        errorDescription = QT_TR_NOOP("extra data from JavaScript has wrong size");
        return false;
    }

    cipher = extraData[0];
    keyLength = extraData[1];
    encryptedText = extraData[2];
    hint = extraData[3];
    id = extraData[4];
    return true;
}

void NoteEditorPrivate::rebuildRecognitionIndicesCache()
{
    QNDEBUG("NoteEditorPrivate::rebuildRecognitionIndicesCache");

    m_recognitionIndicesByResourceHash.clear();

    if (Q_UNLIKELY(!m_pNote)) {
        QNTRACE("No note is set");
        return;
    }

    if (!m_pNote->hasResources()) {
        QNTRACE("The note has no resources");
        return;
    }

    QList<ResourceAdapter> resources = m_pNote->resourceAdapters();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceAdapter & resource = resources[i];
        if (Q_UNLIKELY(!resource.hasDataHash())) {
            QNDEBUG("Skipping the resource without the data hash: " << resource);
            continue;
        }

        if (!resource.hasRecognitionDataBody()) {
            QNTRACE("Skipping the resource without recognition data body");
            continue;
        }

        ResourceRecognitionIndices recoIndices(resource.recognitionDataBody());
        if (recoIndices.isNull() || !recoIndices.isValid()) {
            QNTRACE("Skipping null/invalid resource recognition indices");
            continue;
        }

        m_recognitionIndicesByResourceHash[resource.dataHash()] = recoIndices;
    }
}

void NoteEditorPrivate::enableSpellCheck()
{
    QNDEBUG("NoteEditorPrivate::enableSpellCheck");

    refreshMisSpelledWordsList();
    applySpellCheck();
    enableDynamicSpellCheck();
}

void NoteEditorPrivate::disableSpellCheck()
{
    QNDEBUG("NoteEditorPrivate::disableSpellCheck");

    m_currentNoteMisSpelledWords.clear();
    GET_PAGE()
    page->executeJavaScript("spellChecker.remove();", NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onSpellCheckSetOrCleared));

    disableDynamicSpellCheck();
}

void NoteEditorPrivate::refreshMisSpelledWordsList()
{
    QNDEBUG("NoteEditorPrivate::refreshMisSpelledWordsList");

    if (!m_pNote) {
        QNDEBUG("No note is set to the editor");
        return;
    }

    m_currentNoteMisSpelledWords.clear();

    QString error;
    QStringList words = m_pNote->listOfWords(&error);
    if (words.isEmpty() && !error.isEmpty()) {
        error = QT_TR_NOOP("Can't get the list of words from the note: ") + error;
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    for(auto it = words.begin(), end = words.end(); it != end; ++it)
    {
        const QString & originalWord = *it;
        QNTRACE("Checking the word " << originalWord);

        QString word = originalWord;

        bool conversionResult = false;
        qint32 integerNumber = word.toInt(&conversionResult);
        if (conversionResult) {
            QNTRACE("Skipping the integer number " << word);
            continue;
        }

        qint64 longIntegerNumber = word.toLongLong(&conversionResult);
        if (conversionResult) {
            QNTRACE("Skipping the long long integer number " << word);
            continue;
        }

        Q_UNUSED(integerNumber)
        Q_UNUSED(longIntegerNumber)

        m_stringUtils.removePunctuation(word);
        if (word.isEmpty()) {
            QNTRACE("Skipping the word which becomes empty after stripping off the punctuation: " << originalWord);
            continue;
        }

        word = word.trimmed();

        QNTRACE("Checking the spelling of the \"adjusted\" word " << word);

        if (!m_pSpellChecker->checkSpell(word)) {
            QNTRACE("Misspelled word: \"" << word << "\"");
            word = originalWord;
            m_stringUtils.removePunctuation(word);
            word = word.trimmed();
            m_currentNoteMisSpelledWords << word;
            QNTRACE("Word added to the list: " << word);
        }
    }
}

void NoteEditorPrivate::applySpellCheck()
{
    QNDEBUG("NoteEditorPrivate::applySpellCheck");

    QString javascript = "spellChecker.apply('";
    for(auto it = m_currentNoteMisSpelledWords.begin(), end = m_currentNoteMisSpelledWords.end(); it != end; ++it) {
        javascript += *it;
        javascript += "', '";
    }
    javascript.chop(3);     // Remove trailing ", '";
    javascript += ");";

    GET_PAGE()
    page->executeJavaScript(javascript, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onSpellCheckSetOrCleared));
}

void NoteEditorPrivate::enableDynamicSpellCheck()
{
    QNDEBUG("NoteEditorPrivate::enableDynamicSpellCheck");

    GET_PAGE()
    page->executeJavaScript("spellChecker.enableDynamic();");
}

void NoteEditorPrivate::disableDynamicSpellCheck()
{
    QNDEBUG("NoteEditorPrivate::disableDynamicSpellCheck");

    GET_PAGE()
    page->executeJavaScript("spellChecker.disableDynamic();");
}

void NoteEditorPrivate::onSpellCheckSetOrCleared(const QVariant & dummy, const QVector<QPair<QString, QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onSpellCheckSetOrCleared");

    Q_UNUSED(dummy)
    Q_UNUSED(extraData)

    GET_PAGE()
#ifndef USE_QT_WEB_ENGINE
    m_htmlCachedMemory = page->mainFrame()->toHtml();
    onPageHtmlReceived(m_htmlCachedMemory);
#else
    page->toHtml(NoteEditorCallbackFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
#endif
}

bool NoteEditorPrivate::isNoteReadOnly() const
{
    QNDEBUG("NoteEditorPrivate::isNoteReadOnly");

    if (!m_pNote) {
        QNTRACE("No note is set to the editor");
        return true;
    }

    if (!m_pNotebook) {
        QNTRACE("No notebook is set to the editor");
        return true;
    }

    if (!m_pNotebook->hasRestrictions()) {
        QNTRACE("Notebook has no restrictions");
        return false;
    }

    const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
    if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref()) {
        QNTRACE("Restriction on note updating applies");
        return true;
    }

    return false;
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

void NoteEditorPrivate::execJavascriptCommand(const QString & command, NoteEditorPage::Callback callback)
{
    COMMAND_TO_JS(command);

    GET_PAGE()
    page->executeJavaScript(javascript, callback);
}

void NoteEditorPrivate::execJavascriptCommand(const QString & command, const QString & args,
                                              NoteEditorPage::Callback callback)
{
    COMMAND_WITH_ARGS_TO_JS(command, args);

    GET_PAGE()
    page->executeJavaScript(javascript, callback);
}

void NoteEditorPrivate::setUndoStack(QUndoStack * pUndoStack)
{
    QNDEBUG("NoteEditorPrivate::setUndoStack");

    QUTE_NOTE_CHECK_PTR(pUndoStack, "null undo stack passed to note editor");
    m_pUndoStack = pUndoStack;
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

            m_decryptedTextManager->clearNonRememberedForSessionEntries();
            QNTRACE("Removed non-per-session saved passphrases from decrypted text manager");

            // FIXME: remove any stale resource files left which are not related to the previous note
            // i.e. stale files for generic & image resources being removed from the note, stale image files
            // representing such removed generic resources etc; however, keep the resource files corresponding
            // to the resources the previous note still has
        }
    }

    // Clear the caches from previous note
    m_genericResourceLocalGuidBySaveToStorageRequestIds.clear();
    m_imageResourceSaveToStorageRequestIds.clear();
    m_resourceFileStoragePathsByResourceLocalGuid.clear();
    if (m_genericResourceImageFilePathsByResourceHash.size() > 30) {
        m_genericResourceImageFilePathsByResourceHash.clear();
        QNTRACE("Cleared the cache of generic resource image files by resource hash");
    }
    m_saveGenericResourceImageToFileRequestIds.clear();
    rebuildRecognitionIndicesCache();

    m_lastSearchHighlightedText.resize(0);
    m_lastSearchHighlightedTextCaseSensitivity = false;

#ifdef USE_QT_WEB_ENGINE
    m_localGuidsOfResourcesWantedToBeSaved.clear();
#else
    m_pluginFactory->setNote(*m_pNote);
#endif

    m_resourceLocalGuidAndFileStoragePathByReadResourceRequestIds.clear();
    emit currentNoteChanged(*m_pNote);

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

    m_writeNoteHtmlToFileRequestId = QUuid::createUuid();
    m_pendingIndexHtmlWritingToFile = true;
    emit writeNoteHtmlToFile(m_noteEditorPagePath, html.toLocal8Bit(),
                             m_writeNoteHtmlToFileRequestId, /* append = */ false);
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

    if (resource.hasDataHash() && resource.hasRecognitionDataBody())
    {
        ResourceRecognitionIndices recoIndices(resource.recognitionDataBody());
        if (!recoIndices.isNull() && recoIndices.isValid()) {
            m_recognitionIndicesByResourceHash[resource.dataHash()] = recoIndices;
            QNDEBUG("Set recognition indices for new resource: " << recoIndices);
        }
    }
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

    if (resource.hasDataHash())
    {
        auto it = m_recognitionIndicesByResourceHash.find(resource.dataHash());
        if (it != m_recognitionIndicesByResourceHash.end()) {
            Q_UNUSED(m_recognitionIndicesByResourceHash.erase(it));
            highlightRecognizedImageAreas(m_lastSearchHighlightedText, m_lastSearchHighlightedTextCaseSensitivity);
        }
    }
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

    const ResourceWrapper & targetResource = resources[resourceIndex];
    QByteArray previousResourceHash;
    if (targetResource.hasDataHash()) {
        previousResourceHash = targetResource.dataHash();
    }

    updateResource(targetResource.localGuid(), previousResourceHash, resource);
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
    rebuildRecognitionIndicesCache();
}

bool NoteEditorPrivate::isModified() const
{
    return m_modified;
}

void NoteEditorPrivate::setRenameResourceDelegateSubscriptions(RenameResourceDelegate & delegate)
{
    QObject::connect(&delegate, QNSIGNAL(RenameResourceDelegate,finished,QString,QString,ResourceWrapper,bool,QString),
                     this, QNSLOT(NoteEditorPrivate,onRenameResourceDelegateFinished,QString,QString,ResourceWrapper,bool,QString));
    QObject::connect(&delegate, QNSIGNAL(RenameResourceDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onRenameResourceDelegateError,QString));
    QObject::connect(&delegate, QNSIGNAL(RenameResourceDelegate,cancelled),
                     this, QNSLOT(NoteEditorPrivate,onRenameResourceDelegateCancelled));
}

void NoteEditorPrivate::cleanupStaleImageResourceFiles(const QString & resourceLocalGuid)
{
    QNDEBUG("NoteEditorPrivate::cleanupStaleImageResourceFiles: resource local guid = " << resourceLocalGuid);

    QString fileStoragePathPrefix = m_noteEditorImageResourcesStoragePath + "/" + resourceLocalGuid;
    QString fileStoragePath = fileStoragePathPrefix + ".png";

    QDir dir(m_noteEditorImageResourcesStoragePath);
    QFileInfoList entryList = dir.entryInfoList();

    const int numEntries = entryList.size();
    QNTRACE("Found " << numEntries << " files in the image resources folder");

    QString entryFilePath;
    for(int i = 0; i < numEntries; ++i)
    {
        const QFileInfo & entry = entryList[i];
        if (!entry.isFile() && !entry.isSymLink()) {
            continue;
        }

        entryFilePath = entry.absoluteFilePath();
        QNTRACE("See if we need to remove file " << entryFilePath);

        if (!entryFilePath.startsWith(fileStoragePathPrefix)) {
            continue;
        }

        if (entryFilePath.toUpper() == fileStoragePath.toUpper()) {
            continue;
        }

        QFile entryFile(entryFilePath);
        entryFile.close();  // NOTE: it appears to be important for Windows

        bool res = entryFile.remove();
        if (res) {
            QNTRACE("Successfully removed file " << entryFilePath);
            continue;
        }

#ifdef Q_OS_WIN
        if (entryFilePath.endsWith(".lnk")) {
            // NOTE: there appears to be a bug in Qt for Windows, QFile::remove returns false
            // for any *.lnk files even though the files are actually getting removed
            QNTRACE("Skipping the reported failure at removing the .lnk file");
            continue;
        }
#endif

        QNWARNING("Can't remove stale file " << entryFilePath << ": " << entryFile.errorString());
    }
}

QString NoteEditorPrivate::createLinkToImageResourceFile(const QString & fileStoragePath, const QString & localGuid, QString & errorDescription)
{
    QNDEBUG("NoteEditorPrivate::createLinkToImageResourceFile: file storage path = " << fileStoragePath
            << ", local guid = " << localGuid);

    QString linkFileName = fileStoragePath;
    linkFileName.remove(linkFileName.size() - 4, 4);
    linkFileName += "_";
    linkFileName += QString::number(QDateTime::currentMSecsSinceEpoch());

#ifdef Q_OS_WIN
    linkFileName += ".lnk";
#else
    linkFileName += ".png";
#endif

    QNTRACE("Link file name = " << linkFileName);

    cleanupStaleImageResourceFiles(localGuid);

    QFile imageResourceFile(fileStoragePath);
    bool res = imageResourceFile.link(linkFileName);
    if (Q_UNLIKELY(!res)) {
        errorDescription = QT_TR_NOOP("Can't process the update of image resource: can't create a symlink to the resource file");
        errorDescription += ": ";
        errorDescription += imageResourceFile.errorString();
        errorDescription += ", ";
        errorDescription += QT_TR_NOOP("error code = ");
        errorDescription += QString::number(imageResourceFile.error());
        return QString();
    }

    return linkFileName;
}

void NoteEditorPrivate::onDropEvent(QDropEvent * pEvent)
{
    QNDEBUG("NoteEditorPrivate::onDropEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING("Null pointer to drop event was detected");
        return;
    }

    const QMimeData * pMimeData = pEvent->mimeData();
    if (Q_UNLIKELY(!pMimeData)) {
        QNWARNING("Null pointer to mime data from drop event was detected");
        return;
    }

    QList<QUrl> urls = pMimeData->urls();
    typedef QList<QUrl>::iterator Iter;
    Iter urlsEnd = urls.end();
    for(Iter it = urls.begin(); it != urlsEnd; ++it)
    {
        if (Q_UNLIKELY(!it->isLocalFile())) {
            continue;
        }

        QString filePath = it->toLocalFile();
        dropFile(filePath);
    }

    pEvent->acceptProposedAction();
}

const ResourceWrapper NoteEditorPrivate::attachResourceToNote(const QByteArray & data, const QByteArray & dataHash,
                                                              const QMimeType & mimeType, const QString & filename)
{
    QNDEBUG("NoteEditorPrivate::attachResourceToNote: hash = " << dataHash
            << ", mime type = " << mimeType.name());

    ResourceWrapper resource;
    QString resourceLocalGuid = resource.localGuid();
    resource.setLocalGuid(QString());   // Force the resource to have empty local guid for now

    if (Q_UNLIKELY(!m_pNote)) {
        QNINFO("Can't attach resource to note editor: no actual note was selected");
        return resource;
    }

    // Now can return the local guid back to the resource
    resource.setLocalGuid(resourceLocalGuid);

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
    return resource;
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
    m_skipPushingUndoCommandOnNextContentChange = true;
    HANDLE_ACTION(undoPageAction, Undo);
}

void NoteEditorPrivate::redoPageAction()
{
    m_skipPushingUndoCommandOnNextContentChange = true;
    HANDLE_ACTION(redoPageAction, Redo);
}

void NoteEditorPrivate::flipEnToDoCheckboxState(const quint64 enToDoIdNumber)
{
    QNDEBUG("NoteEditorPrivate::flipEnToDoCheckboxState: " << enToDoIdNumber);

    GET_PAGE()
    QString javascript = QString("flipEnToDoCheckboxState(%1);").arg(QString::number(enToDoIdNumber));
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::onSpellCheckCorrectionAction()
{
    QNDEBUG("NoteEditorPrivate::onSpellCheckCorrectionAction");

    if (!m_spellCheckerEnabled) {
        QNDEBUG("Not enabled, won't do anything");
        return;
    }

    QAction * action = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!action)) {
        QString error = QT_TR_NOOP("Internal error: can't get the action which has toggled the spelling correction");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    const QString & correction = action->text();
    if (Q_UNLIKELY(correction.isEmpty())) {
        QNWARNING("No correction specified");
        return;
    }

    GET_PAGE()
    page->executeJavaScript("spellChecker.correctSpelling('" + correction + "');",
                            NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onSpellCheckCorrectionActionDone));
}

void NoteEditorPrivate::onSpellCheckIgnoreWordAction()
{
    QNDEBUG("NoteEditorPrivate::onSpellCheckIgnoreWordAction");

    if (!m_spellCheckerEnabled) {
        QNDEBUG("Not enabled, won't do anything");
        return;
    }

    m_pSpellChecker->ignoreWord(m_lastMisSpelledWord);
    m_currentNoteMisSpelledWords.removeAll(m_lastMisSpelledWord);
    applySpellCheck();

    SpellCheckIgnoreWordUndoCommand * pCommand = new SpellCheckIgnoreWordUndoCommand(*this, m_lastMisSpelledWord, m_pSpellChecker);
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::onSpellCheckAddWordToUserDictionaryAction()
{
    QNDEBUG("NoteEditorPrivate::onSpellCheckAddWordToUserDictionaryAction");

    if (!m_spellCheckerEnabled) {
        QNDEBUG("Not enabled, won't do anything");
        return;
    }

    m_pSpellChecker->addToUserWordlist(m_lastMisSpelledWord);
    m_currentNoteMisSpelledWords.removeAll(m_lastMisSpelledWord);
    applySpellCheck();

    SpellCheckAddToUserWordListUndoCommand * pCommand = new SpellCheckAddToUserWordListUndoCommand(*this, m_lastMisSpelledWord, m_pSpellChecker);
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::onSpellCheckCorrectionActionDone(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onSpellCheckCorrectionActionDone: " << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of spelling correction from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of spelling correction from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't correct spelling: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    SpellCorrectionUndoCommand * pCommand = new SpellCorrectionUndoCommand(*this, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onSpellCheckCorrectionUndoRedoFinished));
    m_pUndoStack->push(pCommand);

    applySpellCheck();

    convertToNote();
}

void NoteEditorPrivate::onSpellCheckCorrectionUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onSpellCheckCorrectionUndoRedoFinished");

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of spelling correction undo/redo from JavaScript");
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
            error = QT_TR_NOOP("Internal error: can't parse the error of spelling correction undo/redo from JavaScript");
        }
        else {
            error = QT_TR_NOOP("Can't undo/redo spelling correction: ") + errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::onSpellCheckerDynamicHelperUpdate(QStringList words)
{
    QNDEBUG("NoteEditorPrivate::onSpellCheckerDynamicHelperUpdate: " << words.join(";"));

    if (!m_spellCheckerEnabled) {
        QNTRACE("No spell checking is enabled, nothing to do");
        return;
    }

    bool foundMisSpelledWords = false;
    for(auto it = words.begin(), end = words.end(); it != end; ++it)
    {
        QString word = *it;
        word = word.trimmed();
        m_stringUtils.removePunctuation(word);

        if (m_pSpellChecker->checkSpell(word)) {
            QNTRACE("No misspelling detected");
            continue;
        }

        if (!m_currentNoteMisSpelledWords.contains(word)) {
            m_currentNoteMisSpelledWords << word;
        }

        foundMisSpelledWords = true;
    }

    if (!foundMisSpelledWords) {
        return;
    }

    applySpellCheck();
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
    QNDEBUG("NoteEditorPrivate::paste");

    GET_PAGE()

    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        QNWARNING("Can't access the application clipboard to analyze the pasted content");
        page->triggerAction(WebPage::Paste);
        setFocus();
        return;
    }

    QString textToPaste = pClipboard->text();
    QNTRACE("Text to paste: " << textToPaste);

    bool shouldBeHyperlink = textToPaste.startsWith("http://") || textToPaste.startsWith("https://") || textToPaste.startsWith("mailto:") || textToPaste.startsWith("ftp://");
    bool shouldBeAttachment = textToPaste.startsWith("file://");

    if (!shouldBeHyperlink && !shouldBeAttachment)
    {
        QNTRACE("The pasted text doesn't appear to be a url of hyperlink or attachment");
        page->triggerAction(WebPage::Paste);
        setFocus();
        return;
    }

    QUrl url(textToPaste);
    if (shouldBeAttachment)
    {
        if (!url.isValid()) {
            QNTRACE("The pasted text seemed like file url but the url isn't valid after all, fallback to simple paste");
            page->triggerAction(WebPage::Paste);
            setFocus();
            return;
        }

        dropFile(url.toLocalFile());
        return;
    }

    if (!url.isValid()) {
        url.setScheme("evernote");
    }

    if (!url.isValid()) {
        QNDEBUG("It appears we don't paste a url");
        page->triggerAction(WebPage::Paste);
        setFocus();
        return;
    }

    QNDEBUG("Was able to create the url from pasted text, inserting a hyperlink");

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    textToPaste = url.toString(QUrl::FullyEncoded);
#else
    textToPaste = url.toString(QUrl::None);
#endif

    quint64 hyperlinkId = m_lastFreeHyperlinkIdNumber++;

    AddHyperlinkToSelectedTextDelegate * delegate = new AddHyperlinkToSelectedTextDelegate(*this, hyperlinkId);
    QObject::connect(delegate, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,finished,QString,int,int),
                     this, QNSLOT(NoteEditorPrivate,onAddHyperlinkToSelectedTextDelegateFinished,QString,int,int));
    QObject::connect(delegate, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,cancelled),
                     this, QNSLOT(NoteEditorPrivate,onAddHyperlinkToSelectedTextDelegateCancelled));
    QObject::connect(delegate, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onAddHyperlinkToSelectedTextDelegateError,QString));
    delegate->startWithPresetHyperlink(textToPaste);
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

QString NoteEditorPrivate::selectedText() const
{
    return page()->selectedText();
}

bool NoteEditorPrivate::hasSelection() const
{
    return page()->hasSelection();
}

void NoteEditorPrivate::findNext(const QString & text, const bool matchCase) const
{
    QNDEBUG("NoteEditorPrivate::findNext: " << text << "; match case = " << (matchCase ? "true" : "false"));

    findText(text, matchCase);
}

void NoteEditorPrivate::findPrevious(const QString & text, const bool matchCase) const
{
    QNDEBUG("NoteEditorPrivate::findPrevious: " << text << "; match case = " << (matchCase ? "true" : "false"));

    findText(text, matchCase, /* search backward = */ true);
}

void NoteEditorPrivate::replace(const QString & textToReplace, const QString & replacementText, const bool matchCase)
{
    QNDEBUG("NoteEditorPrivate::replace: text to replace = " << textToReplace << "; replacement text = "
            << replacementText << "; match case = " << (matchCase ? "true" : "false"));

    GET_PAGE()

    QString escapedTextToReplace = textToReplace;
    ENMLConverter::escapeString(escapedTextToReplace);

    QString escapedReplacementText = replacementText;
    ENMLConverter::escapeString(escapedReplacementText);

    QString javascript = QString("findReplaceManager.replace('%1', '%2', %3);").arg(escapedTextToReplace, escapedReplacementText, (matchCase ? "true" : "false"));
    page->executeJavaScript(javascript, ReplaceCallback(this));

    ReplaceUndoCommand * pCommand = new ReplaceUndoCommand(textToReplace, matchCase, *this, ReplaceCallback(this));
    m_pUndoStack->push(pCommand);

    setSearchHighlight(textToReplace, matchCase, /* force = */ true);
    findNext(textToReplace, matchCase);
}

void NoteEditorPrivate::replaceAll(const QString & textToReplace, const QString & replacementText, const bool matchCase)
{
    QNDEBUG("NoteEditorPrivate::replaceAll: text to replace = " << textToReplace << "; replacement text = "
            << replacementText << "; match case = " << (matchCase ? "true" : "false"));

    GET_PAGE()

    QString escapedTextToReplace = textToReplace;
    ENMLConverter::escapeString(escapedTextToReplace);

    QString escapedReplacementText = replacementText;
    ENMLConverter::escapeString(escapedReplacementText);

    QString javascript = QString("findReplaceManager.replaceAll('%1', '%2', %3);").arg(escapedTextToReplace, escapedReplacementText, (matchCase ? "true" : "false"));
    page->executeJavaScript(javascript, ReplaceCallback(this));

    ReplaceAllUndoCommand * pCommand = new ReplaceAllUndoCommand(textToReplace, matchCase, *this, ReplaceCallback(this));
    m_pUndoStack->push(pCommand);

    setSearchHighlight(textToReplace, matchCase, /* force = */ true);
}

void NoteEditorPrivate::onReplaceJavaScriptDone(const QVariant & data)
{
    QNDEBUG("NoteEditorPrivate::onReplaceJavaScriptDone");

    Q_UNUSED(data);
    convertToNote();
}

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

void NoteEditorPrivate::setSpellcheck(const bool enabled)
{
    QNDEBUG("stub: NoteEditorPrivate::setSpellcheck: enabled = "
            << (enabled ? "true" : "false"));

    if (m_spellCheckerEnabled == enabled) {
        QNTRACE("Spell checker enabled flag didn't change");
        return;
    }

    m_spellCheckerEnabled = enabled;
    if (m_spellCheckerEnabled) {
        enableSpellCheck();
    }
    else {
        disableSpellCheck();
    }

    setFocus();
}

bool NoteEditorPrivate::spellCheckEnabled() const
{
    return m_spellCheckerEnabled;
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

void NoteEditorPrivate::insertTableRow()
{
    QNDEBUG("NoteEditorPrivate::insertTableRow");

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onTableActionDone);

    GET_PAGE()
    page->executeJavaScript("tableManager.insertRow();", callback);

    TableActionUndoCommand * pCommand = new TableActionUndoCommand(*this, tr("Insert row"), callback);
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::insertTableColumn()
{
    QNDEBUG("NoteEditorPrivate::insertTableColumn");

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onTableActionDone);

    GET_PAGE()
    page->executeJavaScript("tableManager.insertColumn();", callback);

    TableActionUndoCommand * pCommand = new TableActionUndoCommand(*this, tr("Insert column"), callback);
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::removeTableRow()
{
    QNDEBUG("NoteEditorPrivate::removeTableRow");

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onTableActionDone);

    GET_PAGE()
    page->executeJavaScript("tableManager.removeRow();", callback);

    TableActionUndoCommand * pCommand = new TableActionUndoCommand(*this, tr("Remove row"), callback);
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::removeTableColumn()
{
    QNDEBUG("NoteEditorPrivate::removeTableColumn");

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onTableActionDone);

    GET_PAGE()
    page->executeJavaScript("tableManager.removeColumn();", callback);

    TableActionUndoCommand * pCommand = new TableActionUndoCommand(*this, tr("Remove column"), callback);
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::addAttachmentDialog()
{
    QNDEBUG("NoteEditorPrivate::addAttachmentDialog");

    QString addAttachmentInitialFolderPath;

    ApplicationSettings appSettings;
    QVariant lastAttachmentAddLocation = appSettings.value("LastAttachmentAddLocation");
    if (!lastAttachmentAddLocation.isNull() && lastAttachmentAddLocation.isValid())
    {
        QNTRACE("Found last attachment add location: " << lastAttachmentAddLocation);
        QFileInfo lastAttachmentAddDirInfo(lastAttachmentAddLocation.toString());
        if (!lastAttachmentAddDirInfo.exists()) {
            QNTRACE("Cached last attachment add directory does not exist");
        }
        else if (!lastAttachmentAddDirInfo.isDir()) {
            QNTRACE("Cached last attachment add directory path is not a directory really");
        }
        else if (!lastAttachmentAddDirInfo.isWritable()) {
            QNTRACE("Cached last attachment add directory path is not writable");
        }
        else {
            addAttachmentInitialFolderPath = lastAttachmentAddDirInfo.absolutePath();
        }
    }

    QString absoluteFilePath = QFileDialog::getOpenFileName(this, tr("Add attachment..."),
                                                            addAttachmentInitialFolderPath);
    if (absoluteFilePath.isEmpty()) {
        QNTRACE("User cancelled adding the attachment");
        return;
    }

    QNTRACE("Absolute file path of chosen attachment: " << absoluteFilePath);

    QFileInfo fileInfo(absoluteFilePath);
    QString absoluteDirPath = fileInfo.absoluteDir().absolutePath();
    if (!absoluteDirPath.isEmpty()) {
        appSettings.setValue("LastAttachmentAddLocation", absoluteDirPath);
        QNTRACE("Updated last attachment add location to " << absoluteDirPath);
    }

    dropFile(absoluteFilePath);
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

void NoteEditorPrivate::removeAttachment(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::removeAttachment: hash = " << resourceHash);

    if (Q_UNLIKELY(!m_pNote)) {
        QString error = QT_TR_NOOP("Can't remove resource by hash: no note is set to the editor");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool foundResourceToRemove = false;
    QList<ResourceWrapper> resources = m_pNote->resources();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceWrapper & resource = resources[i];
        if (resource.hasDataHash() && (resource.dataHash() == resourceHash))
        {
            m_resourceInfo.removeResourceInfo(resource.dataHash());

            RemoveResourceDelegate * delegate = new RemoveResourceDelegate(resource, *this);
            QObject::connect(delegate, QNSIGNAL(RemoveResourceDelegate,finished,ResourceWrapper),
                             this, QNSLOT(NoteEditorPrivate,onRemoveResourceDelegateFinished,ResourceWrapper));
            QObject::connect(delegate, QNSIGNAL(RemoveResourceDelegate,notifyError,QString),
                             this, QNSLOT(NoteEditorPrivate,onRemoveResourceDelegateError,QString));
            delegate->start();

            foundResourceToRemove = true;
            break;
        }
    }

    if (Q_UNLIKELY(!foundResourceToRemove)) {
        QString error = QT_TR_NOOP("Can't remove resource by hash: no resource with such hash was found within the note");
        QNWARNING(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::removeAttachmentUnderCursor()
{
    QNDEBUG("NoteEditorPrivate::removeAttachmentUnderCursor");

    if ((m_currentContextMenuExtraData.m_contentType != "ImageResource") &&
        (m_currentContextMenuExtraData.m_contentType != "NonImageResource"))
    {
        QString error = QT_TR_NOOP("Can't remove attachment under cursor: wrong current context menu extra data's content type");
        QNWARNING(error << ": content type = " << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    removeAttachment(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::renameAttachmentUnderCursor()
{
    QNDEBUG("NoteEditorPrivate::renameAttachmentUnderCursor");

    if (m_currentContextMenuExtraData.m_contentType != "NonImageResource") {
        QString error = QT_TR_NOOP("Can't rename attachment under cursor: wrong current context menu extra data's content type");
        QNWARNING(error << ": content type = " << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    renameAttachment(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::renameAttachment(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::renameAttachment: resource hash = " << resourceHash);

    QString errorPrefix = QT_TR_NOOP("Can't rename attachment:") + QString(" ");

    if (Q_UNLIKELY(!m_pNote)) {
        QString error = errorPrefix + QT_TR_NOOP("no note is set to the editor");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int targetResourceIndex = -1;
    QList<ResourceWrapper> resources = m_pNote->resources();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceWrapper & resource = resources[i];
        if (!resource.hasDataHash() || (resource.dataHash() != resourceHash)) {
            continue;
        }

        targetResourceIndex = i;
        break;
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        QString error = errorPrefix + QT_TR_NOOP("can't find the corresponding resource in the note");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    ResourceWrapper & resource = resources[targetResourceIndex];
    if (Q_UNLIKELY(!resource.hasDataBody())) {
        QString error = errorPrefix + QT_TR_NOOP("the resource doesn't have the data body set");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!resource.hasDataHash())) {
        QString error = errorPrefix + QT_TR_NOOP("the resource doesn't have the data hash set");
        QNWARNING(error << ", resource: " << resource);
        emit notifyError(error);
        return;
    }

    RenameResourceDelegate * delegate = new RenameResourceDelegate(resource, *this, m_pGenericResourceImageWriter);

    QObject::connect(delegate, QNSIGNAL(RenameResourceDelegate,finished,QString,QString,ResourceWrapper,bool,QString),
                     this, QNSLOT(NoteEditorPrivate,onRenameResourceDelegateFinished,QString,QString,ResourceWrapper,bool,QString));
    QObject::connect(delegate, QNSIGNAL(RenameResourceDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onRenameResourceDelegateError,QString));
    QObject::connect(delegate, QNSIGNAL(RenameResourceDelegate,cancelled),
                     this, QNSLOT(NoteEditorPrivate,onRenameResourceDelegateCancelled));

    delegate->start();
}

void NoteEditorPrivate::rotateImageAttachment(const QString & resourceHash, const Rotation::type rotationDirection)
{
    QNDEBUG("NoteEditorPrivate::rotateImageAttachment: resource hash = " << resourceHash << ", rotation: " << rotationDirection);

    QString errorPrefix = QT_TR_NOOP("Can't rotate image attachment:") + QString(" ");

    if (Q_UNLIKELY(!m_pNote)) {
        QString error = errorPrefix + QT_TR_NOOP("no note is set to the editor");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int targetResourceIndex = -1;
    QList<ResourceWrapper> resources = m_pNote->resources();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceWrapper & resource = resources[i];
        if (!resource.hasDataHash() || (resource.dataHash() != resourceHash)) {
            continue;
        }

        if (Q_UNLIKELY(!resource.hasMime())) {
            QString error = errorPrefix + QT_TR_NOOP("corresponding resource's mime type is not set");
            QNWARNING(error << ", resource: " << resource);
            emit notifyError(error);
            return;
        }

        if (Q_UNLIKELY(!resource.mime().startsWith("image/"))) {
            QString error = errorPrefix + QT_TR_NOOP("corresponding resource's mime type indicates it is not an image");
            QNWARNING(error << ", resource: " << resource);
            emit notifyError(error);
            return;
        }

        targetResourceIndex = i;
        break;
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        QString error = errorPrefix + QT_TR_NOOP("can't find the corresponding resource in the note");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    ResourceWrapper & resource = resources[targetResourceIndex];
    if (Q_UNLIKELY(!resource.hasDataBody())) {
        QString error = errorPrefix + QT_TR_NOOP("the resource doesn't have the data body set");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!resource.hasDataHash())) {
        QString error = errorPrefix + QT_TR_NOOP("the resource doesn't have the data hash set");
        QNWARNING(error << ", resource: " << resource);
        emit notifyError(error);
        return;
    }

    ImageResourceRotationDelegate * delegate = new ImageResourceRotationDelegate(resource.dataHash(), rotationDirection,
                                                                                 *this, m_resourceInfo, *m_pResourceFileStorageManager,
                                                                                 m_resourceFileStoragePathsByResourceLocalGuid);

    QObject::connect(delegate, QNSIGNAL(ImageResourceRotationDelegate,finished,QByteArray,QString,QByteArray,QByteArray,ResourceWrapper,INoteEditorBackend::Rotation::type),
                     this, QNSLOT(NoteEditorPrivate,onImageResourceRotationDelegateFinished,QByteArray,QString,QByteArray,QByteArray,ResourceWrapper,INoteEditorBackend::Rotation::type));
    QObject::connect(delegate, QNSIGNAL(ImageResourceRotationDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onImageResourceRotationDelegateError,QString));

    delegate->start();
}

void NoteEditorPrivate::rotateImageAttachmentUnderCursor(const Rotation::type rotationDirection)
{
    QNDEBUG("INoteEditorBackend::rotateImageAttachmentUnderCursor: rotation: " << rotationDirection);

    if (m_currentContextMenuExtraData.m_contentType != "ImageResource") {
        QString error = QT_TR_NOOP("Can't rotate image attachment under cursor: wrong current context menu extra data's content type");
        QNWARNING(error << ": content type = " << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    rotateImageAttachment(m_currentContextMenuExtraData.m_resourceHash, rotationDirection);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::rotateImageAttachmentUnderCursorClockwise()
{
    QNDEBUG("NoteEditorPrivate::rotateImageAttachmentUnderCursorClockwise");
    rotateImageAttachmentUnderCursor(Rotation::Clockwise);
}

void NoteEditorPrivate::rotateImageAttachmentUnderCursorCounterclockwise()
{
    QNDEBUG("NoteEditorPrivate::rotateImageAttachmentUnderCursorCounterclockwise");
    rotateImageAttachmentUnderCursor(Rotation::Counterclockwise);
}

void NoteEditorPrivate::encryptSelectedText()
{
    QNDEBUG("NoteEditorPrivate::encryptSelectedText");

    EncryptSelectedTextDelegate * delegate = new EncryptSelectedTextDelegate(this, m_encryptionManager, m_decryptedTextManager);
    QObject::connect(delegate, QNSIGNAL(EncryptSelectedTextDelegate,finished),
                     this, QNSLOT(NoteEditorPrivate,onEncryptSelectedTextDelegateFinished));
    QObject::connect(delegate, QNSIGNAL(EncryptSelectedTextDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onEncryptSelectedTextDelegateError,QString));
    QObject::connect(delegate, QNSIGNAL(EncryptSelectedTextDelegate,cancelled),
                     this, QNSLOT(NoteEditorPrivate,onEncryptSelectedTextDelegateCancelled));
    delegate->start(m_lastSelectedHtml);
}

void NoteEditorPrivate::decryptEncryptedTextUnderCursor()
{
    QNDEBUG("NoteEditorPrivate::decryptEncryptedTextUnderCursor");

    if (Q_UNLIKELY(m_currentContextMenuExtraData.m_contentType != "EncryptedText")) {
        QString error = QT_TR_NOOP("Can't decrypt the encrypted text under cursor: wrong current context menu extra data's content type");
        QNWARNING(error << ": content type = " << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    decryptEncryptedText(m_currentContextMenuExtraData.m_encryptedText, m_currentContextMenuExtraData.m_cipher,
                         m_currentContextMenuExtraData.m_keyLength, m_currentContextMenuExtraData.m_hint,
                         m_currentContextMenuExtraData.m_id);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::decryptEncryptedText(QString encryptedText, QString cipher,
                                             QString length, QString hint, QString enCryptIndex)
{
    QNDEBUG("NoteEditorPrivate::decryptEncryptedText");

    DecryptEncryptedTextDelegate * delegate = new DecryptEncryptedTextDelegate(enCryptIndex, encryptedText, cipher, length, hint, this,
                                                                               m_encryptionManager, m_decryptedTextManager);

    QObject::connect(delegate, QNSIGNAL(DecryptEncryptedTextDelegate,finished,QString,QString,size_t,QString,QString,QString,bool,bool),
                     this, QNSLOT(NoteEditorPrivate,onDecryptEncryptedTextDelegateFinished,QString,QString,size_t,QString,QString,QString,bool,bool));
    QObject::connect(delegate, QNSIGNAL(DecryptEncryptedTextDelegate,cancelled),
                     this, QNSLOT(NoteEditorPrivate,onDecryptEncryptedTextDelegateCancelled));
    QObject::connect(delegate, QNSIGNAL(DecryptEncryptedTextDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onDecryptEncryptedTextDelegateError,QString));

    delegate->start();
}

void NoteEditorPrivate::hideDecryptedTextUnderCursor()
{
    QNDEBUG("NoteEditorPrivate::hideDecryptedTextUnderCursor");

    if (Q_UNLIKELY(m_currentContextMenuExtraData.m_contentType != "GenericText")) {
        QString error = QT_TR_NOOP("Can't hide the decrypted text under cursor: wrong current context menu extra data's content type");
        QNWARNING(error << ": content type = " << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!m_currentContextMenuExtraData.m_insideDecryptedText)) {
        QString error = QT_TR_NOOP("Can't hide the decrypted text under cursor: the cursor doesn't appear to be inside the decrypted text area");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    hideDecryptedText(m_currentContextMenuExtraData.m_encryptedText, m_currentContextMenuExtraData.m_cipher,
                      m_currentContextMenuExtraData.m_keyLength, m_currentContextMenuExtraData.m_hint,
                      m_currentContextMenuExtraData.m_id);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::hideDecryptedText(QString encryptedText, QString cipher, QString keyLength, QString hint, QString id)
{
    QNDEBUG("NoteEditorPrivate::hideDecryptedText");

    bool conversionResult = false;
    size_t keyLengthInt = static_cast<size_t>(keyLength.toInt(&conversionResult));
    if (Q_UNLIKELY(!conversionResult)) {
        QString error = QT_TR_NOOP("Can't hide the decrypted text: can't convert the key length attribute to integer number");
        QNWARNING(error << ", key length = " << keyLength);
        emit notifyError(error);
        return;
    }

    quint64 enCryptIndex = m_lastFreeEnCryptIdNumber++;
    QString html = ENMLConverter::encryptedTextHtml(encryptedText, hint, cipher, keyLengthInt, enCryptIndex);
    ENMLConverter::escapeString(html);

    QString javascript = "encryptDecryptManager.replaceDecryptedTextWithEncryptedText('" + id + "', '" + html + "');";
    GET_PAGE()
    page->executeJavaScript(javascript, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onHideDecryptedTextFinished));
}

void NoteEditorPrivate::editHyperlinkDialog()
{
    QNDEBUG("NoteEditorPrivate::editHyperlinkDialog");

    // NOTE: when adding the new hyperlink, the selected html can be empty, it's ok
    m_lastSelectedHtmlForHyperlink = m_lastSelectedHtml;

    QString javascript = "hyperlinkManager.findSelectedHyperlinkId();";
    GET_PAGE()

    page->executeJavaScript(javascript, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onFoundSelectedHyperlinkId));
}

void NoteEditorPrivate::copyHyperlink()
{
    QNDEBUG("NoteEditorPrivate::copyHyperlink");

    GET_PAGE()
    page->executeJavaScript("hyperlinkManager.getSelectedHyperlinkData();",
                            NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onFoundHyperlinkToCopy));
}

void NoteEditorPrivate::removeHyperlink()
{
    QNDEBUG("NoteEditorPrivate::removeHyperlink");

    RemoveHyperlinkDelegate * delegate = new RemoveHyperlinkDelegate(*this);
    QObject::connect(delegate, QNSIGNAL(RemoveHyperlinkDelegate,finished),
                     this, QNSLOT(NoteEditorPrivate,onRemoveHyperlinkDelegateFinished));
    QObject::connect(delegate, QNSIGNAL(RemoveHyperlinkDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onRemoveHyperlinkDelegateError,QString));
    delegate->start();
}

void NoteEditorPrivate::onNoteLoadCancelled()
{
    stop();
    QNINFO("Note load has been cancelled");
    // TODO: add some overlay widget for NoteEditor to properly indicate visually that the note load has been cancelled
}

void NoteEditorPrivate::onTableResized()
{
    QNDEBUG("NoteEditorPrivate::onTableResized");
    convertToNote();
}

void NoteEditorPrivate::onFoundSelectedHyperlinkId(const QVariant & hyperlinkData,
                                                   const QVector<QPair<QString, QString> > & extraData)
{
    QNDEBUG("NoteEditorPrivate::onFoundSelectedHyperlinkId: " << hyperlinkData);
    Q_UNUSED(extraData);

    QMap<QString,QVariant> resultMap = hyperlinkData.toMap();

    auto statusIt = resultMap.find("status");
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the result of the attempt to find the hyperlink data by id from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QNTRACE("No hyperlink id under cursor was found, assuming we're adding the new hyperlink to the selected text");

        GET_PAGE()

        quint64 hyperlinkId = m_lastFreeHyperlinkIdNumber++;
        AddHyperlinkToSelectedTextDelegate * delegate = new AddHyperlinkToSelectedTextDelegate(*this, hyperlinkId);
        QObject::connect(delegate, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,finished),
                         this, QNSLOT(NoteEditorPrivate,onAddHyperlinkToSelectedTextDelegateFinished));
        QObject::connect(delegate, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,cancelled),
                         this, QNSLOT(NoteEditorPrivate,onAddHyperlinkToSelectedTextDelegateCancelled));
        QObject::connect(delegate, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,notifyError,QString),
                         this, QNSLOT(NoteEditorPrivate,onAddHyperlinkToSelectedTextDelegateError,QString));
        delegate->start();

        return;
    }

    auto dataIt = resultMap.find("data");
    if (Q_UNLIKELY(dataIt == resultMap.end())) {
        QString error = QT_TR_NOOP("Internal error: can't parse the seemingly positive result of the attempt "
                                   "to find the hyperlink data by id from JavaScript");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString hyperlinkDataStr = dataIt.value().toString();

    bool conversionResult = false;
    quint64 hyperlinkId = hyperlinkDataStr.toULongLong(&conversionResult);
    if (!conversionResult) {
        m_errorCachedMemory = QT_TR_NOOP("Can't add or edit hyperlink under cursor: can't convert hyperlink id number to unsigned int");
        QNWARNING(m_errorCachedMemory << "; hyperlink data: " << hyperlinkDataStr);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    QNTRACE("Will edit the hyperlink with id " << hyperlinkId);
    EditHyperlinkDelegate * delegate = new EditHyperlinkDelegate(*this, hyperlinkId);
    QObject::connect(delegate, QNSIGNAL(EditHyperlinkDelegate,finished),
                     this, QNSLOT(NoteEditorPrivate,onEditHyperlinkDelegateFinished));
    QObject::connect(delegate, QNSIGNAL(EditHyperlinkDelegate,cancelled), this, QNSLOT(NoteEditorPrivate,onEditHyperlinkDelegateCancelled));
    QObject::connect(delegate, QNSIGNAL(EditHyperlinkDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onEditHyperlinkDelegateError,QString));
    delegate->start();
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

    if (hyperlinkDataList.size() != 3) {
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

void NoteEditorPrivate::dropFile(const QString & filePath)
{
    QNDEBUG("NoteEditorPrivate::dropFile: " << filePath);

    AddResourceDelegate * delegate = new AddResourceDelegate(filePath, *this, m_pResourceFileStorageManager,
                                                             m_pFileIOThreadWorker, m_pGenericResourceImageWriter,
                                                             m_genericResourceImageFilePathsByResourceHash);

    QObject::connect(delegate, QNSIGNAL(AddResourceDelegate,finished,ResourceWrapper,QString),
                     this, QNSLOT(NoteEditorPrivate,onAddResourceDelegateFinished,ResourceWrapper,QString));
    QObject::connect(delegate, QNSIGNAL(AddResourceDelegate,notifyError,QString),
                     this, QNSLOT(NoteEditorPrivate,onAddResourceDelegateError,QString));

    delegate->start();
}

} // namespace qute_note

void __initNoteEditorResources()
{
    Q_INIT_RESOURCE(underline);
    Q_INIT_RESOURCE(css);
    Q_INIT_RESOURCE(checkbox_icons);
    Q_INIT_RESOURCE(encrypted_area_icons);
    Q_INIT_RESOURCE(generic_resource_icons);
    Q_INIT_RESOURCE(jquery);
    Q_INIT_RESOURCE(colResizable);
    Q_INIT_RESOURCE(debounce);
    Q_INIT_RESOURCE(scripts);
    Q_INIT_RESOURCE(hilitor);

    QNDEBUG("Initialized NoteEditor's resources");
}
