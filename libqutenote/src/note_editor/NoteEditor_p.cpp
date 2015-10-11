#include "NoteEditor_p.h"
#include "NoteEditorPage.h"
#include "javascript_glue/ResourceInfoJavaScriptHandler.h"
#include "javascript_glue/TextCursorPositionJavaScriptHandler.h"
#include "javascript_glue/ContextMenuEventJavaScriptHandler.h"

#ifndef USE_QT_WEB_ENGINE
#include "EncryptedAreaPlugin.h"
#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#include <qute_note/utility/ApplicationSettings.h>
#include <QWebFrame>
typedef QWebSettings WebSettings;
#else
#include "javascript_glue/MimeTypeIconJavaScriptHandler.h"
#include "javascript_glue/PageMutationHandler.h"
#include "javascript_glue/EnCryptElementOnClickHandler.h"
#include "javascript_glue/IconThemeJavaScriptHandler.h"
#include "javascript_glue/GenericResourceOpenAndSaveButtonsOnClickHandler.h"
#include "javascript_glue/GenericResourceImageJavaScriptHandler.h"
#include "NoteDecryptionDialog.h"
#include "WebSocketClientWrapper.h"
#include "WebSocketTransport.h"
#include "GenericResourceImageWriter.h"
#include <qute_note/utility/ApplicationSettings.h>
#include <QDesktopServices>
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

#define GET_PAGE() \
    NoteEditorPage * page = qobject_cast<NoteEditorPage*>(q->page()); \
    if (Q_UNLIKELY(!page)) { \
        QNFATAL("Can't get access to note editor's underlying page!"); \
        return; \
    }

namespace qute_note {

NoteEditorPrivate::NoteEditorPrivate(NoteEditor & noteEditor) :
    QObject(&noteEditor),
    m_noteEditorPageFolderPath(),
    m_noteEditorPagePath(),
    m_noteEditorImageResourcesStoragePath(),
    m_font(),
    m_backgroundColor(),
    m_jQueryJs(),
    m_resizableTableColumnsJs(),
    m_onFixedWidthTableResizeJs(),
    m_getSelectionHtmlJs(),
    m_snapSelectionToWordJs(),
    m_replaceSelectionWithHtmlJs(),
    m_provideSrcForResourceImgTagsJs(),
    m_provideGenericResourceDisplayNameAndSizeJs(),
    m_setupEnToDoTagsJs(),
    m_onResourceInfoReceivedJs(),
    m_determineStatesForCurrentTextCursorPositionJs(),
    m_determineContextMenuEventTargetJs(),
#ifndef USE_QT_WEB_ENGINE
    m_qWebKitSetupJs(),
#else
    m_provideSrcForGenericResourceImagesJs(),
    m_onGenericResourceImageReceivedJs(),
    m_provideSrcForGenericResourceIconsJs(),
    m_provideSrcAndOnClickScriptForEnCryptImgTagsJs(),
    m_qWebChannelJs(),
    m_qWebChannelSetupJs(),
    m_pageMutationObserverJs(),
    m_onIconFilePathForIconThemeNameReceivedJs(),
    m_provideSrcForGenericResourceOpenAndSaveIconsJs(),
    m_setupSaveResourceButtonOnClickHandlerJs(),
    m_setupOpenResourceButtonOnClickHandlerJs(),
    m_notifyTextCursorPositionChangedJs(),
    m_pWebSocketServer(new QWebSocketServer("QWebChannel server", QWebSocketServer::NonSecureMode, this)),
    m_pWebSocketClientWrapper(new WebSocketClientWrapper(m_pWebSocketServer, this)),
    m_pWebChannel(new QWebChannel(this)),
    m_pPageMutationHandler(new PageMutationHandler(this)),
    m_pMimeTypeIconJavaScriptHandler(Q_NULLPTR),
    m_pEnCryptElementClickHandler(new EnCryptElementOnClickHandler(this)),
    m_pIconThemeJavaScriptHandler(Q_NULLPTR),
    m_pGenericResourceOpenAndSaveButtonsOnClickHandler(new GenericResourceOpenAndSaveButtonsOnClickHandler(this)),
    m_pGenericResourceImageWriter(Q_NULLPTR),
    m_webSocketServerPort(0),
#endif
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
                 "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-todo.css\">"
                 "<title></title></head>"),
    m_enmlCachedMemory(),
    m_htmlCachedMemory(),
    m_errorCachedMemory(),
    m_pIOThread(Q_NULLPTR),
    m_pResourceFileStorageManager(Q_NULLPTR),
    m_pFileIOThreadWorker(Q_NULLPTR),
    m_resourceInfo(),
    m_pResourceInfoJavaScriptHandler(new ResourceInfoJavaScriptHandler(m_resourceInfo, this)),
    m_resourceLocalFileStorageFolder(),
    m_genericResourceLocalGuidBySaveToStorageRequestIds(),
    m_resourceFileStoragePathsByResourceLocalGuid(),
#ifdef USE_QT_WEB_ENGINE
    m_localGuidsOfResourcesWantedToBeSaved(),
    m_localGuidsOfResourcesWantedToBeOpened(),
    m_fileSuffixesForMimeType(),
    m_fileFilterStringForMimeType(),
    m_manualSaveResourceToFileRequestIds(),
    m_genericResourceImageFilePathsByResourceHash(),
    m_pGenericResoureImageJavaScriptHandler(new GenericResourceImageJavaScriptHandler(m_genericResourceImageFilePathsByResourceHash, this)),
    m_saveGenericResourceImageToFileRequestIds(),
#endif
    m_droppedFilePathsAndMimeTypesByReadRequestIds(),
    q_ptr(&noteEditor)
{
    QString initialHtml = m_pagePrefix + "<body></body></html>";
    m_noteEditorPageFolderPath = applicationPersistentStoragePath() + "/NoteEditorPage";
    m_noteEditorPagePath = m_noteEditorPageFolderPath + "/index.html";
    m_noteEditorImageResourcesStoragePath = m_noteEditorPageFolderPath + "/imageResources";

    setupFileIO();

#ifdef USE_QT_WEB_ENGINE
    setupWebSocketServer();
    setupJavaScriptObjects();
#endif

    setupTextCursorPositionJavaScriptHandlerConnections();
    setupNoteEditorPage();
    setupScripts();

    Q_Q(NoteEditor);
    m_resourceLocalFileStorageFolder = ResourceFileStorageManager::resourceFileStorageLocation(q);
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

    Q_Q(NoteEditor);
    GET_PAGE()

    page->executeJavaScript(m_jQueryJs);

#ifndef USE_QT_WEB_ENGINE
    QWebFrame * frame = page->mainFrame();
    if (Q_UNLIKELY(!frame)) {
        return;
    }

    frame->addToJavaScriptWindowObject("resourceCache", m_pResourceInfoJavaScriptHandler,
                                       QScriptEngine::QtOwnership);
    frame->addToJavaScriptWindowObject("textCursorPositionHandler", m_pTextCursorPositionJavaScriptHandler,
                                       QScriptEngine::QtOwnership);
    frame->addToJavaScriptWindowObject("contextMenuEventHandler", m_pContextMenuEventJavaScriptHandler,
                                       QScriptEngine::QtOwnership);

    page->executeJavaScript(m_onResourceInfoReceivedJs);
    page->executeJavaScript(m_qWebKitSetupJs);
#else
    page->executeJavaScript(m_pageMutationObserverJs, /* clear previous queue = */ true);
    page->executeJavaScript(m_qWebChannelJs);
    page->executeJavaScript(QString("(function(){window.websocketserverport = ") +
                            QString::number(m_webSocketServerPort) + QString("})();"));
    page->executeJavaScript(m_onResourceInfoReceivedJs);
    page->executeJavaScript(m_onIconFilePathForIconThemeNameReceivedJs);
    page->executeJavaScript(m_onGenericResourceImageReceivedJs);
    page->executeJavaScript(m_qWebChannelSetupJs);
    page->executeJavaScript(m_provideGenericResourceDisplayNameAndSizeJs);
    page->executeJavaScript(m_provideSrcAndOnClickScriptForEnCryptImgTagsJs);
    page->executeJavaScript(m_provideSrcForGenericResourceImagesJs);
    page->executeJavaScript(m_provideSrcForGenericResourceIconsJs);
    page->executeJavaScript(m_provideSrcForGenericResourceOpenAndSaveIconsJs);
    page->executeJavaScript(m_setupSaveResourceButtonOnClickHandlerJs);
    page->executeJavaScript(m_setupOpenResourceButtonOnClickHandlerJs);
    page->executeJavaScript(m_notifyTextCursorPositionChangedJs);
#endif

    page->executeJavaScript(m_resizableTableColumnsJs);
    page->executeJavaScript(m_onFixedWidthTableResizeJs);
    page->executeJavaScript(m_getSelectionHtmlJs);
    page->executeJavaScript(m_snapSelectionToWordJs);
    page->executeJavaScript(m_replaceSelectionWithHtmlJs);
    page->executeJavaScript(m_provideSrcForResourceImgTagsJs);
    page->executeJavaScript(m_setupEnToDoTagsJs);
    page->executeJavaScript(m_determineStatesForCurrentTextCursorPositionJs);
    page->executeJavaScript(m_determineContextMenuEventTargetJs);

    setPageEditable(true);

    updateColResizableTableBindings();
    saveNoteResourcesToLocalFiles();

#ifdef USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
    provideSrcForGenericResourceOpenAndSaveIcons();
    setupSaveResourceButtonOnClickHandler();
    setupOpenResourceButtonOnClickHandler();
    setupTextCursorPositionTracking();
    setupGenericResourceImages();
#endif

    QNTRACE("Evaluated all JavaScript helper functions");

#ifndef USE_QT_WEB_ENGINE
    m_htmlCachedMemory = q->page()->mainFrame()->toHtml();
    onPageHtmlReceived(m_htmlCachedMemory);
#else
    q->page()->toHtml(HtmlRetrieveFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
#endif
}

void NoteEditorPrivate::onContentChanged()
{
    QNTRACE("NoteEditorPrivate::onContentChanged");
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

    const QString & localGuid = it.value();
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
        updateResourceInfoOnJavaScriptSide();
    }

#ifdef USE_QT_WEB_ENGINE
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
#endif
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
    }
}

void NoteEditorPrivate::onEnCryptElementClicked(QString encryptedText, QString cipher, QString length, QString hint)
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

    Q_Q(NoteEditor);
    QScopedPointer<NoteDecryptionDialog> pDecryptionDialog(new NoteDecryptionDialog(encryptedText,
                                                                                    cipher, hint, keyLength,
                                                                                    m_encryptionManager,
                                                                                    m_decryptedTextManager, q));
    pDecryptionDialog->setWindowModality(Qt::WindowModal);
    QObject::connect(pDecryptionDialog.data(), QNSIGNAL(NoteDecryptionDialog,accepted,QString,QString,bool),
                     this,QNSLOT(NoteEditorPrivate,onEncryptedAreaDecryption,QString,QString,bool));
    QNTRACE("Will exec note decryption dialog now");
    pDecryptionDialog->exec();
    QNTRACE("Executed note decryption dialog");
}

void NoteEditorPrivate::onOpenResourceButtonClicked(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::onOpenResourceButtonClicked: " << resourceHash);

    if (Q_UNLIKELY(!m_pNote)) {
        QString error = QT_TR_NOOP("Can't open resource: no note is set to the editor");
        QNINFO(error << ", resource hash = " << resourceHash);
        emit notifyError(error);
        return;
    }

    int resourceIndex = -1;
    QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
    int numResources = resourceAdapters.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceAdapter & resourceAdapter = resourceAdapters[i];
        if (resourceAdapter.hasDataHash() && (resourceAdapter.dataHash() == resourceHash)) {
            resourceIndex = i;
            break;
        }
    }

    if (Q_UNLIKELY(resourceIndex < 0)) {
        QString error = QT_TR_NOOP("Resource to be opened was not found in the note");
        QNINFO(error << ", resource hash = " << resourceHash);
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

void NoteEditorPrivate::onSaveResourceButtonClicked(const QString & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::onSaveResourceButtonClicked: " << resourceHash);

    if (Q_UNLIKELY(!m_pNote)) {
        QString error = QT_TR_NOOP("Can't save resource: no note is set to the editor");
        QNINFO(error << ", resource hash = " << resourceHash);
        emit notifyError(error);
        return;
    }

    int resourceIndex = -1;
    QList<ResourceAdapter> resourceAdapters = m_pNote->resourceAdapters();
    int numResources = resourceAdapters.size();
    for(int i = 0; i < numResources; ++i)
    {
        const ResourceAdapter & resourceAdapter = resourceAdapters[i];
        if (resourceAdapter.hasDataHash() && (resourceAdapter.dataHash() == resourceHash)) {
            resourceIndex = i;
            break;
        }
    }

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

void NoteEditorPrivate::onJavaScriptLoaded()
{
    QNDEBUG("NoteEditorPrivate::onJavaScriptLoaded");
}

#endif

void NoteEditorPrivate::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNTRACE("NoteEditorPrivate::contextMenuEvent");

    if (Q_UNLIKELY(!pEvent)) {
        QNINFO("detected null pointer to context menu event");
        return;
    }

    if (m_pendingIndexHtmlWritingToFile || m_pendingNotePageLoad) {
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

void NoteEditorPrivate::onContextMenuEventReply(QString contentType, quint64 sequenceNumber)
{
    QNDEBUG("NoteEditorPrivate::onContextMenuEventReply: content type = " << contentType
            << ", sequence number = " << sequenceNumber);

    if (!checkContextMenuSequenceNumber(sequenceNumber)) {
        QNTRACE("Sequence number is not valid, not doing anything");
        return;
    }

    ++m_contextMenuSequenceNumber;

    if (contentType == "GenericText") {
        setupGenericTextContextMenu();
    }
    else if (contentType == "ImageResource") {
        setupImageResourceContextMenu();
    }
    else if (contentType == "NonImageResource") {
        setupNonImageResourceContextMenu();
    }
    else if (contentType == "EncryptedText") {
        setupEncryptedTextContextMenu();
    }
    else {
        QNWARNING("Unknown content type on context menu event reply: " << contentType << ", sequence number " << sequenceNumber);
    }
}

void NoteEditorPrivate::onTextCursorPositionChange()
{
    QNDEBUG("NoteEditorPrivate::onTextCursorPositionChange");
    if (!m_pendingIndexHtmlWritingToFile && !m_pendingNotePageLoad) {
        determineStatesForCurrentTextCursorPosition();
    }
}

void NoteEditorPrivate::onTextCursorBoldStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorBoldStateChanged: " << (state ? "bold" : "not bold"));
    m_currentTextFormattingState.m_bold = state;

    emit textCursorPositionBoldState(state);
}

void NoteEditorPrivate::onTextCursorItalicStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorItalicStateChanged: " << (state ? "italic" : "not italic"));
    m_currentTextFormattingState.m_italic = state;

    emit textCursorPositionItalicState(state);
}

void NoteEditorPrivate::onTextCursorUnderlineStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorUnderlineStateChanged: " << (state ? "underline" : "not underline"));
    m_currentTextFormattingState.m_underline = state;

    emit textCursorPositionUnderlineState(state);
}

void NoteEditorPrivate::onTextCursorStrikethgouthStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorStrikethgouthStateChanged: " << (state ? "strikethrough" : "not strikethrough"));
    m_currentTextFormattingState.m_strikethrough = state;

    emit textCursorPositionStrikethgouthState(state);
}

void NoteEditorPrivate::onTextCursorAlignLeftStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorAlignLeftStateChanged: " << (state ? "true" : "false"));

    if (state) {
        m_currentTextFormattingState.m_alignment = Alignment::Left;
    }

    emit textCursorPositionAlignLeftState(state);
}

void NoteEditorPrivate::onTextCursorAlignCenterStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorAlignCenterStateChanged: " << (state ? "true" : "false"));

    if (state) {
        m_currentTextFormattingState.m_alignment = Alignment::Center;
    }

    emit textCursorPositionAlignCenterState(state);
}

void NoteEditorPrivate::onTextCursorAlignRightStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorAlignRightStateChanged: " << (state ? "true" : "false"));

    if (state) {
        m_currentTextFormattingState.m_alignment = Alignment::Right;
    }

    emit textCursorPositionAlignRightState(state);
}

void NoteEditorPrivate::onTextCursorInsideOrderedListStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorInsideOrderedListStateChanged: " << (state ? "true" : "false"));
    m_currentTextFormattingState.m_insideOrderedList = state;

    emit textCursorPositionInsideOrderedListState(state);
}

void NoteEditorPrivate::onTextCursorInsideUnorderedListStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorInsideUnorderedListStateChanged: " << (state ? "true" : "false"));
    m_currentTextFormattingState.m_insideUnorderedList = state;

    emit textCursorPositionInsideUnorderedListState(state);
}

void NoteEditorPrivate::onTextCursorInsideTableStateChanged(bool state)
{
    QNDEBUG("NoteEditorPrivate::onTextCursorInsideTableStateChanged: " << (state ? "true" : "false"));
    m_currentTextFormattingState.m_insideTable = state;

    emit textCursorPositionInsideTableState(state);
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

        Q_Q(NoteEditor);
        QUrl url = QUrl::fromLocalFile(m_noteEditorPagePath);
        q->load(url);
        QNTRACE("Loaded url: " << url);
        m_pendingNotePageLoad = true;
    }

#ifdef USE_QT_WEB_ENGINE
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
#endif
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
    bool res = m_enmlConverter.noteContentToHtml(m_pNote->content(), m_htmlCachedMemory, m_errorCachedMemory,
                                                 m_decryptedTextManager
#ifndef USE_QT_WEB_ENGINE
                                                 , m_pluginFactory
#endif
                                                 );
    if (!res) {
        QNWARNING("Can't convert note's content to HTML: " << m_errorCachedMemory);
        emit notifyError(m_errorCachedMemory);
        clearEditorContent();
        return;
    }

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
            Q_Q(NoteEditor);
            QWebPage * page = q->page();
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
    QNTRACE("Done setting the current note and notebook");
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

    Q_Q(NoteEditor);
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

    Q_Q(const NoteEditor);

    m_pendingConversionToNote = true;

#ifndef USE_QT_WEB_ENGINE
    m_htmlCachedMemory = q->page()->mainFrame()->toHtml();
    onPageHtmlReceived(m_htmlCachedMemory);
#else
    q->page()->toHtml(HtmlRetrieveFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
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
            updateResourceInfoOnJavaScriptSide();
        }
        return;
    }

    QNTRACE("Scheduled writing of " << numPendingResourceWritesToLocalFiles
            << " to local files, will wait until they are written "
            "and add the src attributes to img resources when the files are ready");
}

void NoteEditorPrivate::updateResourceInfoOnJavaScriptSide()
{
    QNDEBUG("NoteEditorPrivate::updateResourceInfoOnJavaScriptSide");

    Q_Q(NoteEditor);
    GET_PAGE()

    page->executeJavaScript("provideSrcForResourceImgTags();");

#ifdef USE_QT_WEB_ENGINE
    page->executeJavaScript("provideGenericResourceDisplayNameAndSize();");
    page->executeJavaScript("provideSrcForGenericResourceIcons();");
#endif
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

    Q_Q(NoteEditor);
    GET_PAGE()

    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::provideSrcForGenericResourceOpenAndSaveIcons()
{
    QString javascript = "provideSrcForGenericResourceOpenAndSaveIcons();";

    Q_Q(NoteEditor);
    GET_PAGE()

    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::setupSaveResourceButtonOnClickHandler()
{
    QString javascript = "setupSaveResourceButtonOnClickHandler();";

    Q_Q(NoteEditor);
    GET_PAGE()

    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::setupOpenResourceButtonOnClickHandler()
{
    QString javascript = "setupOpenResourceButtonOnClickHandler();";

    Q_Q(NoteEditor);
    GET_PAGE()

    page->executeJavaScript(javascript);
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

    Q_Q(NoteEditor);
    QString absoluteFilePath = QFileDialog::getSaveFileName(q, QObject::tr("Save as..."),
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
    // TODO: implement
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

    Q_Q(NoteEditor);

    // As long as QWebEnginePage doesn't offer a convenience signal on its content change,
    // need to reinvent the wheel using JavaScript
    QObject::connect(m_pPageMutationHandler, &PageMutationHandler::contentsChanged,
                     q, &NoteEditor::contentChanged);
    QObject::connect(m_pPageMutationHandler, &PageMutationHandler::contentsChanged,
                     this, &NoteEditorPrivate::onContentChanged);

    m_pMimeTypeIconJavaScriptHandler = new MimeTypeIconJavaScriptHandler(m_noteEditorPageFolderPath,
                                                                         m_pIOThread, this);

    m_pIconThemeJavaScriptHandler = new IconThemeJavaScriptHandler(m_noteEditorPageFolderPath,
                                                                   m_pIOThread, this);

    QObject::connect(m_pEnCryptElementClickHandler, &EnCryptElementOnClickHandler::decrypt,
                     this, &NoteEditorPrivate::onEnCryptElementClicked);

    QObject::connect(m_pGenericResourceOpenAndSaveButtonsOnClickHandler, &GenericResourceOpenAndSaveButtonsOnClickHandler::saveResourceRequest,
                     this, &NoteEditorPrivate::onSaveResourceButtonClicked);

    QObject::connect(m_pGenericResourceOpenAndSaveButtonsOnClickHandler, &GenericResourceOpenAndSaveButtonsOnClickHandler::openResourceRequest,
                     this, &NoteEditorPrivate::onOpenResourceButtonClicked);

    QObject::connect(m_pTextCursorPositionJavaScriptHandler, &TextCursorPositionJavaScriptHandler::textCursorPositionChanged,
                     this, &NoteEditorPrivate::onTextCursorPositionChange);

    QObject::connect(m_pContextMenuEventJavaScriptHandler, &ContextMenuEventJavaScriptHandler::contextMenuEventReply,
                     this, &NoteEditorPrivate::onContextMenuEventReply);

    m_pWebChannel->registerObject("resourceCache", m_pResourceInfoJavaScriptHandler);
    m_pWebChannel->registerObject("enCryptElementClickHandler", m_pEnCryptElementClickHandler);
    m_pWebChannel->registerObject("pageMutationObserver", m_pPageMutationHandler);
    m_pWebChannel->registerObject("mimeTypeIconHandler", m_pMimeTypeIconJavaScriptHandler);
    m_pWebChannel->registerObject("iconThemeHandler", m_pIconThemeJavaScriptHandler);
    m_pWebChannel->registerObject("openAndSaveResourceButtonsHandler", m_pGenericResourceOpenAndSaveButtonsOnClickHandler);
    m_pWebChannel->registerObject("textCursorPositionHandler", m_pTextCursorPositionJavaScriptHandler);
    m_pWebChannel->registerObject("contextMenuEventHandler", m_pContextMenuEventJavaScriptHandler);
    m_pWebChannel->registerObject("genericResourceImageHandler", m_pGenericResoureImageJavaScriptHandler);
    QNDEBUG("Registered objects exposed to JavaScript");
}

void NoteEditorPrivate::setupTextCursorPositionTracking()
{
    QNDEBUG("NoteEditorPrivate::setupTextCursorPositionTracking");

    QString javascript = "document.body.onkeyup = notifyTextCursorPositionChanged; "
                         "document.body.onmouseup = notifyTextCursorPositionChanged;";

    Q_Q(NoteEditor);
    GET_PAGE()

    page->executeJavaScript(javascript);
}

#endif

void NoteEditorPrivate::setupGenericTextContextMenu()
{
    QNDEBUG("NoteEditorPrivate::setupGenericTextContextMenu");

    Q_Q(NoteEditor);

    delete m_pGenericTextContextMenu;
    m_pGenericTextContextMenu = new QMenu(q);

#define ADD_ACTION(name, menu, slot) \
    { \
        QAction * action = new QAction(QObject::tr(#name), menu); \
        menu->addAction(action); \
        QObject::connect(action, QNSIGNAL(QAction,triggered), q, QNSLOT(NoteEditor,slot)); \
    }

#define ADD_ACTION_WITH_SHORTCUT(key, name, menu, slot) \
    { \
        QAction * action = new QAction(QObject::tr(#key), menu); \
        setupActionShortcut(#key, *action); \
        menu->addAction(action); \
        QObject::connect(action, QNSIGNAL(QAction,triggered), q, QNSLOT(NoteEditor,slot)); \
    }

    ADD_ACTION_WITH_SHORTCUT(Cut, Cut, m_pGenericTextContextMenu, cut);
    ADD_ACTION_WITH_SHORTCUT(Copy, Copy, m_pGenericTextContextMenu, copy);

    QClipboard * pClipboard = QApplication::clipboard();
    if (pClipboard && pClipboard->mimeData(QClipboard::Clipboard)) {
        QNTRACE("Clipboard buffer has something, adding paste actions");
        ADD_ACTION_WITH_SHORTCUT(Paste, Paste, m_pGenericTextContextMenu, paste);
        ADD_ACTION_WITH_SHORTCUT(Paste unformatted, Paste as unformatted text, m_pGenericTextContextMenu, pasteUnformatted);
    }

    Q_UNUSED(m_pGenericTextContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(Font, Font..., m_pGenericTextContextMenu, fontMenu);
    QMenu * pStyleSubMenu = m_pGenericTextContextMenu->addMenu(QObject::tr("Style"));
    ADD_ACTION_WITH_SHORTCUT(Bold, Bold, pStyleSubMenu, textBold);
    ADD_ACTION_WITH_SHORTCUT(Italic, Italic, pStyleSubMenu, textItalic);
    ADD_ACTION_WITH_SHORTCUT(Underline, Underline, pStyleSubMenu, textUnderline);
    ADD_ACTION_WITH_SHORTCUT(Strikethrough, Strikethrough, pStyleSubMenu, textStrikethrough);
    // TODO: continue filling the menu items

    m_pGenericTextContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupImageResourceContextMenu()
{
    QNDEBUG("NoteEditorPrivate::setupImageResourceContextMenu");

    Q_Q(NoteEditor);

    delete m_pImageResourceContextMenu;
    m_pImageResourceContextMenu = new QMenu(q);

    ADD_ACTION_WITH_SHORTCUT(Cut, Cut, m_pImageResourceContextMenu, cut);
    ADD_ACTION_WITH_SHORTCUT(Copy, Copy, m_pImageResourceContextMenu, copy);

    QClipboard * pClipboard = QApplication::clipboard();
    if (pClipboard && pClipboard->mimeData(QClipboard::Clipboard)) {
        QNTRACE("Clipboard buffer has something, adding paste action");
        ADD_ACTION_WITH_SHORTCUT(Paste, Paste, m_pImageResourceContextMenu, paste);
    }

    // TODO: continue filling the menu items

    m_pImageResourceContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupNonImageResourceContextMenu()
{
    QNDEBUG("NoteEditorPrivate::setupNonImageResourceContextMenu");

    Q_Q(NoteEditor);

    delete m_pNonImageResourceContextMenu;
    m_pNonImageResourceContextMenu = new QMenu(q);

    ADD_ACTION_WITH_SHORTCUT(Cut, Cut, m_pNonImageResourceContextMenu, cut);
    ADD_ACTION_WITH_SHORTCUT(Copy, Copy, m_pNonImageResourceContextMenu, copy);

    QClipboard * pClipboard = QApplication::clipboard();
    if (pClipboard && pClipboard->mimeData(QClipboard::Clipboard)) {
        QNTRACE("Clipboard buffer has something, adding paste action");
        ADD_ACTION_WITH_SHORTCUT(Paste, Paste, m_pNonImageResourceContextMenu, paste);
    }

    // TODO: continue filling the menu items

    m_pNonImageResourceContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupEncryptedTextContextMenu()
{
    QNDEBUG("NoteEditorPrivate::setupEncryptedTextContextMenu");

    Q_Q(NoteEditor);

    delete m_pEncryptedTextContextMenu;
    m_pEncryptedTextContextMenu = new QMenu(q);

    ADD_ACTION_WITH_SHORTCUT(Cut, Cut, m_pEncryptedTextContextMenu, cut);
    ADD_ACTION_WITH_SHORTCUT(Copy, Copy, m_pEncryptedTextContextMenu, copy);

    QClipboard * pClipboard = QApplication::clipboard();
    if (pClipboard && pClipboard->mimeData(QClipboard::Clipboard)) {
        QNTRACE("Clipboard buffer has something, adding paste action");
        ADD_ACTION_WITH_SHORTCUT(Paste, Paste, m_pEncryptedTextContextMenu, paste);
    }

    // TODO: continue filling the menu items

    m_pEncryptedTextContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupActionShortcut(const QString & key, QAction & action)
{
    QNDEBUG("NoteEditorPrivate::setupActionShortcut: key = " << key);

    ApplicationSettings appSettings;
    QString shortcut = appSettings.value(key).toString();
    if (!shortcut.isEmpty()) {
        action.setShortcut(QKeySequence(shortcut));
        return;
    }

    if (key == "Cut") {
        action.setShortcut(Qt::CTRL + Qt::Key_X);
    }
    else if (key == "Copy") {
        action.setShortcut(Qt::CTRL + Qt::Key_C);
    }
    else if (key == "Paste") {
        action.setShortcut(Qt::CTRL + Qt::Key_V);
    }
    else if (key == "Paste unformatted") {
        action.setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_V);
    }
    else if (key == "Font") {
        action.setShortcut(Qt::CTRL + Qt::Key_D);
    }
    else if (key == "Bold") {
        action.setShortcut(Qt::CTRL + Qt::Key_B);
    }
    else if (key == "Italic") {
        action.setShortcut(Qt::CTRL + Qt::Key_I);
    }
    else if (key == "Underline") {
        action.setShortcut(Qt::CTRL + Qt::Key_U);
    }
    else if (key == "Strikethrough") {
        action.setShortcut(Qt::CTRL + Qt::Key_T);
    }
    else if (key == "Justify left") {
        action.setShortcut(Qt::CTRL + Qt::Key_L);
    }
    else if (key == "Justify center") {
        action.setShortcut(Qt::CTRL + Qt::Key_E);
    }
    else if (key == "Justify right") {
        action.setShortcut(Qt::CTRL + Qt::Key_R);
    }
    else if (key == "Justify width") {
        action.setShortcut(Qt::CTRL + Qt::Key_J);
    }
    else if (key == "Indent") {
        action.setShortcut(Qt::CTRL + Qt::Key_M);
    }
    else if (key == "Unindent") {
        action.setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_M);
    }
    else if (key == "Numbered list") {
        action.setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_N);
    }
    else if (key == "Bulleted list") {
        action.setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_B);
    }
    else if (key == "Horizontal line") {
        action.setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Minus);
    }
    else if (key == "ToDo") {
        action.setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_C);
    }
    else if (key == "Add hyperlink") {
        action.setShortcut(Qt::CTRL + Qt::Key_K);
    }
    else if (key == "Edit hyperlink") {
        action.setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_K);
    }
    else if (key == "Remove hyperlink") {
        action.setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_R);
    }
    else if (key == "Encrypt") {
        action.setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_X);
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
#ifdef USE_QT_WEB_ENGINE
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,saveResourceToFile,QString,QByteArray,QUuid),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid));
#endif
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
    SETUP_SCRIPT("javascript/colResizable/colResizable-1.5.min.js", m_resizableTableColumnsJs);
    SETUP_SCRIPT("javascript/scripts/onFixedWidthTableResize.js", m_onFixedWidthTableResizeJs);
    SETUP_SCRIPT("javascript/scripts/getSelectionHtml.js", m_getSelectionHtmlJs);
    SETUP_SCRIPT("javascript/scripts/snapSelectionToWord.js", m_snapSelectionToWordJs);
    SETUP_SCRIPT("javascript/scripts/replaceSelectionWithHtml.js", m_replaceSelectionWithHtmlJs);
    SETUP_SCRIPT("javascript/scripts/provideSrcForResourceImgTags.js", m_provideSrcForResourceImgTagsJs);
    SETUP_SCRIPT("javascript/scripts/provideGenericResourceDisplayNameAndSize.js", m_provideGenericResourceDisplayNameAndSizeJs);
    SETUP_SCRIPT("javascript/scripts/enToDoTagsSetup.js", m_setupEnToDoTagsJs);
    SETUP_SCRIPT("javascript/scripts/onResourceInfoReceived.js", m_onResourceInfoReceivedJs);
    SETUP_SCRIPT("javascript/scripts/determineStatesForCurrentTextCursorPosition.js", m_determineStatesForCurrentTextCursorPositionJs);
    SETUP_SCRIPT("javascript/scripts/determineContextMenuEventTarget.js", m_determineContextMenuEventTargetJs);

#ifndef USE_QT_WEB_ENGINE
    SETUP_SCRIPT("javascript/scripts/qWebKitSetup.js", m_qWebKitSetupJs);
#else
    SETUP_SCRIPT("qtwebchannel/qwebchannel.js", m_qWebChannelJs);
    SETUP_SCRIPT("javascript/scripts/qWebChannelSetup.js", m_qWebChannelSetupJs);
    SETUP_SCRIPT("javascript/scripts/pageMutationObserver.js", m_pageMutationObserverJs);
    SETUP_SCRIPT("javascript/scripts/provideSrcAndOnClickScriptForEnCryptImgTags.js", m_provideSrcAndOnClickScriptForEnCryptImgTagsJs);
    SETUP_SCRIPT("javascript/scripts/provideSrcForGenericResourceImages.js", m_provideSrcForGenericResourceImagesJs);
    SETUP_SCRIPT("javascript/scripts/onGenericResourceImageReceived.js", m_onGenericResourceImageReceivedJs);
    SETUP_SCRIPT("javascript/scripts/provideSrcForGenericResourceIcons.js", m_provideSrcForGenericResourceIconsJs);
    SETUP_SCRIPT("javascript/scripts/onIconFilePathForIconThemeNameReceived.js", m_onIconFilePathForIconThemeNameReceivedJs);
    SETUP_SCRIPT("javascript/scripts/provideSrcForGenericResourceOpenAndSaveIcons.js", m_provideSrcForGenericResourceOpenAndSaveIconsJs);
    SETUP_SCRIPT("javascript/scripts/setupSaveResourceButtonOnClickHandler.js", m_setupSaveResourceButtonOnClickHandlerJs);
    SETUP_SCRIPT("javascript/scripts/setupOpenResourceButtonOnClickHandler.js", m_setupOpenResourceButtonOnClickHandlerJs);
    SETUP_SCRIPT("javascript/scripts/notifyTextCursorPositionChanged.js", m_notifyTextCursorPositionChangedJs);
#endif

#undef SETUP_SCRIPT
}

void NoteEditorPrivate::setupNoteEditorPage()
{
    QNDEBUG("NoteEditorPrivate::setupNoteEditorPage");

    Q_Q(NoteEditor);

    NoteEditorPage * page = new NoteEditorPage(*q);
    page->settings()->setAttribute(WebSettings::LocalContentCanAccessFileUrls, true);
    page->settings()->setAttribute(WebSettings::LocalContentCanAccessRemoteUrls, true);

#ifndef USE_QT_WEB_ENGINE
    page->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    page->setContentEditable(true);

    QObject::connect(page, QNSIGNAL(NoteEditorPage,contentsChanged), q, QNSIGNAL(NoteEditor,contentChanged));
    QObject::connect(page, QNSIGNAL(NoteEditorPage,contentsChanged), this, QNSLOT(NoteEditorPrivate,onContentChanged));

    page->mainFrame()->addToJavaScriptWindowObject("resourceCache", m_pResourceInfoJavaScriptHandler,
                                                   QScriptEngine::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject("textCursorPositionHandler", m_pTextCursorPositionJavaScriptHandler,
                                                   QScriptEngine::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject("contextMenuEventHandler", m_pContextMenuEventJavaScriptHandler,
                                                   QScriptEngine::QtOwnership);

    EncryptedAreaPlugin * encryptedAreaPlugin = new EncryptedAreaPlugin(m_encryptionManager, m_decryptedTextManager);
    m_pluginFactory = new NoteEditorPluginFactory(*q, *m_pResourceFileStorageManager, *m_pFileIOThreadWorker,
                                                  encryptedAreaPlugin, page);
    page->setPluginFactory(m_pluginFactory);

    m_errorCachedMemory.resize(0);

    QObject::connect(page, QNSIGNAL(NoteEditor,microFocusChanged), this, QNSLOT(NoteEditorPrivate,onTextCursorPositionChange));
#endif

    QObject::connect(m_pContextMenuEventJavaScriptHandler, QNSIGNAL(ContextMenuEventJavaScriptHandler,contextMenuEventReply,QString,quint64),
                     this, QNSLOT(NoteEditorPrivate,onContextMenuEventReply,QString,quint64));
    QObject::connect(q, QNSIGNAL(NoteEditor,loadFinished,bool), this, QNSLOT(NoteEditorPrivate,onNoteLoadFinished,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,notifyError,QString), q, QNSIGNAL(NoteEditor,notifyError,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note), q, QNSIGNAL(NoteEditor,convertedToNote,Note));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,cantConvertToNote,QString), q, QNSIGNAL(NoteEditor,cantConvertToNote,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,noteEditorHtmlUpdated,QString), q, QNSIGNAL(NoteEditor,noteEditorHtmlUpdated,QString));

    q->setPage(page);
    q->setAcceptDrops(true);
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
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionStrikethgouthState,bool),
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

    // Connect signals to signals of public class
    Q_Q(NoteEditor);

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionBoldState,bool), q, QNSIGNAL(NoteEditor,textBoldState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionItalicState,bool), q, QNSIGNAL(NoteEditor,textItalicState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionUnderlineState,bool), q, QNSIGNAL(NoteEditor,textUnderlineState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionStrikethgouthState,bool), q, QNSIGNAL(NoteEditor,textStrikethroughState,bool));

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionAlignLeftState,bool), q, QNSIGNAL(NoteEditor,textAlignLeftState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionAlignCenterState,bool), q, QNSIGNAL(NoteEditor,textAlignCenterState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionAlignRightState,bool), q, QNSIGNAL(NoteEditor,textAlignRightState,bool));

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionInsideOrderedListState,bool), q, QNSIGNAL(NoteEditor,textInsideOrderedListState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionInsideUnorderedListState,bool), q, QNSIGNAL(NoteEditor,textInsideUnorderedListState,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,textCursorPositionInsideTableState,bool), q, QNSIGNAL(NoteEditor,textInsideTableState,bool));
}

void NoteEditorPrivate::determineStatesForCurrentTextCursorPosition()
{
    QNDEBUG("NoteEditorPrivate::determineStatesForCurrentTextCursorPosition");

    QString javascript = "determineStatesForCurrentTextCursorPosition();";

    Q_Q(NoteEditor);
    GET_PAGE()

    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::determineContextMenuEventTarget()
{
    QNDEBUG("NoteEditorPrivate::determineContextMenuEventTarget");

    QString javascript = "determineContextMenuEventTarget(" + QString::number(m_contextMenuSequenceNumber) +
                         ", " + QString::number(m_lastContextMenuEventPagePos.x()) + ", " +
                         QString::number(m_lastContextMenuEventPagePos.y()) + ");";

    Q_Q(NoteEditor);
    GET_PAGE()

    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::setPageEditable(const bool editable)
{
    QNTRACE("NoteEditorPrivate::setPageEditable: " << (editable ? "true" : "false"));

    Q_Q(NoteEditor);
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

    m_htmlCachedMemory = html;
    m_enmlCachedMemory.resize(0);
    m_errorCachedMemory.resize(0);
    bool res = m_enmlConverter.htmlToNoteContent(m_htmlCachedMemory, m_enmlCachedMemory,
                                                 m_decryptedTextManager, m_errorCachedMemory);
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

void NoteEditorPrivate::onPageSelectedHtmlForEncryptionReceived(const QVariant & selectedHtmlData,
                                                                const QVector<QPair<QString, QString> > & extraData)
{
    QString selectedHtml = selectedHtmlData.toString();

    if (selectedHtml.isEmpty()) {
        QNDEBUG("Note editor page has no selected text, nothing to encrypt");
        return;
    }

    QString passphrase;
    QString hint;
    typedef QVector<QPair<QString,QString> >::const_iterator CIter;
    CIter extraDataEnd = extraData.constEnd();
    for(CIter it = extraData.constBegin(); it != extraDataEnd; ++it)
    {
        const QPair<QString,QString> & itemPair = *it;
        if (itemPair.first == "passphrase") {
            passphrase = itemPair.second;
        }
        else if (itemPair.first == "hint") {
            hint = itemPair.second;
        }
    }

    if (passphrase.isEmpty()) {
        m_errorCachedMemory = QT_TR_NOOP("Internal error: passphrase was either not found within extra data "
                                         "passed along with the selected HTML for encryption or it was passed but is empty");
        QNWARNING(m_errorCachedMemory << ", extra data: " << extraData);
        emit notifyError(m_errorCachedMemory);
        return;
    }

    QString error;
    QString encryptedText;
    QString cipher = "AES";
    size_t keyLength = 128;
    bool res = m_encryptionManager->encrypt(selectedHtml, passphrase, cipher, keyLength,
                                            encryptedText, error);
    if (!res) {
        error.prepend(QT_TR_NOOP("Can't encrypt selected text: "));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString encryptedTextHtmlObject = "<object type=\"application/octet-stream\" en-tag=\"en-crypt\" >"
                                      "<param name=\"cipher\" value=\"";
    encryptedTextHtmlObject += cipher;
    encryptedTextHtmlObject += "\" />";
    encryptedTextHtmlObject += "<param name=\"length\" value=\"";
    encryptedTextHtmlObject += "\" />";
    encryptedTextHtmlObject += "<param name=\"encryptedText\" value=\"";
    encryptedTextHtmlObject += encryptedText;
    encryptedTextHtmlObject += "\" />";

    if (!hint.isEmpty())
    {
        encryptedTextHtmlObject = "<param name=\"hint\" value=\"";

        QString hintWithEscapedDoubleQuotes = hint;
        for(int i = 0; i < hintWithEscapedDoubleQuotes.size(); ++i)
        {
            if (hintWithEscapedDoubleQuotes.at(i) == QChar('"'))
            {
                if (i == 0) {
                    hintWithEscapedDoubleQuotes.insert(i, QChar('\\'));
                }
                else if (hintWithEscapedDoubleQuotes.at(i-1) != QChar('\\')) {
                    hintWithEscapedDoubleQuotes.insert(i, QChar('\\'));
                }
            }
        }

        encryptedTextHtmlObject += hintWithEscapedDoubleQuotes;
        encryptedTextHtmlObject += "\" />";
    }

    encryptedTextHtmlObject += "<object/>";

    QString javascript = QString("replaceSelectionWithHtml('%1');").arg(encryptedTextHtmlObject);
    // TODO: for QtWebKit: see whether contentChanged signal would be emitted automatically

    Q_Q(NoteEditor);
    GET_PAGE()
    page->executeJavaScript(javascript);
}

#define COMMAND_TO_JS(command) \
    QString javascript = QString("document.execCommand(\"%1\", false, null)").arg(command)

#define COMMAND_WITH_ARGS_TO_JS(command, args) \
    QString javascript = QString("document.execCommand('%1', false, '%2')").arg(command).arg(args)

#ifndef USE_QT_WEB_ENGINE
QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command)
{
    Q_Q(NoteEditor);
    COMMAND_TO_JS(command);
    QWebFrame * frame = q->page()->mainFrame();
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript << ", result = " << result.toString());
    return result;
}

QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command, const QString & args)
{
    Q_Q(NoteEditor);
    COMMAND_WITH_ARGS_TO_JS(command, args);
    QWebFrame * frame = q->page()->mainFrame();
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE("Executed javascript command: " << javascript << ", result = " << result.toString());
    return result;
}
#endif

void NoteEditorPrivate::execJavascriptCommand(const QString & command)
{
    COMMAND_TO_JS(command);

    Q_Q(NoteEditor);
    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::execJavascriptCommand(const QString & command, const QString & args)
{
    COMMAND_WITH_ARGS_TO_JS(command, args);
    Q_Q(NoteEditor);
    GET_PAGE()
    page->executeJavaScript(javascript);
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
}

const Note * NoteEditorPrivate::getNote()
{
    QNDEBUG("NoteEditorPrivate::getNote");

    if (!m_pNote) {
        QNTRACE("No note was set to the editor");
        return Q_NULLPTR;
    }

    if (m_modified)
    {
        QNTRACE("Note editor's content was modified, converting into note");

        bool res = htmlToNoteContent(m_errorCachedMemory);
        if (!res) {
            return Q_NULLPTR;
        }

        m_modified = false;
    }

    return m_pNote;
}

const Notebook * NoteEditorPrivate::getNotebook() const
{
    return m_pNotebook;
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

void NoteEditorPrivate::onDropEvent(QDropEvent *pEvent)
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
    resource.setDataHash(dataHash);
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

void NoteEditorPrivate::insertToDoCheckbox()
{
    QString html = ENMLConverter::getToDoCheckboxHtml(/* checked = */ false);
    QString javascript = QString("document.execCommand('insertHtml', false, '%1'); ").arg(html);
    javascript += m_setupEnToDoTagsJs;

    Q_Q(NoteEditor);
    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::setFont(const QFont & font)
{
    m_font = font;
    QString fontName = font.family();
    execJavascriptCommand("fontName", fontName);
}

void NoteEditorPrivate::setFontHeight(const int height)
{
    if (height > 0) {
        m_font.setPointSize(height);
        execJavascriptCommand("fontSize", QString::number(height));
    }
    else {
        QString error = QT_TR_NOOP("Detected incorrect font size: " + QString::number(height));
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::setFontColor(const QColor & color)
{
    if (color.isValid()) {
        m_fontColor = color;
        execJavascriptCommand("foreColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid font color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::setBackgroundColor(const QColor & color)
{
    if (color.isValid()) {
        m_backgroundColor = color;
        execJavascriptCommand("hiliteColor", color.name());
    }
    else {
        QString error = QT_TR_NOOP("Detected invalid background color: " + color.name());
        QNINFO(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::insertHorizontalLine()
{
    execJavascriptCommand("insertHorizontalRule");
}

void NoteEditorPrivate::changeIndentation(const bool increase)
{
    execJavascriptCommand((increase ? "indent" : "outdent"));
}

void NoteEditorPrivate::insertBulletedList()
{
    execJavascriptCommand("insertUnorderedList");
}

void NoteEditorPrivate::insertNumberedList()
{
    execJavascriptCommand("insertOrderedList");
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
    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    Q_Q(NoteEditor);
    int pageWidth = q->geometry().width();
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
}

void NoteEditorPrivate::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
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
}

void NoteEditorPrivate::encryptSelectedText(const QString & passphrase,
                                            const QString & hint)
{
    QNDEBUG("NoteEditorPrivate::encryptSelectedText");

    Q_Q(NoteEditor);
    QVector<QPair<QString,QString> > extraData;
    extraData << QPair<QString,QString>("passphrase",passphrase) << QPair<QString,QString>("hint",hint);

#ifndef USE_QT_WEB_ENGINE
    if (!q->page()->hasSelection()) {
        QNINFO("Note editor page has no selected text, nothing to encrypt");
        return;
    }

    // NOTE: use JavaScript to get the selected text here, not the convenience method of NoteEditorPage
    // in order to ensure the method returning the selected html and the method replacing the selected html
    // agree about the selected html
    QString selectedHtml = execJavascriptCommandWithResult("getSelectionHtml").toString();
    if (selectedHtml.isEmpty()) {
        QNINFO("Selected html is empty, nothing to encrypt");
        return;
    }

    onPageSelectedHtmlForEncryptionReceived(selectedHtml, extraData);
#else
    q->page()->runJavaScript("getSelectionHtml", HtmlRetrieveFunctor<QVariant>(this, &NoteEditorPrivate::onPageSelectedHtmlForEncryptionReceived, extraData));
#endif
}

void NoteEditorPrivate::onEncryptedAreaDecryption(QString encryptedText, QString decryptedText, bool rememberForSession)
{
    QNDEBUG("NoteEditorPrivate::onEncryptedAreaDecryption");
    Q_UNUSED(encryptedText)
    Q_UNUSED(decryptedText)
    Q_UNUSED(rememberForSession)
    noteToEditorContent();
}

void NoteEditorPrivate::onNoteLoadCancelled()
{
    Q_Q(NoteEditor);
    q->stop();
    QNINFO("Note load has been cancelled");
    // TODO: add some overlay widget for NoteEditor to properly indicate visually that the note load has been cancelled
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
    Q_INIT_RESOURCE(scripts);

    QNDEBUG("Initialized NoteEditor's resources");
}
