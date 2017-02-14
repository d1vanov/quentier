/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteEditor_p.h"
#include "GenericResourceImageManager.h"
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
#include "javascript_glue/ResizableImageJavaScriptHandler.h"
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
#include "undo_stack/ImageResizeUndoCommand.h"
#include "undo_stack/ReplaceUndoCommand.h"
#include "undo_stack/ReplaceAllUndoCommand.h"
#include "undo_stack/SpellCorrectionUndoCommand.h"
#include "undo_stack/SpellCheckIgnoreWordUndoCommand.h"
#include "undo_stack/SpellCheckAddToUserWordListUndoCommand.h"
#include "undo_stack/TableActionUndoCommand.h"
#include <quentier/utility/ApplicationSettings.h>
#include <quentier/note_editor/SpellChecker.h>

#ifndef QUENTIER_USE_QT_WEB_ENGINE
#include <QWebFrame>
#include <QWebPage>
typedef QWebSettings WebSettings;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    typedef QScriptEngine OwnershipNamespace;
#else
    typedef QWebFrame OwnershipNamespace;
#endif
#else
#include "javascript_glue/EnCryptElementOnClickHandler.h"
#include "javascript_glue/GenericResourceOpenAndSaveButtonsOnClickHandler.h"
#include "javascript_glue/GenericResourceImageJavaScriptHandler.h"
#include "javascript_glue/HyperlinkClickJavaScriptHandler.h"
#include "WebSocketClientWrapper.h"
#include "WebSocketTransport.h"
#include <quentier/utility/DesktopServices.h>
#include <QPainter>
#include <QIcon>
#include <QFontMetrics>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebChannel>
#include <QWebEngineSettings>
typedef QWebEngineSettings WebSettings;
#endif

#include <quentier/note_editor/ResourceFileStorageManager.h>
#include <quentier/exception/NoteEditorInitializationException.h>
#include <quentier/exception/NoteEditorPluginInitializationException.h>
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <quentier/types/Resource.h>
#include <quentier/types/ResourceRecognitionIndexItem.h>
#include <quentier/types/Account.h>
#include <quentier/enml/ENMLConverter.h>
#include <quentier/utility/Utility.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/FileIOThreadWorker.h>
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/utility/ShortcutManager.h>
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
        QNFATAL(QStringLiteral("Can't get access to note editor's underlying page!")); \
        return; \
    }

#define CHECK_NOTE_EDITABLE(message) \
    if (Q_UNLIKELY(!isPageEditable())) { \
        ErrorString error(message); \
        error.additionalBases().append(QT_TRANSLATE_NOOP("", "note is not editable")); \
        QNINFO(error << QStringLiteral(", note: ") << (m_pNote.isNull() ? QStringLiteral("<null>") : m_pNote->toString()) \
               << QStringLiteral("\nNotebook: ") << (m_pNotebook.isNull() ? QStringLiteral("<null>") : m_pNotebook->toString())); \
        emit notifyError(error); \
        return; \
    }

namespace quentier {

NoteEditorPrivate::NoteEditorPrivate(NoteEditor & noteEditor) :
    INoteEditorBackend(&noteEditor),
    m_noteEditorPageFolderPath(),
    m_noteEditorImageResourcesStoragePath(),
    m_genericResourceImageFileStoragePath(),
    m_font(),
    m_jQueryJs(),
    m_jQueryUiJs(),
    m_resizableTableColumnsJs(),
    m_resizableImageManagerJs(),
    m_debounceJs(),
    m_rangyCoreJs(),
    m_rangySelectionSaveRestoreJs(),
    m_onTableResizeJs(),
    m_selectionManagerJs(),
    m_textEditingUndoRedoManagerJs(),
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
    m_managedPageActionJs(),
#ifndef QUENTIER_USE_QT_WEB_ENGINE
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
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("QWebChannel server"), QWebSocketServer::NonSecureMode, this)),
    m_pWebSocketClientWrapper(new WebSocketClientWrapper(m_pWebSocketServer, this)),
    m_pWebChannel(new QWebChannel(this)),
    m_pEnCryptElementClickHandler(new EnCryptElementOnClickHandler(this)),
    m_pGenericResourceOpenAndSaveButtonsOnClickHandler(new GenericResourceOpenAndSaveButtonsOnClickHandler(this)),
    m_pHyperlinkClickJavaScriptHandler(new HyperlinkClickJavaScriptHandler(this)),
    m_webSocketServerPort(0),
#endif
    m_pSpellCheckerDynamicHandler(new SpellCheckerDynamicHelper(this)),
    m_pTableResizeJavaScriptHandler(new TableResizeJavaScriptHandler(this)),
    m_pResizableImageJavaScriptHandler(new ResizableImageJavaScriptHandler(this)),
    m_pGenericResourceImageManager(Q_NULLPTR),
    m_pToDoCheckboxClickHandler(new ToDoCheckboxOnClickHandler(this)),
    m_pPageMutationHandler(new PageMutationHandler(this)),
    m_pUndoStack(Q_NULLPTR),
    m_pAccount(),
    m_blankPageHtml(),
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
    m_pNote(),
    m_pNotebook(),
    m_modified(false),
    m_watchingForContentChange(false),
    m_contentChangedSinceWatchingStart(false),
    m_secondsToWaitBeforeConversionStart(30),
    m_pageToNoteContentPostponeTimerId(0),
    m_encryptionManager(new EncryptionManager),
    m_decryptedTextManager(new DecryptedTextManager),
    m_enmlConverter(),
#ifndef QUENTIER_USE_QT_WEB_ENGINE
    m_pPluginFactory(Q_NULLPTR),
#endif
    m_pGenericTextContextMenu(Q_NULLPTR),
    m_pImageResourceContextMenu(Q_NULLPTR),
    m_pNonImageResourceContextMenu(Q_NULLPTR),
    m_pEncryptedTextContextMenu(Q_NULLPTR),
    m_pSpellChecker(Q_NULLPTR),
    m_spellCheckerEnabled(false),
    m_currentNoteMisSpelledWords(),
    m_stringUtils(),
    m_pagePrefix(QStringLiteral("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
                                "<html><head>"
                                "<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"UTF-8\" />"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/jquery-ui.min.css\">"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-crypt.css\">"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/hover.css\">"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-decrypted.css\">"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-media-generic.css\">"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-media-image.css\">"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/image-area-hilitor.css\">"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/en-todo.css\">"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/link.css\">"
                                "<link rel=\"stylesheet\" type=\"text/css\" href=\"qrc:/css/misspell.css\">"
                                "<title></title></head>")),
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
    m_pResourceFileStorageManager(Q_NULLPTR),
    m_pFileIOThreadWorker(Q_NULLPTR),
    m_resourceInfo(),
    m_pResourceInfoJavaScriptHandler(new ResourceInfoJavaScriptHandler(m_resourceInfo, this)),
    m_resourceLocalFileStorageFolder(),
    m_genericResourceLocalUidBySaveToStorageRequestIds(),
    m_imageResourceSaveToStorageRequestIds(),
    m_resourceFileStoragePathsByResourceLocalUid(),
    m_localUidsOfResourcesWantedToBeSaved(),
    m_localUidsOfResourcesWantedToBeOpened(),
    m_manualSaveResourceToFileRequestIds(),
    m_fileSuffixesForMimeType(),
    m_fileFilterStringForMimeType(),
    m_genericResourceImageFilePathsByResourceHash(),
#ifdef QUENTIER_USE_QT_WEB_ENGINE
    m_pGenericResoureImageJavaScriptHandler(new GenericResourceImageJavaScriptHandler(m_genericResourceImageFilePathsByResourceHash, this)),
#endif
    m_saveGenericResourceImageToFileRequestIds(),
    m_recognitionIndicesByResourceHash(),
    m_currentContextMenuExtraData(),
    m_resourceLocalUidAndFileStoragePathByReadResourceRequestIds(),
    m_lastFreeEnToDoIdNumber(1),
    m_lastFreeHyperlinkIdNumber(1),
    m_lastFreeEnCryptIdNumber(1),
    m_lastFreeEnDecryptedIdNumber(1),
    q_ptr(&noteEditor)
{
    setupSkipRulesForHtmlToEnmlConversion();

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    setupWebSocketServer();
    setupJavaScriptObjects();
#endif

    setupTextCursorPositionJavaScriptHandlerConnections();
    setupGeneralSignalSlotConnections();
    setupScripts();
    setAcceptDrops(false);
}

NoteEditorPrivate::~NoteEditorPrivate()
{}

void NoteEditorPrivate::setBlankPageHtml(const QString & html)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setBlankPageHtml: ") << html);

    m_blankPageHtml = html;

    if (!m_pNote || !m_pNotebook) {
        clearEditorContent();
    }
}

bool NoteEditorPrivate::isNoteLoaded() const
{
    if (!m_pNote || !m_pNotebook) {
        return false;
    }

    return !m_pendingNotePageLoad && !m_pendingJavaScriptExecution;
}

void NoteEditorPrivate::onNoteLoadFinished(bool ok)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onNoteLoadFinished: ok = ") << (ok ? QStringLiteral("true") : QStringLiteral("false")));

    if (!ok) {
        QNDEBUG(QStringLiteral("Note page was not loaded successfully"));
        return;
    }

    if (Q_UNLIKELY(!m_pNote))  {
        QNDEBUG(QStringLiteral("No note is set to the editor"));
        setPageEditable(false);
        return;
    }

    if (Q_UNLIKELY(!m_pNotebook)) {
        QNDEBUG(QStringLiteral("No notebook is set to the editor"));
        setPageEditable(false);
        return;
    }

    m_pendingNotePageLoad = false;
    m_pendingJavaScriptExecution = true;

    GET_PAGE()
    page->stopJavaScriptAutoExecution();

    bool editable = true;
    if (m_pNote->hasActive() && !m_pNote->active()) {
        QNDEBUG(QStringLiteral("Current note is not active, setting it to read-only state"));
        editable = false;
    }
    else if (m_pNote->isInkNote()) {
        QNDEBUG(QStringLiteral("Current note is an ink note, setting it to read-only state"));
        editable = false;
    }
    else if (m_pNotebook->hasRestrictions())
    {
        const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
        if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref()) {
            QNDEBUG(QStringLiteral("Notebook restrictions forbid the note modification, setting note's content to read-only state"));
            editable = false;
        }
    }

    setPageEditable(editable);

    page->executeJavaScript(m_jQueryJs);
    page->executeJavaScript(m_jQueryUiJs);
    page->executeJavaScript(m_getSelectionHtmlJs);
    page->executeJavaScript(m_replaceSelectionWithHtmlJs);
    page->executeJavaScript(m_findReplaceManagerJs);

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    QWebFrame * frame = page->mainFrame();
    if (Q_UNLIKELY(!frame)) {
        return;
    }

    frame->addToJavaScriptWindowObject(QStringLiteral("pageMutationObserver"), m_pPageMutationHandler,
                                       OwnershipNamespace::QtOwnership);
    frame->addToJavaScriptWindowObject(QStringLiteral("resourceCache"), m_pResourceInfoJavaScriptHandler,
                                       OwnershipNamespace::QtOwnership);
    frame->addToJavaScriptWindowObject(QStringLiteral("textCursorPositionHandler"), m_pTextCursorPositionJavaScriptHandler,
                                       OwnershipNamespace::QtOwnership);
    frame->addToJavaScriptWindowObject(QStringLiteral("contextMenuEventHandler"), m_pContextMenuEventJavaScriptHandler,
                                       OwnershipNamespace::QtOwnership);
    frame->addToJavaScriptWindowObject(QStringLiteral("toDoCheckboxClickHandler"), m_pToDoCheckboxClickHandler,
                                       OwnershipNamespace::QtOwnership);
    frame->addToJavaScriptWindowObject(QStringLiteral("tableResizeHandler"), m_pTableResizeJavaScriptHandler,
                                       OwnershipNamespace::QtOwnership);
    frame->addToJavaScriptWindowObject(QStringLiteral("resizableImageHandler"), m_pResizableImageJavaScriptHandler,
                                       OwnershipNamespace::QtOwnership);
    frame->addToJavaScriptWindowObject(QStringLiteral("spellCheckerDynamicHelper"), m_pSpellCheckerDynamicHandler,
                                       OwnershipNamespace::QtOwnership);

    page->executeJavaScript(m_onResourceInfoReceivedJs);
    page->executeJavaScript(m_qWebKitSetupJs);
#else
    page->executeJavaScript(m_qWebChannelJs);
    page->executeJavaScript(QStringLiteral("(function(){window.websocketserverport = ") +
                            QString::number(m_webSocketServerPort) + QStringLiteral("})();"));
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
    page->executeJavaScript(m_resizableImageManagerJs);
    page->executeJavaScript(m_debounceJs);
    page->executeJavaScript(m_rangyCoreJs);
    page->executeJavaScript(m_rangySelectionSaveRestoreJs);
    page->executeJavaScript(m_onTableResizeJs);
    page->executeJavaScript(m_selectionManagerJs);
    page->executeJavaScript(m_textEditingUndoRedoManagerJs);
    page->executeJavaScript(m_snapSelectionToWordJs);
    page->executeJavaScript(m_replaceHyperlinkContentJs);
    page->executeJavaScript(m_updateResourceHashJs);
    page->executeJavaScript(m_updateImageResourceSrcJs);
    page->executeJavaScript(m_provideSrcForResourceImgTagsJs);
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
    page->executeJavaScript(m_managedPageActionJs);

    if (m_isPageEditable) {
        QNTRACE(QStringLiteral("Note page is editable"));
        page->executeJavaScript(m_setupEnToDoTagsJs);
        page->executeJavaScript(m_flipEnToDoCheckboxStateJs);
    }

    updateColResizableTableBindings();
    saveNoteResourcesToLocalFiles();

#ifdef QUENTIER_USE_QT_WEB_ENGINE
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

    QNTRACE(QStringLiteral("Sent commands to execute all the page's necessary scripts"));
    page->startJavaScriptAutoExecution();
}

void NoteEditorPrivate::onContentChanged()
{
    QNTRACE(QStringLiteral("NoteEditorPrivate::onContentChanged"));

    if (m_pendingNotePageLoad || m_pendingIndexHtmlWritingToFile || m_pendingJavaScriptExecution) {
        QNTRACE(QStringLiteral("Skipping the content change as the note page has not fully loaded yet"));
        return;
    }

    if (m_skipPushingUndoCommandOnNextContentChange) {
        m_skipPushingUndoCommandOnNextContentChange = false;
        QNTRACE(QStringLiteral("Skipping the push of edit undo command on this content change"));
    }
    else {
        pushNoteContentEditUndoCommand();
    }

    setModified();

    if (Q_LIKELY(m_watchingForContentChange)) {
        m_contentChangedSinceWatchingStart = true;
        return;
    }

    m_pageToNoteContentPostponeTimerId = startTimer(SEC_TO_MSEC(m_secondsToWaitBeforeConversionStart));
    m_watchingForContentChange = true;
    m_contentChangedSinceWatchingStart = false;
    QNTRACE(QStringLiteral("Started timer to postpone note editor page's content to ENML conversion: timer id = ")
            << m_pageToNoteContentPostponeTimerId);
}

void NoteEditorPrivate::onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                                 QString fileStoragePath, int errorCode,
                                                 ErrorString errorDescription)
{
    auto it = m_genericResourceLocalUidBySaveToStorageRequestIds.find(requestId);
    if (it == m_genericResourceLocalUidBySaveToStorageRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorPrivate::onResourceSavedToStorage: requestId = ") << requestId
            << QStringLiteral(", data hash = ") << dataHash.toHex() << QStringLiteral(", file storage path = ")
            << fileStoragePath << QStringLiteral(", error code = ") << errorCode << QStringLiteral(", error description: ")
            << errorDescription);

    if (errorCode != 0) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't write the resource to local file"));
        error.additionalBases().append(errorDescription.base());
        error.additionalBases().append(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING(error << QStringLiteral(", error code = ") << errorCode);
        emit notifyError(error);
        return;
    }

    const QString localUid = it.value();
    QNDEBUG(QStringLiteral("Local uid of the resource updated in the local storage: ") << localUid);

    bool shouldCreateLinkToTheResourceFile = false;
    auto imageResourceIt = m_imageResourceSaveToStorageRequestIds.find(requestId);
    shouldCreateLinkToTheResourceFile = (imageResourceIt != m_imageResourceSaveToStorageRequestIds.end());
    if (shouldCreateLinkToTheResourceFile)
    {
        QNDEBUG(QStringLiteral("The just saved resource is an image for which we need to create a link "
                               "in order to fool the web engine's cache to ensure it would reload the image "
                               "if its previous version has already been displayed on the page"));

        Q_UNUSED(m_imageResourceSaveToStorageRequestIds.erase(imageResourceIt));

        QString linkFileName = createSymlinkToImageResourceFile(fileStoragePath, localUid, errorDescription);
        if (linkFileName.isEmpty()) {
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
        }
        else {
            fileStoragePath = linkFileName;
        }
    }

    m_resourceFileStoragePathsByResourceLocalUid[localUid] = fileStoragePath;

    QString resourceDisplayName;
    QString resourceDisplaySize;

    QByteArray oldResourceHash;
    if (!m_pNote.isNull())
    {
        bool shouldUpdateNoteResources = false;
        QList<Resource> resources = m_pNote->resources();
        const int numResources = resources.size();
        for(int i = 0; i < numResources; ++i)
        {
            Resource & resource = resources[i];

            if (resource.localUid() != localUid) {
                continue;
            }

            shouldUpdateNoteResources = true;
            if (resource.hasDataHash()) {
                oldResourceHash = resource.dataHash();
            }

            resource.setDataHash(dataHash);

            if (resource.hasRecognitionDataBody())
            {
                ResourceRecognitionIndices recoIndices(resource.recognitionDataBody());
                if (!recoIndices.isNull() && recoIndices.isValid()) {
                    m_recognitionIndicesByResourceHash[dataHash] = recoIndices;
                    QNDEBUG(QStringLiteral("Updated recognition indices for resource with hash ") << dataHash.toHex());
                }
            }

            resourceDisplayName = resource.displayName();
            if (resource.hasDataSize()) {
                resourceDisplaySize = humanReadableSize(static_cast<quint64>(resource.dataSize()));
            }

            break;
        }

        if (shouldUpdateNoteResources) {
            m_pNote->setResources(resources);
        }
    }

    m_resourceInfo.cacheResourceInfo(dataHash, resourceDisplayName,
                                     resourceDisplaySize, fileStoragePath);

    Q_UNUSED(m_genericResourceLocalUidBySaveToStorageRequestIds.erase(it));

    if (!oldResourceHash.isEmpty() && (oldResourceHash != dataHash)) {
        updateHashForResourceTag(oldResourceHash, dataHash);
        highlightRecognizedImageAreas(m_lastSearchHighlightedText, m_lastSearchHighlightedTextCaseSensitivity);
    }

    if (m_genericResourceLocalUidBySaveToStorageRequestIds.isEmpty() && !m_pendingNotePageLoad) {
        QNTRACE(QStringLiteral("All current note's resources were saved to local file storage and are actual. "
                               "Will set filepaths to these local files to src attributes of img resource tags"));
        provideSrcForResourceImgTags();
    }

    auto saveIt = m_localUidsOfResourcesWantedToBeSaved.find(localUid);
    if (saveIt != m_localUidsOfResourcesWantedToBeSaved.end())
    {
        QNTRACE(QStringLiteral("Resource with local uid ") << localUid << QStringLiteral(" is pending manual saving to file"));

        Q_UNUSED(m_localUidsOfResourcesWantedToBeSaved.erase(saveIt));

        if (Q_UNLIKELY(m_pNote.isNull())) {
            ErrorString error(QT_TRANSLATE_NOOP("", "can't save the resource to file: no note is set to the editor"));
            QNINFO(error << QStringLiteral(", resource local uid = ") << localUid);
            emit notifyError(error);
            return;
        }

        QList<Resource> resources = m_pNote->resources();
        int numResources = resources.size();
        for(int i = 0; i < numResources; ++i)
        {
            const Resource & resource = resources[i];
            if (resource.localUid() == localUid) {
                manualSaveResourceToFile(resource);
                return;
            }
        }

        ErrorString error(QT_TRANSLATE_NOOP("", "can't save the resource to file: can't find the target resource within the note"));
        QNINFO(error << QStringLiteral(", resource local uid = ") << localUid);
        emit notifyError(error);
    }

    auto openIt = m_localUidsOfResourcesWantedToBeOpened.find(localUid);
    if (openIt != m_localUidsOfResourcesWantedToBeOpened.end()) {
        QNTRACE(QStringLiteral("Resource with local uid ") << localUid << QStringLiteral(" is pending opening in application"));
        Q_UNUSED(m_localUidsOfResourcesWantedToBeOpened.erase(openIt));
        openResource(fileStoragePath);
    }

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    setupGenericResourceImages();
#endif
}

void NoteEditorPrivate::onResourceFileChanged(QString resourceLocalUid, QString fileStoragePath)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onResourceFileChanged: resource local uid = ") << resourceLocalUid
            << QStringLiteral(", file storage path = ") << fileStoragePath);

    if (Q_UNLIKELY(m_pNote.isNull())) {
        QNDEBUG(QStringLiteral("Can't process resource file change: no note is set to the editor"));
        return;
    }

    QList<Resource> resources = m_pNote->resources();
    const int numResources = resources.size();
    int targetResourceIndex = -1;
    for(int i = 0; i < numResources; ++i)
    {
        const Resource & resource = resources[i];
        if (resource.localUid() == resourceLocalUid) {
            targetResourceIndex = i;
            break;
        }
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        QNDEBUG(QStringLiteral("Can't process resource file change: can't find the resource by local uid within note's resources"));
        return;
    }

    QUuid requestId = QUuid::createUuid();
    m_resourceLocalUidAndFileStoragePathByReadResourceRequestIds[requestId] = QPair<QString,QString>(resourceLocalUid, fileStoragePath);
    QNTRACE(QStringLiteral("Emitting request to read the resource file from storage: file storage path = ")
            << fileStoragePath << QStringLiteral(", local uid = ") << resourceLocalUid << QStringLiteral(", request id = ") << requestId);
    emit readResourceFromStorage(fileStoragePath, resourceLocalUid, requestId);
}

void NoteEditorPrivate::onResourceFileReadFromStorage(QUuid requestId, QByteArray data, QByteArray dataHash,
                                                      int errorCode, ErrorString errorDescription)
{
    auto it = m_resourceLocalUidAndFileStoragePathByReadResourceRequestIds.find(requestId);
    if (it == m_resourceLocalUidAndFileStoragePathByReadResourceRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("NoteEditorPrivate::onResourceFileReadFromStorage: request id = ") << requestId
            << QStringLiteral(", data hash = ") << dataHash.toHex() << QStringLiteral(", errorCode = ")
            << errorCode << QStringLiteral(", error description: ") << errorDescription);

    auto pair = it.value();
    QString resourceLocalUid = pair.first;
    QString fileStoragePath = pair.second;

    Q_UNUSED(m_resourceLocalUidAndFileStoragePathByReadResourceRequestIds.erase(it));

    if (Q_UNLIKELY(errorCode != 0)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't process the update of the resource file data: can't read the data from file"));
        error.additionalBases().append(errorDescription.base());
        error.additionalBases().append(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        QNWARNING(error << QStringLiteral(", resource local uid = ") << resourceLocalUid << QStringLiteral(", error code = ") << errorCode);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(m_pNote.isNull())) {
        QNDEBUG(QStringLiteral("Can't process the update of the resource file data: no note is set to the editor"));
        return;
    }

    QByteArray oldResourceHash;
    QString resourceDisplayName;
    QString resourceDisplaySize;
    QString resourceMimeTypeName;

    QList<Resource> resources = m_pNote->resources();
    int targetResourceIndex = -1;
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        Resource & resource = resources[i];
        if (resource.localUid() != resourceLocalUid) {
            continue;
        }

        if (resource.hasDataHash()) {
            oldResourceHash = resource.dataHash();
        }

        resource.setDataBody(data);
        resource.setDataHash(dataHash);
        resource.setDataSize(data.size());

        if (Q_LIKELY(resource.hasMime())) {
            resourceMimeTypeName = resource.mime();
        }

        resourceDisplayName = resource.displayName();
        resourceDisplaySize = humanReadableSize(static_cast<quint64>(resource.dataSize()));

        targetResourceIndex = i;
        break;
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        QNDEBUG(QStringLiteral("Can't process the update of the resource file data: can't find the corresponding resource within the note's resources"));
        return;
    }

    const Resource & resource = resources[targetResourceIndex];
    bool updated = m_pNote->updateResource(resource);
    if (Q_UNLIKELY(!updated)) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "failed to update the resource within the note"));
        QNWARNING(errorDescription << QStringLiteral(", resource: ") << resource << QStringLiteral("\nNote: ") << *m_pNote);
        emit notifyError(errorDescription);
        return;
    }

    setModified();

    if (!oldResourceHash.isEmpty() && (oldResourceHash != dataHash))
    {
        m_resourceInfo.removeResourceInfo(oldResourceHash);
        m_resourceInfo.cacheResourceInfo(dataHash, resourceDisplayName,
                                         resourceDisplaySize, fileStoragePath);
        updateHashForResourceTag(oldResourceHash, dataHash);
    }

    if (resourceMimeTypeName.startsWith(QStringLiteral("image/")))
    {
        QString linkFileName = createSymlinkToImageResourceFile(fileStoragePath, resourceLocalUid, errorDescription);
        if (linkFileName.isEmpty()) {
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return;
        }

        m_resourceFileStoragePathsByResourceLocalUid[resourceLocalUid] = linkFileName;

        m_resourceInfo.cacheResourceInfo(dataHash, resourceDisplayName,
                                         resourceDisplaySize, linkFileName);

        if (!m_pendingNotePageLoad) {
            GET_PAGE()
            page->executeJavaScript(QStringLiteral("updateImageResourceSrc('") + dataHash.toHex() + QStringLiteral("', '") +
                                    linkFileName + QStringLiteral("');"));
        }
    }
    else
    {
        QImage image = buildGenericResourceImage(resource);
        saveGenericResourceImage(resource, image);
    }
}

#ifdef QUENTIER_USE_QT_WEB_ENGINE
void NoteEditorPrivate::onGenericResourceImageSaved(bool success, QByteArray resourceActualHash,
                                                    QString filePath, ErrorString errorDescription,
                                                    QUuid requestId)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onGenericResourceImageSaved: success = ")
            << (success ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", resource actual hash = ") << resourceActualHash.toHex()
            << QStringLiteral(", file path = ") << filePath << QStringLiteral(", error description = ")
            << errorDescription << QStringLiteral(", requestId = ") << requestId);

    auto it = m_saveGenericResourceImageToFileRequestIds.find(requestId);
    if (it == m_saveGenericResourceImageToFileRequestIds.end()) {
        QNDEBUG(QStringLiteral("Haven't found request id in the cache"));
        return;
    }

    Q_UNUSED(m_saveGenericResourceImageToFileRequestIds.erase(it));

    if (Q_UNLIKELY(!success)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save the generic resource image to file"));
        error.additionalBases().append(errorDescription.base());
        error.additionalBases().append(errorDescription.additionalBases());
        error.details() = errorDescription.details();
        emit notifyError(error);
        return;
    }

    m_genericResourceImageFilePathsByResourceHash[resourceActualHash] = filePath;
    QNDEBUG(QStringLiteral("Cached generic resource image file path ") << filePath
            << QStringLiteral(" for resource hash ") << resourceActualHash.toHex());

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

#endif // QUENTIER_USE_QT_WEB_ENGINE

void NoteEditorPrivate::onToDoCheckboxClicked(quint64 enToDoCheckboxId)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onToDoCheckboxClicked: ") << enToDoCheckboxId);

    setModified();

    ToDoCheckboxUndoCommand * pCommand = new ToDoCheckboxUndoCommand(enToDoCheckboxId, *this);
    QObject::connect(pCommand, QNSIGNAL(ToDoCheckboxUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::onToDoCheckboxClickHandlerError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onToDoCheckboxClickHandlerError: ") << error);
    emit notifyError(error);
}

void NoteEditorPrivate::onJavaScriptLoaded()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onJavaScriptLoaded"));

    NoteEditorPage * pSenderPage = qobject_cast<NoteEditorPage*>(sender());
    if (Q_UNLIKELY(!pSenderPage)) {
        QNWARNING(QStringLiteral("Can't get the pointer to NoteEditor page from which the event of JavaScrupt loading came in"));
        return;
    }

    GET_PAGE()
    if (page != pSenderPage) {
        QNDEBUG(QStringLiteral("Skipping JavaScript loaded event from page which is not the currently set one"));
        return;
    }

    if (m_pendingJavaScriptExecution)
    {
        m_pendingJavaScriptExecution = false;

        if (Q_UNLIKELY(!m_pNote)) {
            QNDEBUG(QStringLiteral("No note is set to the editor, won't retrieve the editor content's html"));
            return;
        }

        if (Q_UNLIKELY(!m_pNotebook)) {
            QNDEBUG(QStringLiteral("No notebook is set to the editor, won't retrieve the editor content's html"));
            return;
        }

#ifndef QUENTIER_USE_QT_WEB_ENGINE
        m_htmlCachedMemory = page->mainFrame()->toHtml();
        onPageHtmlReceived(m_htmlCachedMemory);
#else
        page->toHtml(NoteEditorCallbackFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
#endif

        QNTRACE(QStringLiteral("Emitting noteLoaded signal"));
        emit noteLoaded();
    }
}

void NoteEditorPrivate::onOpenResourceRequest(const QByteArray & resourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onOpenResourceRequest: ") << resourceHash.toHex());

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't open the resource: no note is set to the editor"));
        QNWARNING(error << QStringLiteral(", resource hash = ") << resourceHash.toHex());
        emit notifyError(error);
        return;
    }

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't open attachment"))

    QList<Resource> resources = m_pNote->resources();
    int resourceIndex = resourceIndexByHash(resources, resourceHash);
    if (Q_UNLIKELY(resourceIndex < 0)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "The resource to be opened was not found within the note"));
        QNWARNING(error << QStringLiteral(", resource hash = ") << resourceHash);
        emit notifyError(error);
        return;
    }

    const Resource & resource = resources[resourceIndex];
    const QString resourceLocalUid = resource.localUid();

    // See whether this resource has already been written to file
    auto it = m_resourceFileStoragePathsByResourceLocalUid.find(resourceLocalUid);
    if (it == m_resourceFileStoragePathsByResourceLocalUid.end()) {
        // It must be being written to file at the moment - all note's resources are saved to files on note load -
        // so just mark this resource local uid as pending for open
        Q_UNUSED(m_localUidsOfResourcesWantedToBeOpened.insert(resourceLocalUid));
        return;
    }

    openResource(it.value());
}

void NoteEditorPrivate::onSaveResourceRequest(const QByteArray & resourceHash)
{
    QNDEBUG("NoteEditorPrivate::onSaveResourceRequest: " << resourceHash.toHex());

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save the resource to file: no note is set to the editor"));
        QNINFO(error << QStringLiteral(", resource hash = ") << resourceHash.toHex());
        emit notifyError(error);
        return;
    }

    QList<Resource> resources = m_pNote->resources();
    int resourceIndex = resourceIndexByHash(resources, resourceHash);
    if (Q_UNLIKELY(resourceIndex < 0)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "The resource to be saved was not found within the note"));
        QNINFO(error << QStringLiteral(", resource hash = ") << resourceHash.toHex());
        return;
    }

    const Resource & resource = resources[resourceIndex];
    const QString resourceLocalUid = resource.localUid();

    // See whether this resource has already been written to file
    auto it = m_resourceFileStoragePathsByResourceLocalUid.find(resourceLocalUid);
    if (it == m_resourceFileStoragePathsByResourceLocalUid.end()) {
        // It must be being written to file at the moment - all note's resources are saved to files on note load -
        // so just mark this resource local uid as pending for save
        Q_UNUSED(m_localUidsOfResourcesWantedToBeSaved.insert(resourceLocalUid));
        return;
    }

    manualSaveResourceToFile(resource);
}

void NoteEditorPrivate::contextMenuEvent(QContextMenuEvent * pEvent)
{
    QNTRACE(QStringLiteral("NoteEditorPrivate::contextMenuEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNINFO(QStringLiteral("detected null pointer to context menu event"));
        return;
    }

    if (m_pendingIndexHtmlWritingToFile || m_pendingNotePageLoad || m_pendingJavaScriptExecution) {
        QNINFO(QStringLiteral("Ignoring context menu event for now, until the note is fully loaded..."));
        return;
    }

    m_lastContextMenuEventGlobalPos = pEvent->globalPos();
    m_lastContextMenuEventPagePos = pEvent->pos();

    QNTRACE(QStringLiteral("Context menu event's global pos: x = ") << m_lastContextMenuEventGlobalPos.x()
            << QStringLiteral(", y = ") << m_lastContextMenuEventGlobalPos.y() << QStringLiteral("; pos relative to child widget: x = ")
            << m_lastContextMenuEventPagePos.x() << QStringLiteral(", y = ") << m_lastContextMenuEventPagePos.y()
            << QStringLiteral("; context menu sequence number = ") << m_contextMenuSequenceNumber);

    determineContextMenuEventTarget();
}

void NoteEditorPrivate::onContextMenuEventReply(QString contentType, QString selectedHtml,
                                                bool insideDecryptedTextFragment,
                                                QStringList extraData, quint64 sequenceNumber)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onContextMenuEventReply: content type = ") << contentType
            << QStringLiteral(", selected html = ") << selectedHtml << QStringLiteral(", inside decrypted text fragment = ")
            << (insideDecryptedTextFragment ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", extraData: [")
            << extraData.join(QStringLiteral(", ")) << QStringLiteral("], sequence number = ") << sequenceNumber);

    if (!checkContextMenuSequenceNumber(sequenceNumber)) {
        QNTRACE(QStringLiteral("Sequence number is not valid, not doing anything"));
        return;
    }

    ++m_contextMenuSequenceNumber;

    m_currentContextMenuExtraData.m_contentType = contentType;
    m_currentContextMenuExtraData.m_insideDecryptedText = insideDecryptedTextFragment;

    if (contentType == QStringLiteral("GenericText"))
    {
        setupGenericTextContextMenu(extraData, selectedHtml, insideDecryptedTextFragment);
    }
    else if ((contentType == QStringLiteral("ImageResource")) || (contentType == QStringLiteral("NonImageResource")))
    {
        if (Q_UNLIKELY(extraData.empty())) {
            ErrorString error(QT_TRANSLATE_NOOP("", "can't display the resource context menu: the extra data from JavaScript is empty"));
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        if (Q_UNLIKELY(extraData.size() != 1)) {
            ErrorString error(QT_TRANSLATE_NOOP("", "can't display the resource context menu: the extra data from JavaScript has wrong size"));
            error.details() = QString::number(extraData.size());
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        QByteArray resourceHash = QByteArray::fromHex(extraData[0].toLocal8Bit());

        if (contentType == QStringLiteral("ImageResource")) {
            setupImageResourceContextMenu(resourceHash);
        }
        else {
            setupNonImageResourceContextMenu(resourceHash);
        }
    }
    else if (contentType == QStringLiteral("EncryptedText"))
    {
        QString cipher, keyLength, encryptedText, decryptedText, hint, id;
        ErrorString error;
        bool res = parseEncryptedTextContextMenuExtraData(extraData, encryptedText, decryptedText, cipher, keyLength, hint, id, error);
        if (Q_UNLIKELY(!res)) {
            ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't display the encrypted text's context menu"));
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return;
        }

        setupEncryptedTextContextMenu(cipher, keyLength, encryptedText, hint, id);
    }
    else
    {
        QNWARNING(QStringLiteral("Unknown content type on context menu event reply: ") << contentType
                  << QStringLiteral(", sequence number ") << sequenceNumber);
    }
}

void NoteEditorPrivate::onTextCursorPositionChange()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorPositionChange"));

    if (!m_pendingIndexHtmlWritingToFile && !m_pendingNotePageLoad && !m_pendingJavaScriptExecution) {
        determineStatesForCurrentTextCursorPosition();
    }
}

void NoteEditorPrivate::onTextCursorBoldStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorBoldStateChanged: ")
            << (state ? QStringLiteral("bold") : QStringLiteral("not bold")));

    m_currentTextFormattingState.m_bold = state;
    emit textBoldState(state);
}

void NoteEditorPrivate::onTextCursorItalicStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorItalicStateChanged: ")
            << (state ? QStringLiteral("italic") : QStringLiteral("not italic")));

    m_currentTextFormattingState.m_italic = state;
    emit textItalicState(state);
}

void NoteEditorPrivate::onTextCursorUnderlineStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorUnderlineStateChanged: ")
            << (state ? QStringLiteral("underline") : QStringLiteral("not underline")));

    m_currentTextFormattingState.m_underline = state;
    emit textUnderlineState(state);
}

void NoteEditorPrivate::onTextCursorStrikethgouthStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorStrikethgouthStateChanged: ")
            << (state ? QStringLiteral("strikethrough") : QStringLiteral("not strikethrough")));

    m_currentTextFormattingState.m_strikethrough = state;
    emit textStrikethroughState(state);
}

void NoteEditorPrivate::onTextCursorAlignLeftStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorAlignLeftStateChanged: ")
            << (state ? QStringLiteral("true") : QStringLiteral("false")));

    if (state) {
        m_currentTextFormattingState.m_alignment = Alignment::Left;
    }

    emit textAlignLeftState(state);
}

void NoteEditorPrivate::onTextCursorAlignCenterStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorAlignCenterStateChanged: ")
            << (state ? QStringLiteral("true") : QStringLiteral("false")));

    if (state) {
        m_currentTextFormattingState.m_alignment = Alignment::Center;
    }

    emit textAlignCenterState(state);
}

void NoteEditorPrivate::onTextCursorAlignRightStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorAlignRightStateChanged: ")
            << (state ? QStringLiteral("true") : QStringLiteral("false")));

    if (state) {
        m_currentTextFormattingState.m_alignment = Alignment::Right;
    }

    emit textAlignRightState(state);
}

void NoteEditorPrivate::onTextCursorInsideOrderedListStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorInsideOrderedListStateChanged: ")
            << (state ? QStringLiteral("true") : QStringLiteral("false")));

    m_currentTextFormattingState.m_insideOrderedList = state;
    emit textInsideOrderedListState(state);
}

void NoteEditorPrivate::onTextCursorInsideUnorderedListStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorInsideUnorderedListStateChanged: ")
            << (state ? QStringLiteral("true") : QStringLiteral("false")));

    m_currentTextFormattingState.m_insideUnorderedList = state;
    emit textInsideUnorderedListState(state);
}

void NoteEditorPrivate::onTextCursorInsideTableStateChanged(bool state)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorInsideTableStateChanged: ")
            << (state ? QStringLiteral("true") : QStringLiteral("false")));

    m_currentTextFormattingState.m_insideTable = state;
    emit textInsideTableState(state);
}

void NoteEditorPrivate::onTextCursorOnImageResourceStateChanged(bool state, QByteArray resourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorOnImageResourceStateChanged: ")
            << (state ? QStringLiteral("yes") : QStringLiteral("no")) << QStringLiteral(", resource hash = ")
            << resourceHash.toHex());

    m_currentTextFormattingState.m_onImageResource = state;
    if (state) {
        m_currentTextFormattingState.m_resourceHash = resourceHash;
    }
}

void NoteEditorPrivate::onTextCursorOnNonImageResourceStateChanged(bool state, QByteArray resourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorOnNonImageResourceStateChanged: ")
            << (state ? QStringLiteral("yes") : QStringLiteral("no"))
            << QStringLiteral(", resource hash = ") << resourceHash.toHex());

    m_currentTextFormattingState.m_onNonImageResource = state;
    if (state) {
        m_currentTextFormattingState.m_resourceHash = resourceHash;
    }
}

void NoteEditorPrivate::onTextCursorOnEnCryptTagStateChanged(bool state, QString encryptedText, QString cipher, QString length)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorOnEnCryptTagStateChanged: ") << (state ? QStringLiteral("yes") : "no")
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorFontNameChanged: font name = ") << fontName);
    emit textFontFamilyChanged(fontName);
}

void NoteEditorPrivate::onTextCursorFontSizeChanged(int fontSize)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTextCursorFontSizeChanged: font size = ") << fontSize);
    emit textFontSizeChanged(fontSize);
}

void NoteEditorPrivate::onWriteFileRequestProcessed(bool success, ErrorString errorDescription, QUuid requestId)
{
    if (requestId == m_writeNoteHtmlToFileRequestId)
    {
        QNDEBUG(QStringLiteral("Write note html to file completed: success = ") << (success ? QStringLiteral("true") : QStringLiteral("false"))
                << QStringLiteral(", request id = ") << requestId);

        m_writeNoteHtmlToFileRequestId = QUuid();
        m_pendingIndexHtmlWritingToFile = false;

        if (!success) {
            clearEditorContent();
            ErrorString error(QT_TRANSLATE_NOOP("", "could not write note html to file"));
            error.additionalBases().append(errorDescription.base());
            error.additionalBases().append(errorDescription.additionalBases());
            error.details() = errorDescription.details();
            emit notifyError(error);
            return;
        }

        QUrl url = QUrl::fromLocalFile(noteEditorPagePath());

#ifdef QUENTIER_USE_QT_WEB_ENGINE
        page()->setUrl(url);
        page()->load(url);
#else
        page()->mainFrame()->setUrl(url);
        page()->mainFrame()->load(url);
#endif
        QNTRACE(QStringLiteral("Loaded url: ") << url);
        m_pendingNotePageLoad = true;
    }

    auto manualSaveResourceIt = m_manualSaveResourceToFileRequestIds.find(requestId);
    if (manualSaveResourceIt != m_manualSaveResourceToFileRequestIds.end())
    {
        if (success) {
            QNDEBUG(QStringLiteral("Successfully saved resource to file for request id ") << requestId);
        }
        else {
            QNWARNING(QStringLiteral("Could not save resource to file: ") << errorDescription);
        }

        Q_UNUSED(m_manualSaveResourceToFileRequestIds.erase(manualSaveResourceIt));
        return;
    }
}

void NoteEditorPrivate::onAddResourceDelegateFinished(Resource addedResource, QString resourceFileStoragePath)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onAddResourceDelegateFinished: resource file storage path = ") << resourceFileStoragePath);

    QNTRACE(addedResource);

    if (Q_UNLIKELY(!addedResource.hasDataHash())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "the added resource doesn't contain the data hash"));
        QNWARNING(error);
        removeResourceFromNote(addedResource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!addedResource.hasDataSize())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "the added resource doesn't contain the data size"));
        QNWARNING(error);
        removeResourceFromNote(addedResource);
        emit notifyError(error);
        return;
    }

    m_resourceFileStoragePathsByResourceLocalUid[addedResource.localUid()] = resourceFileStoragePath;

    m_resourceInfo.cacheResourceInfo(addedResource.dataHash(), addedResource.displayName(),
                                     humanReadableSize(static_cast<quint64>(addedResource.dataSize())), resourceFileStoragePath);

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    setupGenericResourceImages();
#endif

    provideSrcForResourceImgTags();

    AddResourceUndoCommand * pCommand =
        new AddResourceUndoCommand(addedResource,
                                   NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onAddResourceUndoRedoFinished),
                                   *this);

    QObject::connect(pCommand, QNSIGNAL(AddResourceUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    AddResourceDelegate * delegate = qobject_cast<AddResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    setModified();
    convertToNote();
}

void NoteEditorPrivate::onAddResourceDelegateError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onAddResourceDelegateError: ") << error);
    emit notifyError(error);

    AddResourceDelegate * delegate = qobject_cast<AddResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onAddResourceUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onAddResourceUndoRedoFinished: ") << data);

    Q_UNUSED(extraData);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of new resource html insertion from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "can't parse the error of new resource html insertion from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "can't insert resource html into the note editor");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::onRemoveResourceDelegateFinished(Resource removedResource)
{
    QNDEBUG(QStringLiteral("onRemoveResourceDelegateFinished: removed resource = ") << removedResource);

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onRemoveResourceUndoRedoFinished);
    RemoveResourceUndoCommand * pCommand = new RemoveResourceUndoCommand(removedResource, callback, *this);
    QObject::connect(pCommand, QNSIGNAL(RemoveResourceUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    RemoveResourceDelegate * delegate = qobject_cast<RemoveResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRemoveResourceDelegateError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onRemoveResourceDelegateError: ") << error);
    emit notifyError(error);

    RemoveResourceDelegate * delegate = qobject_cast<RemoveResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRemoveResourceUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onRemoveResourceUndoRedoFinished: ") << data);

    Q_UNUSED(extraData)

    if (!m_lastSearchHighlightedText.isEmpty()) {
        highlightRecognizedImageAreas(m_lastSearchHighlightedText, m_lastSearchHighlightedTextCaseSensitivity);
    }
}

void NoteEditorPrivate::onRenameResourceDelegateFinished(QString oldResourceName, QString newResourceName,
                                                         Resource resource, bool performingUndo)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onRenameResourceDelegateFinished: old resource name = ") << oldResourceName
            << QStringLiteral(", new resource name = ") << newResourceName << QStringLiteral(", performing undo = ")
            << (performingUndo ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", resource: ") << resource);

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    if (m_pPluginFactory) {
        m_pPluginFactory->updateResource(resource);
    }
#endif

    if (!performingUndo) {
        RenameResourceUndoCommand * pCommand = new RenameResourceUndoCommand(resource, oldResourceName, *this,
                                                                             m_pGenericResourceImageManager,
                                                                             m_genericResourceImageFilePathsByResourceHash);
        QObject::connect(pCommand, QNSIGNAL(RenameResourceUndoCommand,notifyError,ErrorString),
                         this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
        m_pUndoStack->push(pCommand);
    }

    RenameResourceDelegate * delegate = qobject_cast<RenameResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRenameResourceDelegateCancelled()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onRenameResourceDelegateCancelled"));

    RenameResourceDelegate * delegate = qobject_cast<RenameResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRenameResourceDelegateError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onRenameResourceDelegateError: ") << error);

    emit notifyError(error);

    RenameResourceDelegate * delegate = qobject_cast<RenameResourceDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onImageResourceRotationDelegateFinished(QByteArray resourceDataBefore, QByteArray resourceHashBefore,
                                                                QByteArray resourceRecognitionDataBefore, QByteArray resourceRecognitionDataHashBefore,
                                                                Resource resourceAfter, INoteEditorBackend::Rotation::type rotationDirection)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onImageResourceRotationDelegateFinished: previous resource hash = ")
            << resourceHashBefore.toHex() << QStringLiteral(", resource local uid = ") << resourceAfter.localUid()
            << QStringLiteral(", rotation direction = ") << rotationDirection);

    ImageResourceRotationUndoCommand * pCommand = new ImageResourceRotationUndoCommand(resourceDataBefore, resourceHashBefore,
                                                                                       resourceRecognitionDataBefore,
                                                                                       resourceRecognitionDataHashBefore,
                                                                                       resourceAfter, rotationDirection, *this);
    QObject::connect(pCommand, QNSIGNAL(ImageResourceRotationUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    ImageResourceRotationDelegate * delegate = qobject_cast<ImageResourceRotationDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    highlightRecognizedImageAreas(m_lastSearchHighlightedText, m_lastSearchHighlightedTextCaseSensitivity);

    setModified();
    convertToNote();
}

void NoteEditorPrivate::onImageResourceRotationDelegateError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onImageResourceRotationDelegateError"));
    emit notifyError(error);

    ImageResourceRotationDelegate * delegate = qobject_cast<ImageResourceRotationDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onHideDecryptedTextFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onHideDecryptedTextFinished: ") << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of decrypted text hiding from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "can't parse the error of decrypted text hiding from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "can't hide the decrypted text");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    setModified();

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif

    HideDecryptedTextUndoCommand * pCommand =
        new HideDecryptedTextUndoCommand(*this,
                                         NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onHideDecryptedTextUndoRedoFinished));

    QObject::connect(pCommand, QNSIGNAL(HideDecryptedTextUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::onHideDecryptedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onHideDecryptedTextUndoRedoFinished: ") << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of decrypted text hiding undo/redo from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "can't parse the error of decrypted text hiding undo/redo from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "can't undo/redo the decrypted text hiding");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif
}

void NoteEditorPrivate::onEncryptSelectedTextDelegateFinished()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onEncryptSelectedTextDelegateFinished"));

    EncryptUndoCommand * pCommand =
        new EncryptUndoCommand(*this,
                               NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onEncryptSelectedTextUndoRedoFinished));

    QObject::connect(pCommand, QNSIGNAL(EncryptUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    EncryptSelectedTextDelegate * delegate = qobject_cast<EncryptSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    setModified();
    convertToNote();

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif
}

void NoteEditorPrivate::onEncryptSelectedTextDelegateCancelled()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onEncryptSelectedTextDelegateCancelled"));

    EncryptSelectedTextDelegate * delegate = qobject_cast<EncryptSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onEncryptSelectedTextDelegateError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onEncryptSelectedTextDelegateError: ") << error);
    emit notifyError(error);

    EncryptSelectedTextDelegate * delegate = qobject_cast<EncryptSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onEncryptSelectedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onEncryptSelectedTextUndoRedoFinished: ") << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of encryption undo/redo from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "can't parse the error of encryption undo/redo from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "can't undo/redo the selected text encryption");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif
}

void NoteEditorPrivate::onDecryptEncryptedTextDelegateFinished(QString encryptedText, QString cipher, size_t length, QString hint,
                                                               QString decryptedText, QString passphrase, bool rememberForSession,
                                                               bool decryptPermanently)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onDecryptEncryptedTextDelegateFinished"));

    setModified();

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
    extraData << QPair<QString, QString>(QStringLiteral("decryptPermanently"), (decryptPermanently ? QStringLiteral("true") : QStringLiteral("false")));

    DecryptUndoCommand * pCommand =
        new DecryptUndoCommand(info, m_decryptedTextManager, *this,
                               NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onDecryptEncryptedTextUndoRedoFinished, extraData));
    QObject::connect(pCommand, QNSIGNAL(DecryptUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onDecryptEncryptedTextDelegateCancelled"));

    DecryptEncryptedTextDelegate * delegate = qobject_cast<DecryptEncryptedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onDecryptEncryptedTextDelegateError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onDecryptEncryptedTextDelegateError: ") << error);

    emit notifyError(error);

    DecryptEncryptedTextDelegate * delegate = qobject_cast<DecryptEncryptedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onDecryptEncryptedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onDecryptEncryptedTextUndoRedoFinished: ") << data);

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of encrypted text decryption undo/redo from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "can't parse the error of encrypted text decryption undo/redo from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "can't undo/redo the encrypted text decryption");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool shouldConvertToNote = true;
    if (!extraData.isEmpty())
    {
        const QPair<QString, QString> & pair = extraData[0];
        if (pair.second == QStringLiteral("false")) {
            shouldConvertToNote = false;
        }
    }

    if (shouldConvertToNote) {
        convertToNote();
    }
}

void NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateFinished()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateFinished"));

    AddHyperlinkUndoCommand * pCommand =
        new AddHyperlinkUndoCommand(*this,
                                    NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onAddHyperlinkToSelectedTextUndoRedoFinished));
    QObject::connect(pCommand, QNSIGNAL(AddHyperlinkUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    AddHyperlinkToSelectedTextDelegate * delegate = qobject_cast<AddHyperlinkToSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    setModified();
    convertToNote();
}

void NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateCancelled()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateCancelled"));

    AddHyperlinkToSelectedTextDelegate * delegate = qobject_cast<AddHyperlinkToSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onAddHyperlinkToSelectedTextDelegateError"));
    emit notifyError(error);

    AddHyperlinkToSelectedTextDelegate * delegate = qobject_cast<AddHyperlinkToSelectedTextDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onAddHyperlinkToSelectedTextUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onAddHyperlinkToSelectedTextUndoRedoFinished: ") << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of hyperlink addition undo/redo from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "can't parse the error of hyperlink addition undo/redo from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "can't undo/redo the hyperlink addition");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::onEditHyperlinkDelegateFinished()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onEditHyperlinkDelegateFinished"));

    setModified();

    EditHyperlinkUndoCommand * pCommand =
        new EditHyperlinkUndoCommand(*this, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onEditHyperlinkUndoRedoFinished));
    QObject::connect(pCommand, QNSIGNAL(EditHyperlinkUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    EditHyperlinkDelegate * delegate = qobject_cast<EditHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    convertToNote();
}

void NoteEditorPrivate::onEditHyperlinkDelegateCancelled()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onEditHyperlinkDelegateCancelled"));

    EditHyperlinkDelegate * delegate = qobject_cast<EditHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onEditHyperlinkDelegateError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onEditHyperlinkDelegateError: ") << error);

    emit notifyError(error);

    EditHyperlinkDelegate * delegate = qobject_cast<EditHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onEditHyperlinkUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onEditHyperlinkUndoRedoFinished: ") << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of hyperlink edit undo/redo from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "can't parse the error of hyperlink edit undo/redo from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "can't undo/redo the hyperlink edit");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::onRemoveHyperlinkDelegateFinished()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onRemoveHyperlinkDelegateFinished"));

    setModified();

    RemoveHyperlinkUndoCommand * pCommand =
        new RemoveHyperlinkUndoCommand(*this,
                                       NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onRemoveHyperlinkUndoRedoFinished));

    QObject::connect(pCommand, QNSIGNAL(RemoveHyperlinkUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    RemoveHyperlinkDelegate * delegate = qobject_cast<RemoveHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }

    convertToNote();
}

void NoteEditorPrivate::onRemoveHyperlinkDelegateError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onRemoveHyperlinkDelegateError: ") << error);
    emit notifyError(error);

    RemoveHyperlinkDelegate * delegate = qobject_cast<RemoveHyperlinkDelegate*>(sender());
    if (Q_LIKELY(delegate)) {
        delegate->deleteLater();
    }
}

void NoteEditorPrivate::onRemoveHyperlinkUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onRemoveHyperlinkUndoRedoFinished: ") << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of hyperlink removal undo/redo from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "can't parse the error of hyperlink removal undo/redo from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "can't undo/redo the hyperlink removal");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::onUndoCommandError(ErrorString error)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onUndoCommandError: ") << error);
    emit notifyError(error);
}

void NoteEditorPrivate::timerEvent(QTimerEvent * pEvent)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::timerEvent: ") << (pEvent ? QString::number(pEvent->timerId()) : QStringLiteral("<null>")));

    if (Q_UNLIKELY(!pEvent)) {
        QNINFO(QStringLiteral("Detected null pointer to timer event"));
        return;
    }

    if (pEvent->timerId() == m_pageToNoteContentPostponeTimerId)
    {
        if (m_contentChangedSinceWatchingStart)
        {
            QNTRACE(QStringLiteral("Note editor page's content has been changed lately, "
                                   "the editing is most likely in progress now, postponing "
                                   "the conversion to ENML"));
            m_contentChangedSinceWatchingStart = false;
            return;
        }

        ErrorString error;
        QNTRACE(QStringLiteral("Looks like the note editing has stopped for a while, "
                               "will convert the note editor page's content to ENML"));
        bool res = htmlToNoteContent(error);
        if (!res) {
            emit notifyError(error);
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
        QNINFO(QStringLiteral("Detected null pointer to drag move event"));
        return;
    }

    const QMimeData * pMimeData = pEvent->mimeData();
    if (Q_UNLIKELY(!pMimeData)) {
        QNWARNING(QStringLiteral("Null pointer to mime data from drag move event was detected"));
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

void NoteEditorPrivate::init()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::init"));

    if (Q_UNLIKELY(m_pAccount.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't initialize the note editor: account is not set"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString accountName = m_pAccount->name();
    if (Q_UNLIKELY(accountName.isEmpty())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't initialize the note editor: account name is empty"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_noteEditorPageFolderPath = applicationPersistentStoragePath();
    if (m_pAccount->type() == Account::Type::Local)
    {
        m_noteEditorPageFolderPath += QStringLiteral("/LocalAccounts/");
        m_noteEditorPageFolderPath += accountName;
    }
    else
    {
        m_noteEditorPageFolderPath += QStringLiteral("/EvernoteAccounts/");
        m_noteEditorPageFolderPath += accountName;
        m_noteEditorPageFolderPath += QStringLiteral("_");
        m_noteEditorPageFolderPath += m_pAccount->evernoteHost();
        m_noteEditorPageFolderPath += QStringLiteral("_");
        m_noteEditorPageFolderPath += QString::number(m_pAccount->id());
    }

    m_noteEditorPageFolderPath += QStringLiteral("/NoteEditorPage");

    m_resourceLocalFileStorageFolder = m_noteEditorPageFolderPath + QStringLiteral("/nonImageResources");
    m_noteEditorImageResourcesStoragePath = m_noteEditorPageFolderPath + QStringLiteral("/imageResources");
    m_genericResourceImageFileStoragePath = m_noteEditorPageFolderPath + QStringLiteral("/genericResourceImages");

    setupFileIO();
    setupNoteEditorPage();
    setAcceptDrops(true);

    QString initialHtml = initialPageHtml();
    writeNotePageFile(initialHtml);
}

bool NoteEditorPrivate::checkNoteSize(const QString & newNoteContent) const
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::checkNoteSize"));

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Internal error: can't check the note size on note editor update: "
                                            "no note is set ot the editor"));
        QNWARNING(error);
        emit notifyError(error);
        return false;
    }

    qint64 noteSize = noteResourcesSize();
    noteSize += newNoteContent.size();

    if (m_pNote->hasNoteLimits())
    {
        QNTRACE(QStringLiteral("Note has its own limits, will use them to check the note size"));

        const qevercloud::NoteLimits & noteLimits = m_pNote->noteLimits();
        if (noteLimits.noteSizeMax.isSet() && (Q_UNLIKELY(noteLimits.noteSizeMax.ref() > noteSize))) {
            ErrorString error(QT_TRANSLATE_NOOP("", "Can't save note: note size (text + resources) exceeds the allowed maximum"));
            error.details() = humanReadableSize(static_cast<quint64>(noteLimits.noteSizeMax.ref()));
            QNINFO(error);
            emit cantConvertToNote(error);
            return false;
        }
    }
    else
    {
        QNTRACE(QStringLiteral("Note has no its own limits, will use the account-wise limits to check the note size"));

        if (Q_UNLIKELY(m_pAccount.isNull())) {
            ErrorString error(QT_TRANSLATE_NOOP("", "Internal error: can't check the note size on note editor update: no account "
                                                "is associated with the note editor; the note was not saved!"));
            QNWARNING(error);
            emit notifyError(error);
            return false;
        }

        if (Q_UNLIKELY(noteSize > m_pAccount->noteSizeMax())) {
            ErrorString error(QT_TRANSLATE_NOOP("", "Can't save note: note size (text + resources) exceeds the allowed maximum"));
            error.details() = humanReadableSize(static_cast<quint64>(m_pAccount->noteSizeMax()));
            QNINFO(error);
            emit cantConvertToNote(error);
            return false;
        }
    }

    return true;
}

void NoteEditorPrivate::pushNoteContentEditUndoCommand()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::pushNoteTextEditUndoCommand"));

    QUENTIER_CHECK_PTR(m_pUndoStack, QStringLiteral("Undo stack for note editor wasn't initialized"));

    if (Q_UNLIKELY(m_pNote.isNull())) {
        QNINFO(QStringLiteral("Ignoring the content changed signal as the note pointer is null"));
        return;
    }

    QList<Resource> resources;
    if (m_pNote->hasResources()) {
        resources = m_pNote->resources();
    }

    NoteEditorContentEditUndoCommand * pCommand = new NoteEditorContentEditUndoCommand(*this, resources);
    QObject::connect(pCommand, QNSIGNAL(NoteEditorContentEditUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::pushTableActionUndoCommand(const QString & name, NoteEditorPage::Callback callback)
{
    TableActionUndoCommand * pCommand = new TableActionUndoCommand(*this, name, callback);
    QObject::connect(pCommand, QNSIGNAL(TableActionUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::onManagedPageActionFinished(const QVariant & result, const QVector<QPair<QString, QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onManagedPageActionFinished: ") << result);
    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = result.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of managed page action execution attempt"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QString errorMessage;
        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (errorIt != resultMap.end()) {
            errorMessage = errorIt.value().toString();
        }

        ErrorString error(QT_TRANSLATE_NOOP("", "can't execute the page action"));
        error.details() = errorMessage;
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    auto commandIt = resultMap.find(QStringLiteral("command"));
    if (commandIt != resultMap.end())
    {
        QString command = commandIt.value().toString();
        if (command == QStringLiteral("cut"))
        {
            QClipboard * pClipboard = QApplication::clipboard();
            auto extraDataIt = resultMap.find(QStringLiteral("extraData"));
            if (pClipboard && (extraDataIt != resultMap.end())) {
                QMimeData * pMimeData = new QMimeData();
                pMimeData->setHtml(extraDataIt.value().toString());
                pClipboard->setMimeData(pMimeData);
            }
        }
    }

    pushNoteContentEditUndoCommand();

    updateJavaScriptBindings();
}

void NoteEditorPrivate::updateJavaScriptBindings()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::updateJavaScriptBindings"));

    updateColResizableTableBindings();

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    provideSrcAndOnClickScriptForImgEnCryptTags();
    setupGenericResourceImages();
#endif

    if (m_spellCheckerEnabled) {
        applySpellCheck();
    }

    GET_PAGE()
    page->executeJavaScript(m_setupEnToDoTagsJs);
}

void NoteEditorPrivate::changeFontSize(const bool increase)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::changeFontSize: increase = ") << (increase ? QStringLiteral("true") : QStringLiteral("false")));

    int fontSize = m_font.pointSize();
    if (fontSize < 0) {
        QNTRACE(QStringLiteral("Font size is negative which most likely means the font is not set yet, nothing to do. Current font: ") << m_font);
        return;
    }

    QFontDatabase fontDatabase;
    QList<int> fontSizes = fontDatabase.pointSizes(m_font.family(), m_font.styleName());
    if (fontSizes.isEmpty()) {
        QNTRACE(QStringLiteral("Coulnd't find point sizes for font family ") << m_font.family()
                << QStringLiteral(", will use standard sizes instead"));
        fontSizes = fontDatabase.standardSizes();
    }

    int fontSizeIndex = fontSizes.indexOf(fontSize);
    if (fontSizeIndex < 0)
    {
        QNTRACE(QStringLiteral("Couldn't find font size ") << fontSize << QStringLiteral(" within the available sizes, will take the closest one instead"));
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
                QNTRACE(QStringLiteral("Updated current closest index to ") << i << QStringLiteral(": font size = ") << value);
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
            QNTRACE(QStringLiteral("Can't ") << (increase ? QStringLiteral("increase") : QStringLiteral("decrease"))
                    << QStringLiteral(" the font size: hit the boundary of available font sizes"));
            return;
        }
    }
    else
    {
        QNTRACE(QStringLiteral("Wasn't able to find even the closest font size within the available ones, will simply ")
                << (increase ? QStringLiteral("increase") : QStringLiteral("decrease"))
                << QStringLiteral(" the given font size by 1 pt and see what happens"));
        if (increase)
        {
            ++fontSize;
        }
        else
        {
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::changeIndentation: increase = ") << (increase ? QStringLiteral("true") : QStringLiteral("false")));
    execJavascriptCommand(increase ? QStringLiteral("indent") : QStringLiteral("outdent"));
    setModified();
}

void NoteEditorPrivate::findText(const QString & textToFind, const bool matchCase, const bool searchBackward,
                                 NoteEditorPage::Callback callback) const
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::findText: ") << textToFind << QStringLiteral("; match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral(", search backward = ") << (searchBackward ? QStringLiteral("true") : QStringLiteral("false")));

    GET_PAGE()

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    QString escapedTextToFind = textToFind;
    ENMLConverter::escapeString(escapedTextToFind);

    // The order of used parameters to window.find: text to find, match case (bool), search backwards (bool), wrap the search around (bool)
    QString javascript = QStringLiteral("window.find('") + escapedTextToFind + QStringLiteral("', ") +
                         (matchCase ? QStringLiteral("true") : QStringLiteral("false")) + QStringLiteral(", ") +
                         (searchBackward ? QStringLiteral("true") : QStringLiteral("false")) + QStringLiteral(", true);");

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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setSearchHighlight: ") << textToFind << QStringLiteral("; match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false"))
            << QStringLiteral("; force = ") << (force ? QStringLiteral("true") : QStringLiteral("false")));

    if ( !force && (textToFind.compare(m_lastSearchHighlightedText, (matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive)) == 0) &&
         (m_lastSearchHighlightedTextCaseSensitivity == matchCase) )
    {
        QNTRACE(QStringLiteral("The text to find matches the one highlighted the last time as well as its case sensitivity"));
        return;
    }

    m_lastSearchHighlightedText = textToFind;
    m_lastSearchHighlightedTextCaseSensitivity = matchCase;

    QString escapedTextToFind = textToFind;
    ENMLConverter::escapeString(escapedTextToFind, /* simplify = */ false);

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("findReplaceManager.setSearchHighlight('") + escapedTextToFind + QStringLiteral("', ") +
                            (matchCase ? QStringLiteral("true") : QStringLiteral("false")) + QStringLiteral(");"));

    highlightRecognizedImageAreas(textToFind, matchCase);
}

void NoteEditorPrivate::highlightRecognizedImageAreas(const QString & textToFind, const bool matchCase) const
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::highlightRecognizedImageAreas"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("imageAreasHilitor.clearImageHilitors();"));

    if (m_lastSearchHighlightedText.isEmpty()) {
        QNTRACE(QStringLiteral("Last search highlighted text is empty"));
        return;
    }

    QString escapedTextToFind = m_lastSearchHighlightedText;
    ENMLConverter::escapeString(escapedTextToFind);

    if (escapedTextToFind.isEmpty()) {
        QNTRACE(QStringLiteral("Escaped search highlighted text is empty"));
        return;
    }

    for(auto it = m_recognitionIndicesByResourceHash.begin(), end = m_recognitionIndicesByResourceHash.end(); it != end; ++it)
    {
        const QByteArray & resourceHash = it.key();
        const ResourceRecognitionIndices & recoIndices = it.value();
        QNTRACE(QStringLiteral("Processing recognition data for resource hash ") << resourceHash.toHex());

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
                    QNTRACE(QStringLiteral("Found text item matching with the text to find: ") << textItem.m_text);
                    matchFound = true;
                }
            }

            if (matchFound) {
                page->executeJavaScript(QStringLiteral("imageAreasHilitor.hiliteImageArea('") + resourceHash.toHex() + QStringLiteral("', ") +
                                        QString::number(recoIndexItem.x()) + QStringLiteral(", ") + QString::number(recoIndexItem.y()) + QStringLiteral(", ") +
                                        QString::number(recoIndexItem.h()) + QStringLiteral(", ") + QString::number(recoIndexItem.w()) + QStringLiteral(");"));
            }
        }
    }
}

void NoteEditorPrivate::clearEditorContent()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::clearEditorContent"));

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

    QString initialHtml = initialPageHtml();
    writeNotePageFile(initialHtml);
}

void NoteEditorPrivate::noteToEditorContent()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::noteToEditorContent"));

    if (m_pNote.isNull()) {
        QNDEBUG(QStringLiteral("No note has been set yet"));
        clearEditorContent();
        return;
    }

    if (m_pNote->isInkNote()) {
        inkNoteToEditorContent();
        return;
    }

    QString noteContent;
    if (m_pNote->hasContent()) {
        noteContent = m_pNote->content();
    }
    else {
        QNDEBUG(QStringLiteral("Note without content was inserted into the NoteEditor, "
                               "setting up the empty note content"));
        noteContent = QStringLiteral("<en-note><div></div></en-note>");
    }

    m_htmlCachedMemory.resize(0);

    ErrorString error;
    ENMLConverter::NoteContentToHtmlExtraData extraData;
    bool res = m_enmlConverter.noteContentToHtml(noteContent, m_htmlCachedMemory,
                                                 error, *m_decryptedTextManager,
                                                 extraData);
    if (!res) {
        QNWARNING(QStringLiteral("Can't convert note's content to HTML: ") << m_errorCachedMemory);
        emit notifyError(error);
        clearEditorContent();
        return;
    }

    m_lastFreeEnToDoIdNumber = extraData.m_numEnToDoNodes + 1;
    m_lastFreeHyperlinkIdNumber = extraData.m_numHyperlinkNodes + 1;
    m_lastFreeEnCryptIdNumber = extraData.m_numEnCryptNodes + 1;
    m_lastFreeEnDecryptedIdNumber = extraData.m_numEnDecryptedNodes + 1;

    int bodyTagIndex = m_htmlCachedMemory.indexOf(QStringLiteral("<body"));
    if (bodyTagIndex < 0) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't find <body> tag in the result of note to HTML conversion"));
        QNWARNING(error << QStringLiteral(", note content: ") << m_pNote->content()
                  << QStringLiteral(", html: ") << m_htmlCachedMemory);
        emit notifyError(error);
        clearEditorContent();
        return;
    }

    m_htmlCachedMemory.replace(0, bodyTagIndex, m_pagePrefix);

    int bodyClosingTagIndex = m_htmlCachedMemory.indexOf(QStringLiteral("</body>"));
    if (bodyClosingTagIndex < 0) {
        error.base() = QT_TRANSLATE_NOOP("", "can't find </body> tag in the result of note to HTML conversion");
        QNWARNING(error << QStringLiteral(", note content: ") << m_pNote->content()
                  << QStringLiteral(", html: ") << m_htmlCachedMemory);
        emit notifyError(error);
        clearEditorContent();
        return;
    }

    m_htmlCachedMemory.insert(bodyClosingTagIndex + 7, QStringLiteral("</html>"));
    m_htmlCachedMemory.replace(QStringLiteral("<br></br>"), QStringLiteral("</br>"));   // Webkit-specific fix
    writeNotePageFile(m_htmlCachedMemory);
}

void NoteEditorPrivate::updateColResizableTableBindings()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::updateColResizableTableBindings"));

    bool readOnly = !isPageEditable();

    QString javascript;
    if (readOnly) {
        javascript = QStringLiteral("tableManager.disableColumnHandles(\"table\");");
    }
    else {
        javascript = QStringLiteral("tableManager.updateColumnHandles(\"table\");");
    }

    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::inkNoteToEditorContent()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::inkNoteToEditorContent"));

    m_lastFreeEnToDoIdNumber = 1;
    m_lastFreeHyperlinkIdNumber = 1;
    m_lastFreeEnCryptIdNumber = 1;
    m_lastFreeEnDecryptedIdNumber = 1;

    bool problemDetected = false;
    QList<Resource> resources = m_pNote->resources();
    const int numResources = resources.size();

    QString inkNoteHtml = m_pagePrefix;
    inkNoteHtml += QStringLiteral("<body>");

    for(int i = 0; i < numResources; ++i)
    {
        const Resource & resource = resources[i];

        if (!resource.hasGuid()) {
            QNWARNING(QStringLiteral("Detected ink note which has at least one resource without guid: note = ") << *m_pNote
                      << QStringLiteral("\nResource: ") << resource);
            problemDetected = true;
            break;
        }

        if (!resource.hasDataHash()) {
            QNWARNING(QStringLiteral("Detected ink note which has at least one resource without data hash: note = ") << *m_pNote
                      << QStringLiteral("\nResource: ") << resource);
            problemDetected = true;
            break;
        }

        QFileInfo inkNoteImageFileInfo(m_noteEditorPageFolderPath + QStringLiteral("/inkNoteImages/") + resource.guid() + QStringLiteral(".png"));
        if (!inkNoteImageFileInfo.exists() || !inkNoteImageFileInfo.isFile() || !inkNoteImageFileInfo.isReadable()) {
            QNWARNING(QStringLiteral("Detected broken or nonexistent ink note image file, check file at path ") << inkNoteImageFileInfo.absoluteFilePath());
            problemDetected = true;
            break;
        }

        QString inkNoteImageFilePath = inkNoteImageFileInfo.absoluteFilePath();
        ENMLConverter::escapeString(inkNoteImageFilePath);
        if (Q_UNLIKELY(inkNoteImageFilePath.isEmpty())) {
            QNWARNING(QStringLiteral("Unable to escape the ink note image file path: ") << inkNoteImageFileInfo.absoluteFilePath());
            problemDetected = true;
            break;
        }

        inkNoteHtml += QStringLiteral("<img src=\"");
        inkNoteHtml += inkNoteImageFilePath;
        inkNoteHtml += QStringLiteral("\" ");

        if (resource.hasHeight()) {
            inkNoteHtml += QStringLiteral("height=");
            inkNoteHtml += QString::number(resource.height());
            inkNoteHtml += QStringLiteral(" ");
        }

        if (resource.hasWidth()) {
            inkNoteHtml += QStringLiteral("width=");
            inkNoteHtml += QString::number(resource.width());
            inkNoteHtml += QStringLiteral(" ");
        }

        inkNoteHtml += "/>";
    }

    if (problemDetected) {
        inkNoteHtml = m_pagePrefix;
        inkNoteHtml += QStringLiteral("<body><div>");
        inkNoteHtml += tr("The read-only ink note image should have been present here but something went wrong so the image is not accessible");
        inkNoteHtml += QStringLiteral("</div></body></html>");
    }

    QNTRACE(QStringLiteral("Ink note html: ") << inkNoteHtml);
    writeNotePageFile(inkNoteHtml);
}

bool NoteEditorPrivate::htmlToNoteContent(ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::htmlToNoteContent"));

    if (m_pNote.isNull()) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "no note was set to note editor");
        emit cantConvertToNote(errorDescription);
        return false;
    }

    if (m_pNote->hasActive() && !m_pNote->active()) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "current note is marked as read-only, the changes won't be saved");
        QNINFO(errorDescription << QStringLiteral(", note: local uid = ") << m_pNote->localUid()
               << QStringLiteral(", guid = ") << (m_pNote->hasGuid() ? m_pNote->guid() : QStringLiteral("<null>"))
               << QStringLiteral(", title = ") << (m_pNote->hasTitle() ? m_pNote->title() : QStringLiteral("<null>")));
        emit cantConvertToNote(errorDescription);
        return false;
    }

    if (!m_pNotebook.isNull() && m_pNotebook->hasRestrictions())
    {
        const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
        if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref()) {
            errorDescription.base() = QT_TRANSLATE_NOOP("", "The notebook the current note belongs to doesn't allow "
                                                        "notes modification, the changes won't be saved");
            QNINFO(errorDescription << QStringLiteral(", note: local uid = ") << m_pNote->localUid()
                   << QStringLiteral(", guid = ") << (m_pNote->hasGuid() ? m_pNote->guid() : QStringLiteral("<null>"))
                   << QStringLiteral(", title = ") << (m_pNote->hasTitle() ? m_pNote->title() : QStringLiteral("<null>"))
                   << QStringLiteral(", notebook: local uid = ") << m_pNotebook->localUid() << QStringLiteral(", guid = ")
                   << (m_pNotebook->hasGuid() ? m_pNotebook->guid() : QStringLiteral("<null>")) << QStringLiteral(", name = ")
                   << (m_pNotebook->hasName() ? m_pNotebook->name() : QStringLiteral("<null>")));
            emit cantConvertToNote(errorDescription);
            return false;
        }
    }

    m_pendingConversionToNote = true;

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    m_htmlCachedMemory = page()->mainFrame()->toHtml();
    onPageHtmlReceived(m_htmlCachedMemory);
#else
    page()->toHtml(NoteEditorCallbackFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
#endif

    return true;
}

void NoteEditorPrivate::saveNoteResourcesToLocalFiles()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::saveNoteResourcesToLocalFiles"));

    if (m_pNote.isNull()) {
        QNTRACE(QStringLiteral("No note is set for the editor"));
        return;
    }

    QList<Resource> resources = m_pNote->resources();
    if (resources.isEmpty()) {
        QNTRACE(QStringLiteral("Note has no resources, nothing to do"));
        return;
    }

    auto resourcesConstBegin = resources.constBegin();
    auto resourcesConstEnd = resources.constEnd();

    size_t numPendingResourceWritesToLocalFiles = 0;

    for(auto it = resourcesConstBegin; it != resourcesConstEnd; ++it)
    {
        const Resource & resource = *it;

        if (!resource.hasDataBody() && !resource.hasAlternateDataBody()) {
            QNINFO(QStringLiteral("Detected resource without data body: ") << resource);
            continue;
        }

        if (!resource.hasDataHash() && !resource.hasAlternateDataHash()) {
            QNINFO(QStringLiteral("Detected resource without data hash: ") << resource);
            continue;
        }

        if (!resource.hasMime()) {
            QNINFO(QStringLiteral("Detected resource without mime type: ") << resource);
            continue;
        }

        const QByteArray & dataHash = (resource.hasDataHash()
                                       ? resource.dataHash()
                                       : resource.alternateDataHash());

        QNTRACE(QStringLiteral("Found current note's resource corresponding to the data hash ")
                << dataHash.toHex() << ": " << resource);

        if (!m_resourceInfo.contains(dataHash))
        {
            bool res = saveResourceToLocalFile(resource);
            if (res) {
                ++numPendingResourceWritesToLocalFiles;
            }
        }
    }

    if (numPendingResourceWritesToLocalFiles == 0)
    {
        QNTRACE(QStringLiteral("All current note's resources are written to local files and are actual. "
                               "Will set filepaths to these local files to src attributes of img resource tags"));
        if (!m_pendingNotePageLoad) {
            provideSrcForResourceImgTags();
        }
        return;
    }

    QNTRACE(QStringLiteral("Scheduled writing of ") << numPendingResourceWritesToLocalFiles
            << QStringLiteral(" to local files, will wait until they are written "
                              "and add the src attributes to img resources when the files are ready"));
}

bool NoteEditorPrivate::saveResourceToLocalFile(const Resource & resource)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::saveResourceToLocalFile: ") << resource);

    if (Q_UNLIKELY(!resource.hasMime())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save the resource to local file: no resource mime type"));
        QNWARNING(error << QStringLiteral(", resource: ") << resource);
        emit notifyError(error);
        return false;
    }

    if (Q_UNLIKELY(!resource.hasNoteLocalUid())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save the resource to local file: the resource has no note local uid"));
        QNWARNING(error << QStringLiteral(", resource: ") << resource);
        emit notifyError(error);
        return false;
    }

    if (Q_UNLIKELY(!resource.hasDataBody() && !resource.hasAlternateDataBody())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save the resource to local file: resource has neither data body nor alternate data body"));
        QNWARNING(error << QStringLiteral(", resource: ") << resource);
        emit notifyError(error);
        return false;
    }

    QUuid saveResourceRequestId = QUuid::createUuid();
    QString preferredFileSuffix = resource.preferredFileSuffix();

    m_genericResourceLocalUidBySaveToStorageRequestIds[saveResourceRequestId] = resource.localUid();

    bool isImage = resource.mime().startsWith(QStringLiteral("image"));
    if (isImage) {
        Q_UNUSED(m_imageResourceSaveToStorageRequestIds.insert(saveResourceRequestId))
    }

    QByteArray dataBody = (resource.hasDataBody()
                           ? resource.dataBody()
                           : resource.alternateDataBody());

    QNTRACE(QStringLiteral("Emitting the request to save the resource to file storage: note local uid = ") << resource.noteLocalUid()
            << QStringLiteral(", resource local uid = ") << resource.localUid() << QStringLiteral(", preferred file suffix = ")
            << preferredFileSuffix << QStringLiteral(", request id = ") << saveResourceRequestId << QStringLiteral(", resource is image = ")
            << (isImage ? QStringLiteral("true") : QStringLiteral("false")));
    emit saveResourceToStorage(resource.noteLocalUid(), resource.localUid(), dataBody,
                               QByteArray(), preferredFileSuffix, saveResourceRequestId, isImage);
    return true;
}

void NoteEditorPrivate::updateHashForResourceTag(const QByteArray & oldResourceHash, const QByteArray & newResourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::updateHashForResourceTag: old hash = ") << oldResourceHash.toHex()
            << QStringLiteral(", new hash = ") << newResourceHash.toHex());

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("updateResourceHash('") + oldResourceHash.toHex() + QStringLiteral("', '") +
                            newResourceHash.toHex() + QStringLiteral("');"));
}

void NoteEditorPrivate::provideSrcForResourceImgTags()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::provideSrcForResourceImgTags"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("provideSrcForResourceImgTags();"));
}

void NoteEditorPrivate::manualSaveResourceToFile(const Resource & resource)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::manualSaveResourceToFile"));

    if (Q_UNLIKELY(!resource.hasDataBody() && !resource.hasAlternateDataBody())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save resource to file: resource has neither data body nor alternate data body"));
        QNINFO(error << QStringLiteral(", resource: ") << resource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!resource.hasMime())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save resource to file: resource has no mime type"));
        QNINFO(error << QStringLiteral(", resource: ") << resource);
        emit notifyError(error);
        return;
    }

    QString resourcePreferredSuffix = resource.preferredFileSuffix();
    QString resourcePreferredFilterString;
    if (!resourcePreferredSuffix.isEmpty()) {
        resourcePreferredFilterString = QStringLiteral("(*.") + resourcePreferredSuffix + QStringLiteral(")");
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
            ErrorString error(QT_TRANSLATE_NOOP("", "can't save resource to file: can't identify resource's mime type"));
            QNINFO(error << QStringLiteral(", mime type name: ") << mimeTypeName);
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
            filterString += QStringLiteral(";;") + resourcePreferredFilterString;
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
        int attachmentsSaveLocGroupIndex = childGroups.indexOf(QStringLiteral("AttachmentSaveLocations"));
        if (attachmentsSaveLocGroupIndex >= 0)
        {
            QNTRACE(QStringLiteral("Found cached attachment save location group within application settings"));

            appSettings.beginGroup(QStringLiteral("AttachmentSaveLocations"));
            QStringList cachedFileSuffixes = appSettings.childKeys();
            const int numPreferredSuffixes = preferredSuffixes.size();
            for(int i = 0; i < numPreferredSuffixes; ++i)
            {
                preferredSuffix = preferredSuffixes[i];
                int indexInCache = cachedFileSuffixes.indexOf(preferredSuffix);
                if (indexInCache < 0) {
                    QNTRACE(QStringLiteral("Haven't found cached attachment save directory for file suffix ") << preferredSuffix);
                    continue;
                }

                QVariant dirValue = appSettings.value(preferredSuffix);
                if (dirValue.isNull() || !dirValue.isValid()) {
                    QNTRACE(QStringLiteral("Found inappropriate attachment save directory for file suffix ") << preferredSuffix);
                    continue;
                }

                QFileInfo dirInfo(dirValue.toString());
                if (!dirInfo.exists()) {
                    QNTRACE(QStringLiteral("Cached attachment save directory for file suffix ") << preferredSuffix
                            << QStringLiteral(" does not exist: ") << dirInfo.absolutePath());
                    continue;
                }
                else if (!dirInfo.isDir()) {
                    QNTRACE(QStringLiteral("Cached attachment save directory for file suffix ") << preferredSuffix
                            << QStringLiteral(" is not a directory: ") << dirInfo.absolutePath());
                    continue;
                }
                else if (!dirInfo.isWritable()) {
                    QNTRACE(QStringLiteral("Cached attachment save directory for file suffix ") << preferredSuffix
                            << QStringLiteral(" is not writable: ") << dirInfo.absolutePath());
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
        QNINFO(QStringLiteral("User cancelled saving resource to file"));
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
        absoluteFilePath += QStringLiteral(".") + preferredSuffix;
    }

    QUuid saveResourceToFileRequestId = QUuid::createUuid();

    const QByteArray & data = (resource.hasDataBody()
                               ? resource.dataBody()
                               : resource.alternateDataBody());

    Q_UNUSED(m_manualSaveResourceToFileRequestIds.insert(saveResourceToFileRequestId));
    emit saveResourceToFile(absoluteFilePath, data, saveResourceToFileRequestId, /* append = */ false);
    QNDEBUG(QStringLiteral("Sent request to manually save resource to file, request id = ") << saveResourceToFileRequestId
            << QStringLiteral(", resource local uid = ") << resource.localUid());
}

void NoteEditorPrivate::openResource(const QString & resourceAbsoluteFilePath)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::openResource: ") << resourceAbsoluteFilePath);

    QFile resourceFile(resourceAbsoluteFilePath);
    if (Q_UNLIKELY(!resourceFile.exists())) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't open attachment: the attachment file does not exist"));
        QNWARNING(errorDescription << QStringLiteral(", supposed file path: ") << resourceAbsoluteFilePath);
        emit notifyError(errorDescription);
        return;
    }

    QString symLinkTarget = resourceFile.symLinkTarget();
    if (!symLinkTarget.isEmpty()) {
        QNDEBUG(QStringLiteral("The resource file is actually a symlink, substituting the file path to open to the real file: ")
                << symLinkTarget);
        emit openResourceFile(symLinkTarget);
    }
    else {
        emit openResourceFile(resourceAbsoluteFilePath);
    }
}

QImage NoteEditorPrivate::buildGenericResourceImage(const Resource & resource)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::buildGenericResourceImage"));

    QString resourceDisplayName = resource.displayName();
    if (Q_UNLIKELY(resourceDisplayName.isEmpty())) {
        resourceDisplayName = tr("Attachment");
    }

    QNTRACE(QStringLiteral("Resource display name = ") << resourceDisplayName);

    QFont font = m_font;
    font.setPointSize(10);

    QString originalResourceDisplayName = resourceDisplayName;

    const int maxResourceDisplayNameWidth = 146;
    QFontMetrics fontMetrics(font);
    int width = fontMetrics.width(resourceDisplayName);
    int singleCharWidth = fontMetrics.width(QStringLiteral("n"));
    int ellipsisWidth = fontMetrics.width(QStringLiteral("..."));

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

        int dotIndex = resourceDisplayName.lastIndexOf(QStringLiteral("."));
        if (dotIndex != 0 && (dotIndex > resourceDisplayName.size() / 2))
        {
            // Try to shorten the name while preserving the file extension. Need to skip some chars before the dot index
            int startSkipPos = dotIndex - numCharsToSkip;
            if (startSkipPos >= 0) {
                resourceDisplayName.replace(startSkipPos, numCharsToSkip, QStringLiteral("..."));
                width = fontMetrics.width(resourceDisplayName);
                continue;
            }
        }

        // Either no file extension or name contains a dot, skip some chars without attempt to preserve the file extension
        resourceDisplayName.replace(resourceDisplayName.size() - numCharsToSkip, numCharsToSkip, QStringLiteral("..."));
        width = fontMetrics.width(resourceDisplayName);
    }

    if (!smartReplaceWorked)
    {
        QNTRACE(QStringLiteral("Wasn't able to shorten the resource name nicely, will try to shorten it just somehow"));
        width = fontMetrics.width(originalResourceDisplayName);
        int widthOverflow = width - maxResourceDisplayNameWidth;
        int numCharsToSkip = (widthOverflow + ellipsisWidth) / singleCharWidth + 1;
        resourceDisplayName = originalResourceDisplayName;

        if (resourceDisplayName.size() > numCharsToSkip) {
            resourceDisplayName.replace(resourceDisplayName.size() - numCharsToSkip, numCharsToSkip, QStringLiteral("..."));
        }
        else {
            resourceDisplayName = QStringLiteral("Attachment...");
        }
    }

    QNTRACE(QStringLiteral("(possibly) shortened resource display name: ") << resourceDisplayName
            << QStringLiteral(", width = ") << fontMetrics.width(resourceDisplayName));

    QString resourceHumanReadableSize;
    if (resource.hasDataSize() || resource.hasAlternateDataSize()) {
        resourceHumanReadableSize = humanReadableSize(resource.hasDataSize()
                                                      ? static_cast<quint64>(resource.dataSize())
                                                      : static_cast<quint64>(resource.alternateDataSize()));
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
                QNTRACE(QStringLiteral("Can't get icon from theme by name ") << mimeType.genericIconName());
                useFallbackGenericResourceIcon = true;
            }
        }
        else
        {
            QNTRACE(QStringLiteral("Can't get valid mime type for name ") << resourceMimeTypeName
                    << QStringLiteral(", will use fallback generic resource icon"));
            useFallbackGenericResourceIcon = true;
        }
    }
    else
    {
        QNINFO(QStringLiteral("Found resource without mime type set: ") << resource);
        QNTRACE(QStringLiteral("Will use fallback generic resource icon"));
        useFallbackGenericResourceIcon = true;
    }

    if (useFallbackGenericResourceIcon) {
        resourceIcon = QIcon(QStringLiteral(":/generic_resource_icons/png/attachment.png"));
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
    QIcon openResourceIcon = QIcon::fromTheme(QStringLiteral("document-open"), QIcon(QStringLiteral(":/generic_resource_icons/png/open_with.png")));
    painter.drawPixmap(QPoint(174,4), openResourceIcon.pixmap(QSize(24,24)));

    // Draw save resource icon
    QIcon saveResourceIcon = QIcon::fromTheme(QStringLiteral("document-save"), QIcon(QStringLiteral(":/generic_resource_icons/png/save.png")));
    painter.drawPixmap(QPoint(202,4), saveResourceIcon.pixmap(QSize(24,24)));

    painter.end();
    return pixmap.toImage();
}

void NoteEditorPrivate::saveGenericResourceImage(const Resource & resource, const QImage & image)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::saveGenericResourceImage: resource local uid = ") << resource.localUid());

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save the generic resource image: no note is set to the editor"));
        QNWARNING(error << QStringLiteral(", resource: ") << resource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!resource.hasDataHash() && !resource.hasAlternateDataHash())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save generic resource image: resource has neither data hash nor alternate data hash"));
        QNWARNING(error << QStringLiteral(", resource: ") << resource);
        emit notifyError(error);
        return;
    }

    QByteArray imageData;
    QBuffer buffer(&imageData);
    Q_UNUSED(buffer.open(QIODevice::WriteOnly));
    image.save(&buffer, "PNG");

    QUuid requestId = QUuid::createUuid();
    Q_UNUSED(m_saveGenericResourceImageToFileRequestIds.insert(requestId));

    QNDEBUG(QStringLiteral("Emitting request to write generic resource image for resource with local uid ")
            << resource.localUid() << QStringLiteral(", request id ") << requestId);
    emit saveGenericResourceImageToFile(m_pNote->localUid(), resource.localUid(), imageData, QStringLiteral("png"),
                                        (resource.hasDataHash() ? resource.dataHash() : resource.alternateDataHash()),
                                        resource.displayName(), requestId);
}

#ifdef QUENTIER_USE_QT_WEB_ENGINE
void NoteEditorPrivate::provideSrcAndOnClickScriptForImgEnCryptTags()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::provideSrcAndOnClickScriptForImgEnCryptTags"));

    if (Q_UNLIKELY(m_pNote.isNull())) {
        QNTRACE(QStringLiteral("No note is set for the editor"));
        return;
    }

    if (!m_pNote->containsEncryption()) {
        QNTRACE(QStringLiteral("Current note doesn't contain any encryption, nothing to do"));
        return;
    }

    QString iconPath = QStringLiteral("qrc:/encrypted_area_icons/en-crypt/en-crypt.png");
    QString javascript = QStringLiteral("provideSrcAndOnClickScriptForEnCryptImgTags(\"") + iconPath + QStringLiteral("\")");

    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::setupGenericResourceImages()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupGenericResourceImages"));

    if (m_pNote.isNull()) {
        QNDEBUG(QStringLiteral("No note to build generic resource images for"));
        return;
    }

    if (!m_pNote->hasResources()) {
        QNDEBUG(QStringLiteral("Note has no resources, nothing to do"));
        return;
    }

    QString mimeTypeName;
    size_t resourceImagesCounter = 0;
    bool shouldWaitForResourceImagesToSave = false;

    QList<Resource> resources = m_pNote->resources();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const Resource & resource = resources[i];

        if (resource.hasMime())
        {
            mimeTypeName = resource.mime();
            if (mimeTypeName.startsWith(QStringLiteral("image/"))) {
                QNTRACE(QStringLiteral("Skipping image resource ") << resource);
                continue;
            }
        }

        shouldWaitForResourceImagesToSave |= findOrBuildGenericResourceImage(resource);
        ++resourceImagesCounter;
    }

    if (resourceImagesCounter == 0) {
        QNDEBUG(QStringLiteral("No generic resources requiring building custom images were found"));
        return;
    }

    if (shouldWaitForResourceImagesToSave) {
        QNTRACE(QStringLiteral("Some generic resource images are being saved to files, waiting"));
        return;
    }

    QNTRACE(QStringLiteral("All generic resource images are ready"));
    provideSrcForGenericResourceImages();
    setupGenericResourceOnClickHandler();
}

bool NoteEditorPrivate::findOrBuildGenericResourceImage(const Resource & resource)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::findOrBuildGenericResourceImage: ") << resource);

    if (!resource.hasDataHash() && !resource.hasAlternateDataHash()) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "found resource without either data hash or alternate data hash"));
        QNWARNING(errorDescription << QStringLiteral(": ") << resource);
        emit notifyError(errorDescription);
        return true;
    }

    const QString localUid = resource.localUid();

    const QByteArray & resourceHash = (resource.hasDataHash()
                                       ? resource.dataHash()
                                       : resource.alternateDataHash());

    QNTRACE(QStringLiteral("Looking for existing generic resource image file for resource with hash ") << resourceHash.toHex());
    auto it = m_genericResourceImageFilePathsByResourceHash.find(resourceHash);
    if (it != m_genericResourceImageFilePathsByResourceHash.end()) {
        QNTRACE(QStringLiteral("Found generic resource image file path for resource with hash ") << resourceHash.toHex()
                << QStringLiteral(" and local uid ") << localUid << QStringLiteral(": ") << it.value());
        return false;
    }

    QImage img = buildGenericResourceImage(resource);
    if (img.isNull()) {
        QNDEBUG(QStringLiteral("Can't build generic resource image"));
        return true;
    }

    saveGenericResourceImage(resource, img);
    return true;
}

void NoteEditorPrivate::provideSrcForGenericResourceImages()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::provideSrcForGenericResourceImages"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("provideSrcForGenericResourceImages();"));
}

void NoteEditorPrivate::setupGenericResourceOnClickHandler()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupGenericResourceOnClickHandler"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("setupGenericResourceOnClickHandler();"));
}

void NoteEditorPrivate::setupWebSocketServer()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupWebSocketServer"));

    if (!m_pWebSocketServer->listen(QHostAddress::LocalHost, 0)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't open web socket server"));
        error.details() = m_pWebSocketServer->errorString();
        QNFATAL(error);
        throw NoteEditorInitializationException(error);
    }

    m_webSocketServerPort = m_pWebSocketServer->serverPort();
    QNDEBUG(QStringLiteral("Using automatically selected websocket server port ") << m_webSocketServerPort);

    QObject::connect(m_pWebSocketClientWrapper, &WebSocketClientWrapper::clientConnected,
                     m_pWebChannel, &QWebChannel::connectTo);
}

void NoteEditorPrivate::setupJavaScriptObjects()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupJavaScriptObjects"));

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

    m_pWebChannel->registerObject(QStringLiteral("resourceCache"), m_pResourceInfoJavaScriptHandler);
    m_pWebChannel->registerObject(QStringLiteral("enCryptElementClickHandler"), m_pEnCryptElementClickHandler);
    m_pWebChannel->registerObject(QStringLiteral("pageMutationObserver"), m_pPageMutationHandler);
    m_pWebChannel->registerObject(QStringLiteral("openAndSaveResourceButtonsHandler"), m_pGenericResourceOpenAndSaveButtonsOnClickHandler);
    m_pWebChannel->registerObject(QStringLiteral("textCursorPositionHandler"), m_pTextCursorPositionJavaScriptHandler);
    m_pWebChannel->registerObject(QStringLiteral("contextMenuEventHandler"), m_pContextMenuEventJavaScriptHandler);
    m_pWebChannel->registerObject(QStringLiteral("genericResourceImageHandler"), m_pGenericResoureImageJavaScriptHandler);
    m_pWebChannel->registerObject(QStringLiteral("hyperlinkClickHandler"), m_pHyperlinkClickJavaScriptHandler);
    m_pWebChannel->registerObject(QStringLiteral("toDoCheckboxClickHandler"), m_pToDoCheckboxClickHandler);
    m_pWebChannel->registerObject(QStringLiteral("tableResizeHandler"), m_pTableResizeJavaScriptHandler);
    m_pWebChannel->registerObject(QStringLiteral("resizableImageHandler"), m_pResizableImageJavaScriptHandler);
    m_pWebChannel->registerObject(QStringLiteral("spellCheckerDynamicHelper"), m_pSpellCheckerDynamicHandler);
    QNDEBUG(QStringLiteral("Registered objects exposed to JavaScript"));
}

void NoteEditorPrivate::setupTextCursorPositionTracking()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupTextCursorPositionTracking"));

    QString javascript = QStringLiteral("setupTextCursorPositionTracking();");

    GET_PAGE()
    page->executeJavaScript(javascript);
}

#endif // QUENTIER_USE_QT_WEB_ENGINE

void NoteEditorPrivate::updateResource(const QString & resourceLocalUid, const QByteArray & previousResourceHash,
                                       Resource updatedResource)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::updateResource: resource local uid = ") << resourceLocalUid
            << QStringLiteral(", previous hash = ") << previousResourceHash.toHex() << QStringLiteral(", updated resource: ")
            << updatedResource);

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't update the resource: no note is set to the editor"));
        QNWARNING(error << QStringLiteral(", updated resource: ") << updatedResource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!updatedResource.hasNoteLocalUid())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't update the resource: the updated resource has no note local uid"));
        QNWARNING(error << QStringLiteral(", updated resource: ") << updatedResource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!updatedResource.hasMime())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't update the resource: the updated resource has no mime type"));
        QNWARNING(error << QStringLiteral(", updated resource: ") << updatedResource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!updatedResource.hasDataBody())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't update the resource: the updated resource contains no data body"));
        QNWARNING(error << QStringLiteral(", updated resource: ") << updatedResource);
        emit notifyError(error);
        return;
    }

    // NOTE: intentionally set the "wrong", "stale" hash value here, it is required for proper update procedure
    // once the resource is saved in the local file storage; it's kinda hacky but it seems the simplest option
    updatedResource.setDataHash(previousResourceHash);

    bool res = m_pNote->updateResource(updatedResource);
    if (Q_UNLIKELY(!res)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't update the resource: resource to be updated was not found within the note"));
        QNWARNING(error << QStringLiteral(", updated resource: ") << updatedResource << QStringLiteral("\nNote: ") << *m_pNote);
        emit notifyError(error);
        return;
    }

    emit convertedToNote(*m_pNote);

    m_resourceInfo.removeResourceInfo(previousResourceHash);

    auto recoIt = m_recognitionIndicesByResourceHash.find(previousResourceHash);
    if (recoIt != m_recognitionIndicesByResourceHash.end()) {
        Q_UNUSED(m_recognitionIndicesByResourceHash.erase(recoIt));
    }

    Q_UNUSED(saveResourceToLocalFile(updatedResource))
}

void NoteEditorPrivate::setupGenericTextContextMenu(const QStringList & extraData, const QString & selectedHtml, bool insideDecryptedTextFragment)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupGenericTextContextMenu: selected html = ") << selectedHtml
            << QStringLiteral("; inside decrypted text fragment = ")
            << (insideDecryptedTextFragment ? QStringLiteral("true") : QStringLiteral("false")));

    m_lastSelectedHtml = selectedHtml;

    delete m_pGenericTextContextMenu;
    m_pGenericTextContextMenu = new QMenu(this);

#define ADD_ACTION_WITH_SHORTCUT(key, name, menu, slot, enabled, ...) \
    { \
        QAction * action = new QAction(name, menu); \
        action->setEnabled(enabled); \
        setupActionShortcut(key, QString(#__VA_ARGS__), *action); \
        QObject::connect(action, QNSIGNAL(QAction,triggered), this, QNSLOT(NoteEditorPrivate,slot)); \
        menu->addAction(action); \
    }

    bool enabled = true;

    // See if extraData contains the misspelled word
    QString misSpelledWord;
    const int extraDataSize = extraData.size();
    for(int i = 0; i < extraDataSize; ++i)
    {
        const QString & item = extraData[i];
        if (!item.startsWith(QStringLiteral("MisSpelledWord_"))) {
            continue;
        }

        misSpelledWord = item.mid(15);
    }

    if (!misSpelledWord.isEmpty())
    {
        m_lastMisSpelledWord = misSpelledWord;

        QStringList correctionSuggestions;
        if (m_pSpellChecker) {
            correctionSuggestions = m_pSpellChecker->spellCorrectionSuggestions(misSpelledWord);
        }

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
                action->setEnabled(m_isPageEditable);
                QObject::connect(action, QNSIGNAL(QAction,triggered), this, QNSLOT(NoteEditorPrivate,onSpellCheckCorrectionAction));
                m_pGenericTextContextMenu->addAction(action);
            }

            Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
        }

        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::SpellCheckIgnoreWord, tr("Ignore word"),
                                 m_pGenericTextContextMenu, onSpellCheckIgnoreWordAction, enabled);
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::SpellCheckAddWordToUserDictionary, tr("Add word to user dictionary"),
                                 m_pGenericTextContextMenu, onSpellCheckAddWordToUserDictionaryAction, enabled);
        Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
    }

    if (insideDecryptedTextFragment)
    {
        QString cipher, keyLength, encryptedText, decryptedText, hint, id;
        ErrorString error;
        bool res = parseEncryptedTextContextMenuExtraData(extraData, encryptedText, decryptedText, cipher, keyLength, hint, id, error);
        if (Q_UNLIKELY(!res)) {
            ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't display the encrypted text's context menu"));
            errorDescription.additionalBases().append(error.base());
            errorDescription.additionalBases().append(error.additionalBases());
            errorDescription.details() = error.details();
            QNWARNING(errorDescription);
            emit notifyError(errorDescription);
            return;
        }

        m_currentContextMenuExtraData.m_encryptedText = encryptedText;
        m_currentContextMenuExtraData.m_keyLength = keyLength;
        m_currentContextMenuExtraData.m_cipher = cipher;
        m_currentContextMenuExtraData.m_hint = hint;
        m_currentContextMenuExtraData.m_id = id;
        m_currentContextMenuExtraData.m_decryptedText = decryptedText;
    }

    if (!selectedHtml.isEmpty()) {
        ADD_ACTION_WITH_SHORTCUT(QKeySequence::Cut, tr("Cut"), m_pGenericTextContextMenu, cut, m_isPageEditable);
        ADD_ACTION_WITH_SHORTCUT(QKeySequence::Copy, tr("Copy"), m_pGenericTextContextMenu, copy, enabled);
    }

    setupPasteGenericTextMenuActions();
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Font, tr("Font..."), m_pGenericTextContextMenu, fontMenu, m_isPageEditable);
    setupParagraphSubMenuForGenericTextMenu(selectedHtml);
    setupStyleSubMenuForGenericTextMenu();

    Q_UNUSED(m_pGenericTextContextMenu->addSeparator());

    if (extraData.contains(QStringLiteral("InsideTable"))) {
        QMenu * pTableMenu = m_pGenericTextContextMenu->addMenu(tr("Table"));
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertRow, tr("Insert row"), pTableMenu, insertTableRow, m_isPageEditable);
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertColumn, tr("Insert column"), pTableMenu, insertTableColumn, m_isPageEditable);
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveRow, tr("Remove row"), pTableMenu, removeTableRow, m_isPageEditable);
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveColumn, tr("Remove column"), pTableMenu, removeTableColumn, m_isPageEditable);
        Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
    }
    else {
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertTable, tr("Insert table..."),
                                 m_pGenericTextContextMenu, insertTableDialog, m_isPageEditable);
    }

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertHorizontalLine, tr("Insert horizontal line"),
                             m_pGenericTextContextMenu, insertHorizontalLine, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::AddAttachment, tr("Add attachment..."),
                             m_pGenericTextContextMenu, addAttachmentDialog, m_isPageEditable);

    Q_UNUSED(m_pGenericTextContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertToDoTag, tr("Insert ToDo tag"),
                             m_pGenericTextContextMenu, insertToDoCheckbox, m_isPageEditable);

    QMenu * pHyperlinkMenu = m_pGenericTextContextMenu->addMenu(tr("Hyperlink"));
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::EditHyperlink, tr("Add/edit..."), pHyperlinkMenu, editHyperlinkDialog, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::CopyHyperlink, tr("Copy"), pHyperlinkMenu, copyHyperlink, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveHyperlink, tr("Remove"), pHyperlinkMenu, removeHyperlink, m_isPageEditable);

    if (!insideDecryptedTextFragment && !selectedHtml.isEmpty()) {
        Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Encrypt, tr("Encrypt selected fragment..."),
                                 m_pGenericTextContextMenu, encryptSelectedText, m_isPageEditable);
    }
    else if (insideDecryptedTextFragment) {
        Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Encrypt, tr("Encrypt back"),
                                 m_pGenericTextContextMenu, hideDecryptedTextUnderCursor, m_isPageEditable);
    }

    m_pGenericTextContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupImageResourceContextMenu(const QByteArray & resourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupImageResourceContextMenu: resource hash = ") << resourceHash.toHex());

    m_currentContextMenuExtraData.m_resourceHash = resourceHash;

    delete m_pImageResourceContextMenu;
    m_pImageResourceContextMenu = new QMenu(this);

    bool enabled = true;

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::CopyAttachment, tr("Copy"), m_pImageResourceContextMenu,
                             copyAttachmentUnderCursor, enabled);

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveAttachment, tr("Remove"), m_pImageResourceContextMenu,
                             removeAttachmentUnderCursor, m_isPageEditable);

    Q_UNUSED(m_pImageResourceContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::ImageRotateClockwise, tr("Rotate clockwise"), m_pImageResourceContextMenu,
                             rotateImageAttachmentUnderCursorClockwise, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::ImageRotateCounterClockwise, tr("Rotate countercloskwise"), m_pImageResourceContextMenu,
                             rotateImageAttachmentUnderCursorCounterclockwise, m_isPageEditable);

    Q_UNUSED(m_pImageResourceContextMenu->addSeparator());

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::OpenAttachment, tr("Open"), m_pImageResourceContextMenu,
                             openAttachmentUnderCursor, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::SaveAttachment, tr("Save as..."), m_pImageResourceContextMenu,
                             saveAttachmentUnderCursor, enabled);

    m_pImageResourceContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupNonImageResourceContextMenu(const QByteArray & resourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupNonImageResourceContextMenu: resource hash = ") << resourceHash.toHex());

    m_currentContextMenuExtraData.m_resourceHash = resourceHash;

    delete m_pNonImageResourceContextMenu;
    m_pNonImageResourceContextMenu = new QMenu(this);

    bool enabled = true;

    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Copy, tr("Copy"), m_pNonImageResourceContextMenu, copy, enabled);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RemoveAttachment, tr("Remove"), m_pNonImageResourceContextMenu,
                             removeAttachmentUnderCursor, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::RenameAttachment, tr("Rename"), m_pNonImageResourceContextMenu,
                             renameAttachmentUnderCursor, m_isPageEditable);

    QClipboard * pClipboard = QApplication::clipboard();
    if (pClipboard && pClipboard->mimeData(QClipboard::Clipboard)) {
        QNTRACE(QStringLiteral("Clipboard buffer has something, adding paste action"));
        ADD_ACTION_WITH_SHORTCUT(QKeySequence::Paste, tr("Paste"), m_pNonImageResourceContextMenu, paste, m_isPageEditable);
    }

    m_pNonImageResourceContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupEncryptedTextContextMenu(const QString & cipher, const QString & keyLength,
                                                      const QString & encryptedText, const QString & hint,
                                                      const QString & id)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupEncryptedTextContextMenu: cipher = ") << cipher
            << QStringLiteral(", key length = ") << keyLength << QStringLiteral(", encrypted text = ")
            << encryptedText << QStringLiteral(", hint = ") << hint << QStringLiteral(", en-crypt-id = ") << id);

    m_currentContextMenuExtraData.m_encryptedText = encryptedText;
    m_currentContextMenuExtraData.m_keyLength = keyLength;
    m_currentContextMenuExtraData.m_cipher = cipher;
    m_currentContextMenuExtraData.m_hint = hint;
    m_currentContextMenuExtraData.m_id = id;

    delete m_pEncryptedTextContextMenu;
    m_pEncryptedTextContextMenu = new QMenu(this);

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Decrypt, tr("Decrypt..."), m_pEncryptedTextContextMenu,
                             decryptEncryptedTextUnderCursor, m_isPageEditable);

    m_pEncryptedTextContextMenu->exec(m_lastContextMenuEventGlobalPos);
}

void NoteEditorPrivate::setupActionShortcut(const int key, const QString & context,
                                            QAction & action)
{
    ShortcutManager shortcutManager;
    QKeySequence shortcut = shortcutManager.shortcut(key, context);
    if (!shortcut.isEmpty()) {
        QNTRACE(QStringLiteral("Setting shortcut ") << shortcut << QStringLiteral(" for action ") << action.objectName());
        action.setShortcut(shortcut);
    }
}

void NoteEditorPrivate::setupFileIO()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupFileIO"));

    QUENTIER_CHECK_PTR(m_pFileIOThreadWorker, QStringLiteral("no file IO thread worker was passed to note editor"));

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,writeNoteHtmlToFile,QString,QByteArray,QUuid,bool),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,saveResourceToFile,QString,QByteArray,QUuid,bool),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));
    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,ErrorString,QUuid),
                     this, QNSLOT(NoteEditorPrivate,onWriteFileRequestProcessed,bool,ErrorString,QUuid));

    if (m_pResourceFileStorageManager) {
        m_pResourceFileStorageManager->deleteLater();
        m_pResourceFileStorageManager = Q_NULLPTR;
    }

    m_pResourceFileStorageManager = new ResourceFileStorageManager(m_resourceLocalFileStorageFolder, m_noteEditorImageResourcesStoragePath);
    m_pResourceFileStorageManager->moveToThread(m_pFileIOThreadWorker->thread());

    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,currentNoteChanged,Note),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onCurrentNoteChanged,Note));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,readResourceFromStorage,QString,QString,QUuid),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onReadResourceFromFileRequest,QString,QString,QUuid));
    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,readResourceFromFileCompleted,QUuid,QByteArray,QByteArray,int,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onResourceFileReadFromStorage,QUuid,QByteArray,QByteArray,int,ErrorString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,openResourceFile,QString),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onOpenResourceRequest,QString));
    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,resourceFileChanged,QString,QString),
                     this, QNSLOT(NoteEditorPrivate,onResourceFileChanged,QString,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,saveResourceToStorage,QString,QString,QByteArray,QByteArray,QString,QUuid,bool),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onWriteResourceToFileRequest,QString,QString,QByteArray,QByteArray,QString,QUuid,bool));
    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onResourceSavedToStorage,QUuid,QByteArray,QString,int,ErrorString));

    if (m_pGenericResourceImageManager) {
        m_pGenericResourceImageManager->deleteLater();
        m_pGenericResourceImageManager = Q_NULLPTR;
    }

    m_pGenericResourceImageManager = new GenericResourceImageManager;
    m_pGenericResourceImageManager->setStorageFolderPath(m_genericResourceImageFileStoragePath);
    m_pGenericResourceImageManager->moveToThread(m_pFileIOThreadWorker->thread());

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    QObject::connect(this,
                     QNSIGNAL(NoteEditorPrivate,saveGenericResourceImageToFile,QString,QString,QByteArray,QString,QByteArray,QString,QUuid),
                     m_pGenericResourceImageManager,
                     QNSLOT(GenericResourceImageManager,onGenericResourceImageWriteRequest,QString,QString,QByteArray,QString,QByteArray,QString,QUuid));
    QObject::connect(m_pGenericResourceImageManager,
                     QNSIGNAL(GenericResourceImageManager,genericResourceImageWriteReply,bool,QByteArray,QString,ErrorString,QUuid),
                     this, QNSLOT(NoteEditorPrivate,onGenericResourceImageSaved,bool,QByteArray,QString,ErrorString,QUuid));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,currentNoteChanged,Note), m_pGenericResourceImageManager,
                     QNSLOT(GenericResourceImageManager,onCurrentNoteChanged,Note));
#endif
}

void NoteEditorPrivate::setupSpellChecker()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupSpellChecker"));

    QUENTIER_CHECK_PTR(m_pSpellChecker, QStringLiteral("np spell checker was passed to note editor"));

    if (!m_pSpellChecker->isReady()) {
        QObject::connect(m_pSpellChecker, QNSIGNAL(SpellChecker,ready), this, QNSLOT(NoteEditorPrivate,onSpellCheckerReady));
    }
    else {
        onSpellCheckerReady();
    }
}

void NoteEditorPrivate::setupScripts()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupScripts"));

    initNoteEditorResources();

    QFile file;

#define SETUP_SCRIPT(scriptPathPart, scriptVarName) \
    file.setFileName(QStringLiteral( ":/" scriptPathPart)); \
    file.open(QIODevice::ReadOnly); \
    scriptVarName = file.readAll(); \
    file.close()

    SETUP_SCRIPT("javascript/jquery/jquery-2.1.3.min.js", m_jQueryJs);
    SETUP_SCRIPT("javascript/jquery/jquery-ui.min.js", m_jQueryUiJs);
    SETUP_SCRIPT("javascript/scripts/pageMutationObserver.js", m_pageMutationObserverJs);
    SETUP_SCRIPT("javascript/colResizable/colResizable-1.5.min.js", m_resizableTableColumnsJs);
    SETUP_SCRIPT("javascript/scripts/resizableImageManager.js", m_resizableImageManagerJs);
    SETUP_SCRIPT("javascript/debounce/jquery.debounce-1.0.5.js", m_debounceJs);
    SETUP_SCRIPT("javascript/rangy/rangy-core.js", m_rangyCoreJs);
    SETUP_SCRIPT("javascript/rangy/rangy-selectionsaverestore.js", m_rangySelectionSaveRestoreJs);
    SETUP_SCRIPT("javascript/hilitor/hilitor-utf8.js", m_hilitorJs);
    SETUP_SCRIPT("javascript/scripts/imageAreasHilitor.js", m_imageAreasHilitorJs);
    SETUP_SCRIPT("javascript/scripts/onTableResize.js", m_onTableResizeJs);
    SETUP_SCRIPT("javascript/scripts/selectionManager.js", m_selectionManagerJs);
    SETUP_SCRIPT("javascript/scripts/textEditingUndoRedoManager.js", m_textEditingUndoRedoManagerJs);
    SETUP_SCRIPT("javascript/scripts/getSelectionHtml.js", m_getSelectionHtmlJs);
    SETUP_SCRIPT("javascript/scripts/snapSelectionToWord.js", m_snapSelectionToWordJs);
    SETUP_SCRIPT("javascript/scripts/replaceSelectionWithHtml.js", m_replaceSelectionWithHtmlJs);
    SETUP_SCRIPT("javascript/scripts/findReplaceManager.js", m_findReplaceManagerJs);
    SETUP_SCRIPT("javascript/scripts/spellChecker.js", m_spellCheckerJs);
    SETUP_SCRIPT("javascript/scripts/managedPageAction.js", m_managedPageActionJs);
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

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    SETUP_SCRIPT("javascript/scripts/qWebKitSetup.js", m_qWebKitSetupJs);
#else
    SETUP_SCRIPT("qtwebchannel/qwebchannel.js", m_qWebChannelJs);
    SETUP_SCRIPT("javascript/scripts/qWebChannelSetup.js", m_qWebChannelSetupJs);
#endif

    SETUP_SCRIPT("javascript/scripts/enToDoTagsSetup.js", m_setupEnToDoTagsJs);
    SETUP_SCRIPT("javascript/scripts/flipEnToDoCheckboxState.js", m_flipEnToDoCheckboxStateJs);
#ifdef QUENTIER_USE_QT_WEB_ENGINE
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupGeneralSignalSlotConnections"));

    QObject::connect(m_pTableResizeJavaScriptHandler, QNSIGNAL(TableResizeJavaScriptHandler,tableResized),
                     this, QNSLOT(NoteEditorPrivate,onTableResized));
    QObject::connect(m_pResizableImageJavaScriptHandler, QNSIGNAL(ResizableImageJavaScriptHandler,imageResourceResized,bool),
                     this, QNSLOT(NoteEditorPrivate,onImageResourceResized,bool));
    QObject::connect(m_pSpellCheckerDynamicHandler, QNSIGNAL(SpellCheckerDynamicHelper,lastEnteredWords,QStringList),
                     this, QNSLOT(NoteEditorPrivate,onSpellCheckerDynamicHelperUpdate,QStringList));
    QObject::connect(m_pToDoCheckboxClickHandler, QNSIGNAL(ToDoCheckboxOnClickHandler,toDoCheckboxClicked,quint64),
                     this, QNSLOT(NoteEditorPrivate,onToDoCheckboxClicked,quint64));
    QObject::connect(m_pToDoCheckboxClickHandler, QNSIGNAL(ToDoCheckboxOnClickHandler,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onToDoCheckboxClickHandlerError,ErrorString));
    QObject::connect(m_pPageMutationHandler, QNSIGNAL(PageMutationHandler,contentsChanged),
                     this, QNSIGNAL(NoteEditorPrivate,contentChanged));
    QObject::connect(m_pPageMutationHandler, QNSIGNAL(PageMutationHandler,contentsChanged),
                     this, QNSIGNAL(NoteEditorPrivate,noteModified));
    QObject::connect(m_pPageMutationHandler, QNSIGNAL(PageMutationHandler,contentsChanged),
                     this, QNSLOT(NoteEditorPrivate,onContentChanged));
    QObject::connect(m_pContextMenuEventJavaScriptHandler, QNSIGNAL(ContextMenuEventJavaScriptHandler,contextMenuEventReply,QString,QString,bool,QStringList,quint64),
                     this, QNSLOT(NoteEditorPrivate,onContextMenuEventReply,QString,QString,bool,QStringList,quint64));

    Q_Q(NoteEditor);
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,notifyError,ErrorString),
                     q, QNSIGNAL(NoteEditor,notifyError,ErrorString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,convertedToNote,Note),
                     q, QNSIGNAL(NoteEditor,convertedToNote,Note));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,cantConvertToNote,ErrorString),
                     q, QNSIGNAL(NoteEditor,cantConvertToNote,ErrorString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,noteEditorHtmlUpdated,QString),
                     q, QNSIGNAL(NoteEditor,noteEditorHtmlUpdated,QString));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,currentNoteChanged,Note),
                     q, QNSIGNAL(NoteEditor,currentNoteChanged,Note));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,contentChanged),
                     q, QNSIGNAL(NoteEditor,contentChanged));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,noteModified),
                     q, QNSIGNAL(NoteEditor,noteModified));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,spellCheckerNotReady),
                     q, QNSIGNAL(NoteEditor,spellCheckerNotReady));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,spellCheckerReady),
                     q, QNSIGNAL(NoteEditor,spellCheckerReady));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,noteLoaded),
                     q, QNSIGNAL(NoteEditor,noteLoaded));
    QObject::connect(this, QNSIGNAL(NoteEditorPrivate,insertTableDialogRequested),
                     q, QNSIGNAL(NoteEditor,insertTableDialogRequested));
}

void NoteEditorPrivate::setupNoteEditorPage()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupNoteEditorPage"));

    NoteEditorPage * page = new NoteEditorPage(*this);

    page->settings()->setAttribute(WebSettings::LocalContentCanAccessFileUrls, true);
    page->settings()->setAttribute(WebSettings::LocalContentCanAccessRemoteUrls, false);

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    page->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    page->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    page->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    page->setContentEditable(true);

    page->mainFrame()->addToJavaScriptWindowObject(QStringLiteral("pageMutationObserver"), m_pPageMutationHandler,
                                                   OwnershipNamespace::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject(QStringLiteral("resourceCache"), m_pResourceInfoJavaScriptHandler,
                                                   OwnershipNamespace::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject(QStringLiteral("textCursorPositionHandler"), m_pTextCursorPositionJavaScriptHandler,
                                                   OwnershipNamespace::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject(QStringLiteral("contextMenuEventHandler"), m_pContextMenuEventJavaScriptHandler,
                                                   OwnershipNamespace::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject(QStringLiteral("toDoCheckboxClickHandler"), m_pToDoCheckboxClickHandler,
                                                   OwnershipNamespace::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject(QStringLiteral("tableResizeHandler"), m_pTableResizeJavaScriptHandler,
                                                   OwnershipNamespace::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject(QStringLiteral("resizableImageHandler"), m_pResizableImageJavaScriptHandler,
                                                   OwnershipNamespace::QtOwnership);
    page->mainFrame()->addToJavaScriptWindowObject(QStringLiteral("spellCheckerDynamicHelper"), m_pSpellCheckerDynamicHandler,
                                                   OwnershipNamespace::QtOwnership);

    m_pPluginFactory = new NoteEditorPluginFactory(*this, *m_pResourceFileStorageManager, *m_pFileIOThreadWorker, page);
    if (Q_LIKELY(!m_pNote.isNull())) {
        m_pPluginFactory->setNote(*m_pNote);
    }

    page->setPluginFactory(m_pPluginFactory);
#endif

    setupNoteEditorPageConnections(page);
    setPage(page);

    QNTRACE(QStringLiteral("Done setting up new note editor page"));
}

void NoteEditorPrivate::setupNoteEditorPageConnections(NoteEditorPage * page)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupNoteEditorPageConnections"));

    QObject::connect(page, QNSIGNAL(NoteEditorPage,javaScriptLoaded), this, QNSLOT(NoteEditorPrivate,onJavaScriptLoaded));
#ifndef QUENTIER_USE_QT_WEB_ENGINE
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupTextCursorPositionJavaScriptHandlerConnections"));

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

    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionOnImageResourceState,bool,QByteArray),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorOnImageResourceStateChanged,bool,QByteArray));
    QObject::connect(m_pTextCursorPositionJavaScriptHandler, QNSIGNAL(TextCursorPositionJavaScriptHandler,textCursorPositionOnNonImageResourceState,bool,QByteArray),
                     this, QNSLOT(NoteEditorPrivate,onTextCursorOnNonImageResourceStateChanged,bool,QByteArray));
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupSkipRulesForHtmlToEnmlConversion"));

    m_skipRulesForHtmlToEnmlConversion.reserve(6);

    ENMLConverter::SkipHtmlElementRule tableSkipRule;
    tableSkipRule.m_attributeValueToSkip = QStringLiteral("JCLRgrip");
    tableSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::StartsWith;
    tableSkipRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;
    m_skipRulesForHtmlToEnmlConversion << tableSkipRule;

    ENMLConverter::SkipHtmlElementRule hilitorSkipRule;
    hilitorSkipRule.m_includeElementContents = true;
    hilitorSkipRule.m_attributeValueToSkip = QStringLiteral("hilitorHelper");
    hilitorSkipRule.m_attributeValueCaseSensitivity = Qt::CaseInsensitive;
    hilitorSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::Contains;
    m_skipRulesForHtmlToEnmlConversion << hilitorSkipRule;

    ENMLConverter::SkipHtmlElementRule imageAreaHilitorSkipRule;
    imageAreaHilitorSkipRule.m_includeElementContents = false;
    imageAreaHilitorSkipRule.m_attributeValueToSkip = QStringLiteral("image-area-hilitor");
    imageAreaHilitorSkipRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;
    imageAreaHilitorSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::Contains;
    m_skipRulesForHtmlToEnmlConversion << imageAreaHilitorSkipRule;

    ENMLConverter::SkipHtmlElementRule spellCheckerHelperSkipRule;
    spellCheckerHelperSkipRule.m_includeElementContents = true;
    spellCheckerHelperSkipRule.m_attributeValueToSkip = QStringLiteral("misspell");
    spellCheckerHelperSkipRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;
    spellCheckerHelperSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::Contains;
    m_skipRulesForHtmlToEnmlConversion << spellCheckerHelperSkipRule;

    ENMLConverter::SkipHtmlElementRule rangySelectionBoundaryRule;
    rangySelectionBoundaryRule.m_includeElementContents = false;
    rangySelectionBoundaryRule.m_attributeValueToSkip = QStringLiteral("rangySelectionBoundary");
    rangySelectionBoundaryRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;
    rangySelectionBoundaryRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::Contains;
    m_skipRulesForHtmlToEnmlConversion << rangySelectionBoundaryRule;

    ENMLConverter::SkipHtmlElementRule resizableImageHandleRule;
    resizableImageHandleRule.m_includeElementContents = false;
    resizableImageHandleRule.m_attributeValueToSkip = QStringLiteral("ui-resizable-handle");
    resizableImageHandleRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;
    resizableImageHandleRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::Contains;
    m_skipRulesForHtmlToEnmlConversion << resizableImageHandleRule;
}

QString NoteEditorPrivate::initialPageHtml() const
{
    if (!m_blankPageHtml.isEmpty()) {
        return m_blankPageHtml;
    }

    return m_pagePrefix + QStringLiteral("<body></body></html>");
}

void NoteEditorPrivate::determineStatesForCurrentTextCursorPosition()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::determineStatesForCurrentTextCursorPosition"));

    QString javascript = QStringLiteral("if (typeof window[\"determineStatesForCurrentTextCursorPosition\"] !== 'undefined')"
                                        "{ determineStatesForCurrentTextCursorPosition(); }");

    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::determineContextMenuEventTarget()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::determineContextMenuEventTarget"));

    QString javascript = QStringLiteral("determineContextMenuEventTarget(") + QString::number(m_contextMenuSequenceNumber) +
                         QStringLiteral(", ") + QString::number(m_lastContextMenuEventPagePos.x()) + QStringLiteral(", ") +
                         QString::number(m_lastContextMenuEventPagePos.y()) + QStringLiteral(");");

    GET_PAGE()
    page->executeJavaScript(javascript);
}

void NoteEditorPrivate::setPageEditable(const bool editable)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setPageEditable: ") << (editable ? QStringLiteral("true") : QStringLiteral("false")));

    GET_PAGE()

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    page->setContentEditable(editable);
#else
    QString javascript = QStringLiteral("document.body.contentEditable='") + (editable ? QStringLiteral("true") : QStringLiteral("false")) +
                         QStringLiteral("'; document.designMode='") + (editable ? QStringLiteral("on") : QStringLiteral("off")) +
                         QStringLiteral("'; void 0;");
    page->executeJavaScript(javascript);
    QNTRACE(QStringLiteral("Queued javascript to make page ") << (editable ? QStringLiteral("editable") : QStringLiteral("non-editable"))
            << QStringLiteral(": ") << javascript);
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onPageHtmlReceived"));
    Q_UNUSED(extraData)

    emit noteEditorHtmlUpdated(html);

    if (!m_pendingConversionToNote) {
        return;
    }

    if (m_pNote.isNull()) {
        m_pendingConversionToNote = false;
        ErrorString error(QT_TRANSLATE_NOOP("", "no current note is set to note editor"));
        emit cantConvertToNote(error);
        return;
    }

    if (m_pNote->isInkNote()) {
        m_pendingConversionToNote = false;
        QNINFO(QStringLiteral("Currently selected note is an ink note, it's not editable hence won't respond to the unexpected change of its HTML"));
        return;
    }

    m_lastSelectedHtml.resize(0);
    m_htmlCachedMemory = html;
    m_enmlCachedMemory.resize(0);
    ErrorString error;
    bool res = m_enmlConverter.htmlToNoteContent(m_htmlCachedMemory, m_enmlCachedMemory,
                                                 *m_decryptedTextManager, error,
                                                 m_skipRulesForHtmlToEnmlConversion);
    if (!res)
    {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't convert note editor page's content to ENML"));
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        emit notifyError(errorDescription);

        m_pendingConversionToNote = false;
        emit cantConvertToNote(errorDescription);

        return;
    }

    if (!checkNoteSize(m_enmlCachedMemory)) {
        return;
    }

    m_pNote->setContent(m_enmlCachedMemory);
    m_pendingConversionToNote = false;

    m_modified = false;

    emit convertedToNote(*m_pNote);
}

void NoteEditorPrivate::onSelectedTextEncryptionDone(const QVariant & dummy, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onSelectedTextEncryptionDone"));

    Q_UNUSED(dummy);
    Q_UNUSED(extraData);

    m_pendingConversionToNote = true;

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    m_htmlCachedMemory = page()->mainFrame()->toHtml();
    onPageHtmlReceived(m_htmlCachedMemory);
#else
    page()->toHtml(NoteEditorCallbackFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
    provideSrcAndOnClickScriptForImgEnCryptTags();
#endif
}

void NoteEditorPrivate::onTableActionDone(const QVariant & dummy, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTableActionDone"));

    Q_UNUSED(dummy);
    Q_UNUSED(extraData);

    setModified();
    convertToNote();
}

int NoteEditorPrivate::resourceIndexByHash(const QList<Resource> & resources,
                                           const QByteArray & resourceHash) const
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::resourceIndexByHash: hash = ") << resourceHash.toHex());

    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const Resource & resource = resources[i];
        if (resource.hasDataHash() && (resource.dataHash() == resourceHash)) {
            return i;
        }
    }

    return -1;
}

void NoteEditorPrivate::writeNotePageFile(const QString & html)
{
    m_writeNoteHtmlToFileRequestId = QUuid::createUuid();
    m_pendingIndexHtmlWritingToFile = true;
    QString pagePath = noteEditorPagePath();
    QNTRACE(QStringLiteral("Emitting the request to write the note html to file: request id = ") << m_writeNoteHtmlToFileRequestId);
    emit writeNoteHtmlToFile(pagePath, html.toLocal8Bit(),
                             m_writeNoteHtmlToFileRequestId, /* append = */ false);
}

bool NoteEditorPrivate::parseEncryptedTextContextMenuExtraData(const QStringList & extraData, QString & encryptedText,
                                                               QString & decryptedText, QString & cipher, QString & keyLength,
                                                               QString & hint, QString & id, ErrorString & errorDescription) const
{
    if (Q_UNLIKELY(extraData.empty())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "extra data from JavaScript is empty");
        return false;
    }

    int extraDataSize = extraData.size();
    if (Q_UNLIKELY(extraDataSize != 5) && Q_UNLIKELY(extraDataSize != 6)) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "extra data from JavaScript has wrong size");
        errorDescription.details() = QString::number(extraDataSize);
        return false;
    }

    cipher = extraData[0];
    keyLength = extraData[1];
    encryptedText = extraData[2];
    hint = extraData[3];
    id = extraData[4];

    if (extraDataSize == 6) {
        decryptedText = extraData[5];
    }
    else {
        decryptedText.clear();
    }

    return true;
}

void NoteEditorPrivate::setupPasteGenericTextMenuActions()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupPasteGenericTextMenuActions"));

    if (Q_UNLIKELY(!m_pGenericTextContextMenu)) {
        QNDEBUG("No generic text context menu, nothing to do");
        return;
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
        QNTRACE(QStringLiteral("Clipboard buffer has something, adding paste action"));
        ADD_ACTION_WITH_SHORTCUT(QKeySequence::Paste, tr("Paste"), m_pGenericTextContextMenu, paste, m_isPageEditable);
    }

    if (clipboardHasHtml) {
        QNTRACE(QStringLiteral("Clipboard buffer has html, adding paste unformatted action"));
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::PasteUnformatted, tr("Paste as unformatted text"),
                                 m_pGenericTextContextMenu, pasteUnformatted, m_isPageEditable);
    }

    Q_UNUSED(m_pGenericTextContextMenu->addSeparator());
}

void NoteEditorPrivate::setupParagraphSubMenuForGenericTextMenu(const QString & selectedHtml)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupParagraphSubMenuForGenericTextMenu: selected html = ") << selectedHtml);

    if (Q_UNLIKELY(!m_pGenericTextContextMenu)) {
        QNDEBUG(QStringLiteral("No generic text context menu, nothing to do"));
        return;
    }

    if (!isPageEditable()) {
        QNDEBUG(QStringLiteral("Note is not editable, no paragraph sub-menu actions are allowed"));
        return;
    }

    QMenu * pParagraphSubMenu = m_pGenericTextContextMenu->addMenu(tr("Paragraph"));
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::AlignLeft, tr("Align left"), pParagraphSubMenu, alignLeft, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::AlignCenter, tr("Center text"), pParagraphSubMenu, alignCenter, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::AlignRight, tr("Align right"), pParagraphSubMenu, alignRight, m_isPageEditable);
    Q_UNUSED(pParagraphSubMenu->addSeparator());
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::IncreaseIndentation, tr("Increase indentation"),
                             pParagraphSubMenu, increaseIndentation, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::DecreaseIndentation, tr("Decrease indentation"),
                             pParagraphSubMenu, decreaseIndentation, m_isPageEditable);
    Q_UNUSED(pParagraphSubMenu->addSeparator());

    if (!selectedHtml.isEmpty()) {
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::IncreaseFontSize, tr("Increase font size"),
                                 pParagraphSubMenu, increaseFontSize, m_isPageEditable);
        ADD_ACTION_WITH_SHORTCUT(ShortcutManager::DecreaseFontSize, tr("Decrease font size"),
                                 pParagraphSubMenu, decreaseFontSize, m_isPageEditable);
        Q_UNUSED(pParagraphSubMenu->addSeparator());
    }

    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertNumberedList, tr("Numbered list"),
                             pParagraphSubMenu, insertNumberedList, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::InsertBulletedList, tr("Bulleted list"),
                             pParagraphSubMenu, insertBulletedList, m_isPageEditable);
}

void NoteEditorPrivate::setupStyleSubMenuForGenericTextMenu()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setupStyleSubMenuForGenericTextMenu"));

    if (Q_UNLIKELY(!m_pGenericTextContextMenu)) {
        QNDEBUG(QStringLiteral("No generic text context menu, nothing to do"));
        return;
    }

    if (!isPageEditable()) {
        QNDEBUG(QStringLiteral("Note is not editable, no style sub-menu actions are allowed"));
        return;
    }

    QMenu * pStyleSubMenu = m_pGenericTextContextMenu->addMenu(tr("Style"));
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Bold, tr("Bold"), pStyleSubMenu, textBold, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Italic, tr("Italic"), pStyleSubMenu, textItalic, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(QKeySequence::Underline, tr("Underline"), pStyleSubMenu, textUnderline, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Strikethrough, tr("Strikethrough"), pStyleSubMenu, textStrikethrough, m_isPageEditable);
    ADD_ACTION_WITH_SHORTCUT(ShortcutManager::Highlight, tr("Highlight"), pStyleSubMenu, textHighlight, m_isPageEditable);
}

void NoteEditorPrivate::rebuildRecognitionIndicesCache()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::rebuildRecognitionIndicesCache"));

    m_recognitionIndicesByResourceHash.clear();

    if (Q_UNLIKELY(m_pNote.isNull())) {
        QNTRACE(QStringLiteral("No note is set"));
        return;
    }

    if (!m_pNote->hasResources()) {
        QNTRACE(QStringLiteral("The note has no resources"));
        return;
    }

    QList<Resource> resources = m_pNote->resources();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const Resource & resource = resources[i];
        if (Q_UNLIKELY(!resource.hasDataHash())) {
            QNDEBUG(QStringLiteral("Skipping the resource without the data hash: ") << resource);
            continue;
        }

        if (!resource.hasRecognitionDataBody()) {
            QNTRACE(QStringLiteral("Skipping the resource without recognition data body"));
            continue;
        }

        ResourceRecognitionIndices recoIndices(resource.recognitionDataBody());
        if (recoIndices.isNull() || !recoIndices.isValid()) {
            QNTRACE(QStringLiteral("Skipping null/invalid resource recognition indices"));
            continue;
        }

        m_recognitionIndicesByResourceHash[resource.dataHash()] = recoIndices;
    }
}

void NoteEditorPrivate::enableSpellCheck()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::enableSpellCheck"));

    if (!m_pSpellChecker->isReady()) {
        QNTRACE(QStringLiteral("Spell checker is not ready"));
        emit spellCheckerNotReady();
        return;
    }

    refreshMisSpelledWordsList();
    applySpellCheck();
    enableDynamicSpellCheck();
}

void NoteEditorPrivate::disableSpellCheck()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::disableSpellCheck"));

    m_currentNoteMisSpelledWords.clear();
    removeSpellCheck();
    disableDynamicSpellCheck();
}

void NoteEditorPrivate::refreshMisSpelledWordsList()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::refreshMisSpelledWordsList"));

    if (m_pNote.isNull()) {
        QNDEBUG(QStringLiteral("No note is set to the editor"));
        return;
    }

    m_currentNoteMisSpelledWords.clear();

    ErrorString error;
    QStringList words = m_pNote->listOfWords(&error);
    if (words.isEmpty() && !error.isEmpty()) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "can't get the list of words from the note"));
        errorDescription.additionalBases().append(error.base());
        errorDescription.additionalBases().append(error.additionalBases());
        errorDescription.details() = error.details();
        QNWARNING(errorDescription);
        emit notifyError(errorDescription);
        return;
    }

    for(auto it = words.constBegin(), end = words.constEnd(); it != end; ++it)
    {
        const QString & originalWord = *it;
        QNTRACE(QStringLiteral("Checking the word ") << originalWord);

        QString word = originalWord;

        bool conversionResult = false;
        qint32 integerNumber = word.toInt(&conversionResult);
        if (conversionResult) {
            QNTRACE(QStringLiteral("Skipping the integer number ") << word);
            continue;
        }

        qint64 longIntegerNumber = word.toLongLong(&conversionResult);
        if (conversionResult) {
            QNTRACE(QStringLiteral("Skipping the long long integer number ") << word);
            continue;
        }

        Q_UNUSED(integerNumber)
        Q_UNUSED(longIntegerNumber)

        m_stringUtils.removePunctuation(word);
        if (word.isEmpty()) {
            QNTRACE(QStringLiteral("Skipping the word which becomes empty after stripping off the punctuation: ") << originalWord);
            continue;
        }

        word = word.trimmed();

        QNTRACE(QStringLiteral("Checking the spelling of the \"adjusted\" word ") << word);

        if (!m_pSpellChecker->checkSpell(word)) {
            QNTRACE(QStringLiteral("Misspelled word: \"") << word << QStringLiteral("\""));
            word = originalWord;
            m_stringUtils.removePunctuation(word);
            word = word.trimmed();
            m_currentNoteMisSpelledWords << word;
            QNTRACE(QStringLiteral("Word added to the list: ") << word);
        }
    }
}

void NoteEditorPrivate::applySpellCheck(const bool applyToSelection)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::applySpellCheck: apply to selection = ")
            << (applyToSelection ? QStringLiteral("true") : QStringLiteral("false")));

    QString javascript = QStringLiteral("if (window.hasOwnProperty('spellChecker')) { spellChecker.apply");

    if (applyToSelection) {
        javascript += QStringLiteral("ToSelection");
    }

    javascript += QStringLiteral("('");
    for(auto it = m_currentNoteMisSpelledWords.constBegin(), end = m_currentNoteMisSpelledWords.constEnd(); it != end; ++it) {
        javascript += *it;
        javascript += QStringLiteral("', '");
    }
    javascript.chop(3);     // Remove trailing ", '";
    javascript += QStringLiteral("); }");

    QNTRACE(QStringLiteral("Script: ") << javascript);

    GET_PAGE()
    page->executeJavaScript(javascript, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onSpellCheckSetOrCleared));
}

void NoteEditorPrivate::removeSpellCheck()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::removeSpellCheck"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("if (window.hasOwnProperty('spellChecker')) { spellChecker.remove(); }"),
                            NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onSpellCheckSetOrCleared));
}

void NoteEditorPrivate::enableDynamicSpellCheck()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::enableDynamicSpellCheck"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("if (window.hasOwnProperty('spellChecker')) { spellChecker.enableDynamic(); }"));
}

void NoteEditorPrivate::disableDynamicSpellCheck()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::disableDynamicSpellCheck"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("if (window.hasOwnProperty('spellChecker')) { spellChecker.disableDynamic(); }"));
}

void NoteEditorPrivate::onSpellCheckSetOrCleared(const QVariant & dummy, const QVector<QPair<QString, QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onSpellCheckSetOrCleared"));

    Q_UNUSED(dummy)
    Q_UNUSED(extraData)

    GET_PAGE()
#ifndef QUENTIER_USE_QT_WEB_ENGINE
    m_htmlCachedMemory = page->mainFrame()->toHtml();
    onPageHtmlReceived(m_htmlCachedMemory);
#else
    page->toHtml(NoteEditorCallbackFunctor<QString>(this, &NoteEditorPrivate::onPageHtmlReceived));
#endif
}

bool NoteEditorPrivate::isNoteReadOnly() const
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::isNoteReadOnly"));

    if (m_pNote.isNull()) {
        QNTRACE(QStringLiteral("No note is set to the editor"));
        return true;
    }

    if (m_pNote->hasNoteRestrictions())
    {
        const qevercloud::NoteRestrictions & noteRestrictions = m_pNote->noteRestrictions();
        if (noteRestrictions.noUpdateContent.isSet() && noteRestrictions.noUpdateContent.ref()) {
            QNTRACE(QStringLiteral("Note has noUpdateContent restriction set to true"));
            return true;
        }
    }

    if (m_pNotebook.isNull()) {
        QNTRACE(QStringLiteral("No notebook is set to the editor"));
        return true;
    }

    if (!m_pNotebook->hasRestrictions()) {
        QNTRACE(QStringLiteral("Notebook has no restrictions"));
        return false;
    }

    const qevercloud::NotebookRestrictions & restrictions = m_pNotebook->restrictions();
    if (restrictions.noUpdateNotes.isSet() && restrictions.noUpdateNotes.ref()) {
        QNTRACE(QStringLiteral("Restriction on note updating applies"));
        return true;
    }

    return false;
}

void NoteEditorPrivate::setupAddHyperlinkDelegate(const quint64 hyperlinkId, const QString & presetHyperlink)
{
    AddHyperlinkToSelectedTextDelegate * delegate = new AddHyperlinkToSelectedTextDelegate(*this, hyperlinkId);

    QObject::connect(delegate, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,finished),
                     this, QNSLOT(NoteEditorPrivate,onAddHyperlinkToSelectedTextDelegateFinished));
    QObject::connect(delegate, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,cancelled),
                     this, QNSLOT(NoteEditorPrivate,onAddHyperlinkToSelectedTextDelegateCancelled));
    QObject::connect(delegate, QNSIGNAL(AddHyperlinkToSelectedTextDelegate,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onAddHyperlinkToSelectedTextDelegateError,ErrorString));

    if (presetHyperlink.isEmpty()) {
        delegate->start();
    }
    else {
        delegate->startWithPresetHyperlink(presetHyperlink);
    }
}

#define COMMAND_TO_JS(command) \
    QString javascript = QString("managedPageAction(\"%1\", null)").arg(command)

#define COMMAND_WITH_ARGS_TO_JS(command, args) \
    QString javascript = QString("managedPageAction('%1', '%2')").arg(command,args)

#ifndef QUENTIER_USE_QT_WEB_ENGINE
QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command)
{
    COMMAND_TO_JS(command);
    QWebFrame * frame = page()->mainFrame();
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE(QStringLiteral("Executed javascript command: ") << javascript << QStringLiteral(", result = ") << result.toString());
    return result;
}

QVariant NoteEditorPrivate::execJavascriptCommandWithResult(const QString & command, const QString & args)
{
    COMMAND_WITH_ARGS_TO_JS(command, args);
    QWebFrame * frame = page()->mainFrame();
    QVariant result = frame->evaluateJavaScript(javascript);
    QNTRACE(QStringLiteral("Executed javascript command: ") << javascript << QStringLiteral(", result = ") << result.toString());
    return result;
}
#endif

void NoteEditorPrivate::execJavascriptCommand(const QString & command)
{
    COMMAND_TO_JS(command);

    GET_PAGE()
    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onManagedPageActionFinished);
    page->executeJavaScript(javascript, callback);
}

void NoteEditorPrivate::execJavascriptCommand(const QString & command, const QString & args)
{
    COMMAND_WITH_ARGS_TO_JS(command, args);

    GET_PAGE()
    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onManagedPageActionFinished);
    page->executeJavaScript(javascript, callback);
}

void NoteEditorPrivate::initialize(FileIOThreadWorker & fileIOThreadWorker, SpellChecker & spellChecker)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::initialize"));

    m_pFileIOThreadWorker = &fileIOThreadWorker;
    m_pSpellChecker = &spellChecker;
}

void NoteEditorPrivate::setAccount(const Account & account)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setAccount: ") << account);

    if (!m_pAccount.isNull() && (m_pAccount->type() == account.type()) &&
        (m_pAccount->name() == account.name()) && (m_pAccount->id() == account.id()))
    {
        QNDEBUG(QStringLiteral("The account's type, name and id were not updated so it's the update "
                               "for the account currently set to the note editor"));
        *m_pAccount = account;
        return;
    }

    clear();

    if (m_pAccount.isNull()) {
        m_pAccount.reset(new Account(account));
    }
    else {
        *m_pAccount = account;
    }

    init();
}

void NoteEditorPrivate::setUndoStack(QUndoStack * pUndoStack)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setUndoStack"));

    QUENTIER_CHECK_PTR(pUndoStack, QStringLiteral("null undo stack passed to note editor"));
    m_pUndoStack = pUndoStack;
}

void NoteEditorPrivate::setNoteAndNotebook(const Note & note, const Notebook & notebook)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setNoteAndNotebook: note: local uid = ") << note.localUid()
            << QStringLiteral(", guid = ") << (note.hasGuid() ? note.guid() : QStringLiteral("<null>"))
            << QStringLiteral(", title: ") << (note.hasTitle() ? note.title() : QStringLiteral("<null>"))
            << QStringLiteral("; notebook: local uid = ") << notebook.localUid() << QStringLiteral(", guid = ")
            << (notebook.hasGuid() ? notebook.guid() : QStringLiteral("<null>"))
            << QStringLiteral(", name = ") << (notebook.hasName() ? notebook.name() : QStringLiteral("<null>")));

    if (m_pNotebook.isNull()) {
        m_pNotebook.reset(new Notebook(notebook));
    }
    else {
        *m_pNotebook = notebook;
    }

    if (m_pNote.isNull())
    {
        m_pNote.reset(new Note(note));
    }
    else
    {
        bool sameUpdatedNote = (m_pNote->localUid() == note.localUid());

        if (sameUpdatedNote)
        {
            bool noteChanged = false;
            noteChanged = noteChanged || (m_pNote->hasContent() != note.hasContent());
            noteChanged = noteChanged || (m_pNote->hasResources() != note.hasResources());

            if (!noteChanged && m_pNote->hasResources() && note.hasResources())
            {
                QList<Resource> currentResources = m_pNote->resources();
                QList<Resource> updatedResources = note.resources();

                int size = currentResources.size();
                noteChanged = (size != updatedResources.size());
                if (!noteChanged)
                {
                    // NOTE: clearing out data bodies before comparing resources to speed up the comparison
                    for(int i = 0; i < size; ++i)
                    {
                        Resource currentResource = currentResources[i];
                        currentResource.setDataBody(QByteArray());
                        currentResource.setAlternateDataBody(QByteArray());

                        Resource updatedResource = updatedResources[i];
                        updatedResource.setDataBody(QByteArray());
                        updatedResource.setAlternateDataBody(QByteArray());

                        if (currentResource != updatedResource) {
                            noteChanged = true;
                            break;
                        }
                    }
                }
            }

            if (!noteChanged && m_pNote->hasContent() && note.hasContent()) {
                noteChanged = (m_pNote->content() != note.content());
            }

            if (!noteChanged) {
                QNDEBUG(QStringLiteral("Haven't found the updates within the note which would be sufficient enough "
                                       "to reload the note editor"));
                *m_pNote = note;
                // FIXME: the following line shouldn't really be here by the comparison logics, need to test if things work ok without it
                noteToEditorContent();
                return;
            }
        }

        // If we got here, it's either another note or the same note with sufficient updates

        // Remove the no longer needed html file with the note editor page
        Q_UNUSED(removeFile(noteEditorPagePath()))

        *m_pNote = note;

        if (!sameUpdatedNote) {
            m_decryptedTextManager->clearNonRememberedForSessionEntries();
            QNTRACE(QStringLiteral("Removed non-per-session saved passphrases from decrypted text manager"));
        }
    }

    // Clear the caches from previous note
    m_resourceInfo.clear();
    m_genericResourceLocalUidBySaveToStorageRequestIds.clear();
    m_imageResourceSaveToStorageRequestIds.clear();
    m_resourceFileStoragePathsByResourceLocalUid.clear();
    m_genericResourceImageFilePathsByResourceHash.clear();
    m_saveGenericResourceImageToFileRequestIds.clear();
    rebuildRecognitionIndicesCache();

    m_lastSearchHighlightedText.resize(0);
    m_lastSearchHighlightedTextCaseSensitivity = false;

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    m_localUidsOfResourcesWantedToBeSaved.clear();
#else
    NoteEditorPage * pNoteEditorPage = qobject_cast<NoteEditorPage*>(page());
    if (Q_LIKELY(pNoteEditorPage))
    {
        bool missingPluginFactory = !m_pPluginFactory;
        if (missingPluginFactory) {
            m_pPluginFactory = new NoteEditorPluginFactory(*this, *m_pResourceFileStorageManager, *m_pFileIOThreadWorker, pNoteEditorPage);
        }

        m_pPluginFactory->setNote(*m_pNote);

        if (missingPluginFactory) {
            pNoteEditorPage->setPluginFactory(m_pPluginFactory);
        }
    }
#endif

    m_resourceLocalUidAndFileStoragePathByReadResourceRequestIds.clear();
    emit currentNoteChanged(*m_pNote);

    noteToEditorContent();

    QNTRACE(QStringLiteral("Done setting the current note and notebook"));
}

void NoteEditorPrivate::clear()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::clear"));

    m_pNote.reset(Q_NULLPTR);
    m_pNotebook.reset(Q_NULLPTR);

    // Clear the caches from previous note
    m_resourceInfo.clear();
    m_genericResourceLocalUidBySaveToStorageRequestIds.clear();
    m_imageResourceSaveToStorageRequestIds.clear();
    m_resourceFileStoragePathsByResourceLocalUid.clear();
    m_genericResourceImageFilePathsByResourceHash.clear();
    m_saveGenericResourceImageToFileRequestIds.clear();
    m_recognitionIndicesByResourceHash.clear();

    m_lastSearchHighlightedText.resize(0);
    m_lastSearchHighlightedTextCaseSensitivity = false;

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    m_localUidsOfResourcesWantedToBeSaved.clear();
#else
    page()->setPluginFactory(Q_NULLPTR);
    delete m_pPluginFactory;
    m_pPluginFactory = Q_NULLPTR;
#endif

    m_resourceLocalUidAndFileStoragePathByReadResourceRequestIds.clear();

    clearEditorContent();
}

void NoteEditorPrivate::convertToNote()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::convertToNote"));
    m_pendingConversionToNote = true;

    ErrorString error;
    bool res = htmlToNoteContent(error);
    if (!res) {
        m_pendingConversionToNote = false;
    }
}

void NoteEditorPrivate::updateFromNote()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::updateFromNote"));
    noteToEditorContent();
}

void NoteEditorPrivate::setNoteHtml(const QString & html)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setNoteHtml"));

    m_pendingConversionToNote = true;
    onPageHtmlReceived(html);

    writeNotePageFile(html);
}

void NoteEditorPrivate::addResourceToNote(const Resource & resource)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::addResourceToNote"));
    QNTRACE(resource);

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't add the resource to note: no note is set to the editor"));
        QNWARNING(error << QStringLiteral(", resource to add: ") << resource);
        emit notifyError(error);
        return;
    }

    m_pNote->addResource(resource);
    emit convertedToNote(*m_pNote);

    if (resource.hasDataHash() && resource.hasRecognitionDataBody())
    {
        ResourceRecognitionIndices recoIndices(resource.recognitionDataBody());
        if (!recoIndices.isNull() && recoIndices.isValid()) {
            m_recognitionIndicesByResourceHash[resource.dataHash()] = recoIndices;
            QNDEBUG(QStringLiteral("Set recognition indices for new resource: ") << recoIndices);
        }
    }

    saveResourceToLocalFile(resource);
}

void NoteEditorPrivate::removeResourceFromNote(const Resource & resource)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::removeResourceFromNote"));
    QNTRACE(resource);

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't remove the resource from note: no note is set to the editor"));
        QNWARNING(error << QStringLiteral(", resource to remove: ") << resource);
        emit notifyError(error);
        return;
    }

    m_pNote->removeResource(resource);
    emit convertedToNote(*m_pNote);

    if (resource.hasDataHash())
    {
        auto it = m_recognitionIndicesByResourceHash.find(resource.dataHash());
        if (it != m_recognitionIndicesByResourceHash.end()) {
            Q_UNUSED(m_recognitionIndicesByResourceHash.erase(it));
            highlightRecognizedImageAreas(m_lastSearchHighlightedText, m_lastSearchHighlightedTextCaseSensitivity);
        }

        auto imageIt = m_genericResourceImageFilePathsByResourceHash.find(resource.dataHash());
        if (imageIt != m_genericResourceImageFilePathsByResourceHash.end()) {
            Q_UNUSED(m_genericResourceImageFilePathsByResourceHash.erase(imageIt))
        }
    }
}

void NoteEditorPrivate::replaceResourceInNote(const Resource & resource)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::replaceResourceInNote"));
    QNTRACE(resource);

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't replace the resource within note: no note is set to the editor"));
        QNWARNING(error << QStringLiteral(", replacement resource: ") << resource);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!m_pNote->hasResources())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't replace the resource within note: note has no resources"));
        QNWARNING(error << QStringLiteral(", replacement resource: ") << resource);
        emit notifyError(error);
        return;
    }

    QList<Resource> resources = m_pNote->resources();
    int resourceIndex = -1;
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const Resource & currentResource = resources[i];
        if (currentResource.localUid() == resource.localUid()) {
            resourceIndex = i;
            break;
        }
    }

    if (Q_UNLIKELY(resourceIndex < 0)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't replace the resource within note: can't find the resource to be replaced"));
        QNWARNING(error << QStringLiteral(", replacement resource: ") << resource);
        emit notifyError(error);
        return;
    }

    const Resource & targetResource = resources[resourceIndex];
    QByteArray previousResourceHash;
    if (targetResource.hasDataHash()) {
        previousResourceHash = targetResource.dataHash();
    }

    updateResource(targetResource.localUid(), previousResourceHash, resource);
}

void NoteEditorPrivate::setNoteResources(const QList<Resource> & resources)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setNoteResources"));

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't set the resources to the note: no note is set to the editor"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    m_pNote->setResources(resources);
    emit convertedToNote(*m_pNote);

    rebuildRecognitionIndicesCache();
}

bool NoteEditorPrivate::isModified() const
{
    return m_modified;
}

void NoteEditorPrivate::setFocusToEditor()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setFocusToEditor"));
    setFocus();
}

void NoteEditorPrivate::setModified()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setModified"));

    if (!m_pNote) {
        QNDEBUG(QStringLiteral("No note is set to the editor"));
        return;
    }

    if (!m_modified) {
        m_modified = true;
        QNTRACE(QStringLiteral("Emitting noteModified signal"));
        emit noteModified();
    }
}

QString NoteEditorPrivate::noteEditorPagePath() const
{
    if (m_pNote.isNull()) {
        return m_noteEditorPageFolderPath + QStringLiteral("/index.html");
    }

    return m_noteEditorPageFolderPath + QStringLiteral("/") + m_pNote->localUid() + QStringLiteral(".html");
}

void NoteEditorPrivate::setRenameResourceDelegateSubscriptions(RenameResourceDelegate & delegate)
{
    QObject::connect(&delegate, QNSIGNAL(RenameResourceDelegate,finished,QString,QString,Resource,bool),
                     this, QNSLOT(NoteEditorPrivate,onRenameResourceDelegateFinished,QString,QString,Resource,bool));
    QObject::connect(&delegate, QNSIGNAL(RenameResourceDelegate,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onRenameResourceDelegateError,ErrorString));
    QObject::connect(&delegate, QNSIGNAL(RenameResourceDelegate,cancelled),
                     this, QNSLOT(NoteEditorPrivate,onRenameResourceDelegateCancelled));
}

void NoteEditorPrivate::removeSymlinksToImageResourceFile(const QString & resourceLocalUid)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::removeSymlinksToImageResourceFile: resource local uid = ") << resourceLocalUid);

    if (Q_UNLIKELY(m_pNote.isNull())) {
        QNDEBUG(QStringLiteral("Can't remove symlinks to resource image file: no note is set to the editor"));
        return;
    }

    QString fileStorageDirPath = m_noteEditorImageResourcesStoragePath + QStringLiteral("/") + m_pNote->localUid();
    QString fileStoragePathPrefix = fileStorageDirPath + QStringLiteral("/") + resourceLocalUid;
    QString fileStoragePath = fileStoragePathPrefix + QStringLiteral(".png");

    QDir dir(fileStorageDirPath);
    QFileInfoList entryList = dir.entryInfoList();

    const int numEntries = entryList.size();
    QNTRACE(QStringLiteral("Found ") << numEntries << QStringLiteral(" files in the image resources folder"));

    QString entryFilePath;
    for(int i = 0; i < numEntries; ++i)
    {
        const QFileInfo & entry = entryList[i];
        if (!entry.isSymLink()) {
            continue;
        }

        entryFilePath = entry.absoluteFilePath();
        QNTRACE(QStringLiteral("See if we need to remove the symlink to resource image file ") << entryFilePath);

        if (!entryFilePath.startsWith(fileStoragePathPrefix)) {
            continue;
        }

        if (entryFilePath.toUpper() == fileStoragePath.toUpper()) {
            continue;
        }

        Q_UNUSED(removeFile(entryFilePath))
    }
}

QString NoteEditorPrivate::createSymlinkToImageResourceFile(const QString & fileStoragePath, const QString & localUid,
                                                            ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::createSymlinkToImageResourceFile: file storage path = ") << fileStoragePath
            << QStringLiteral(", local uid = ") << localUid);

    QString linkFileName = fileStoragePath;
    linkFileName.remove(linkFileName.size() - 4, 4);
    linkFileName += QStringLiteral("_");
    linkFileName += QString::number(QDateTime::currentMSecsSinceEpoch());

#ifdef Q_OS_WIN
    linkFileName += QStringLiteral(".lnk");
#else
    linkFileName += QStringLiteral(".png");
#endif

    QNTRACE(QStringLiteral("Link file name = ") << linkFileName);

    removeSymlinksToImageResourceFile(localUid);

    QFile imageResourceFile(fileStoragePath);
    bool res = imageResourceFile.link(linkFileName);
    if (Q_UNLIKELY(!res)) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "can't process the image resource update: can't create a symlink to the resource file");
        errorDescription.details() = imageResourceFile.errorString();
        errorDescription.details() += QStringLiteral(", error code = ");
        errorDescription.details() += QString::number(imageResourceFile.error());
        return QString();
    }

    return linkFileName;
}

void NoteEditorPrivate::onDropEvent(QDropEvent * pEvent)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onDropEvent"));

    if (Q_UNLIKELY(!pEvent)) {
        QNWARNING(QStringLiteral("Null pointer to drop event was detected"));
        return;
    }

    const QMimeData * pMimeData = pEvent->mimeData();
    if (Q_UNLIKELY(!pMimeData)) {
        QNWARNING(QStringLiteral("Null pointer to mime data from drop event was detected"));
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

const Account * NoteEditorPrivate::accountPtr() const
{
    return m_pAccount.data();
}

const Resource NoteEditorPrivate::attachResourceToNote(const QByteArray & data, const QByteArray & dataHash,
                                                              const QMimeType & mimeType, const QString & filename)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::attachResourceToNote: hash = ") << dataHash.toHex()
            << QStringLiteral(", mime type = ") << mimeType.name());

    Resource resource;
    QString resourceLocalUid = resource.localUid();
    resource.setLocalUid(QString());   // Force the resource to have empty local uid for now

    if (Q_UNLIKELY(m_pNote.isNull())) {
        QNINFO(QStringLiteral("Can't attach resource to note editor: no actual note was selected"));
        return resource;
    }

    // Now can return the local uid back to the resource
    resource.setLocalUid(resourceLocalUid);

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
    emit convertedToNote(*m_pNote);

    return resource;
}

template <typename T>
QString NoteEditorPrivate::composeHtmlTable(const T width, const T singleColumnWidth,
                                            const int rows, const int columns,
                                            const bool relative)
{
    // Table header
    QString htmlTable = QStringLiteral("<div><table style=\"border-collapse: collapse; margin-left: 0px; table-layout: fixed; width: ");
    htmlTable += QString::number(width);
    if (relative) {
        htmlTable += QStringLiteral("%");
    }
    else {
        htmlTable += QStringLiteral("px");
    }
    htmlTable += QStringLiteral(";\" ><tbody>");

    for(int i = 0; i < rows; ++i)
    {
        // Row header
        htmlTable += QStringLiteral("<tr>");

        for(int j = 0; j < columns; ++j)
        {
            // Column header
            htmlTable += QStringLiteral("<td style=\"border: 1px solid rgb(219, 219, 219); padding: 10 px; margin: 0px; width: ");
            htmlTable += QString::number(singleColumnWidth);
            if (relative) {
                htmlTable += QStringLiteral("%");
            }
            else {
                htmlTable += QStringLiteral("px");
            }
            htmlTable += QStringLiteral(";\">");

            // Blank line to preserve the size
            htmlTable += QStringLiteral("<div><br></div>");

            // End column
            htmlTable += QStringLiteral("</td>");
        }

        // End row
        htmlTable += QStringLiteral("</tr>");
    }

    // End table
    htmlTable += QStringLiteral("</tbody></table></div>");
    return htmlTable;
}

void NoteEditorPrivate::undo()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::undo"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't perform undo"))

    if (m_pUndoStack->canUndo()) {
        m_pUndoStack->undo();
        setModified();
    }
}

void NoteEditorPrivate::redo()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::redo"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't perform redo"))

    if (m_pUndoStack->canRedo()) {
        m_pUndoStack->redo();
        setModified();
    }
}

void NoteEditorPrivate::undoPageAction()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::undoPageAction"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't undo page action"))

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("textEditingUndoRedoManager.undo()"));
    setModified();
    updateJavaScriptBindings();
}

void NoteEditorPrivate::redoPageAction()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::redoPageAction"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't redo page action"))

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("textEditingUndoRedoManager.redo()"));
    setModified();
    updateJavaScriptBindings();
}

void NoteEditorPrivate::flipEnToDoCheckboxState(const quint64 enToDoIdNumber)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::flipEnToDoCheckboxState: ") << enToDoIdNumber);

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't flip the todo checkbox state"))

    GET_PAGE()
    QString javascript = QString("flipEnToDoCheckboxState(%1);").arg(QString::number(enToDoIdNumber));
    page->executeJavaScript(javascript);
    setModified();
}

qint64 NoteEditorPrivate::noteResourcesSize() const
{
    if (Q_UNLIKELY(!m_pNote)) {
        return qint64(0);
    }

    if (Q_UNLIKELY(!m_pNote->hasResources())) {
        return qint64(0);
    }

    qint64 size = 0;
    QList<Resource> resources = m_pNote->resources();
    for(auto it = resources.constBegin(), end = resources.constEnd(); it != end; ++it)
    {
        const Resource & resource = *it;

        if (resource.hasDataSize()) {
            size += resource.dataSize();
        }

        if (resource.hasAlternateDataSize()) {
            size += resource.alternateDataSize();
        }

        if (resource.hasRecognitionDataSize()) {
            size += resource.recognitionDataSize();
        }
    }

    return size;
}

qint64 NoteEditorPrivate::noteContentSize() const
{
    if (Q_UNLIKELY(!m_pNote)) {
        return qint64(0);
    }

    if (Q_UNLIKELY(!m_pNote->hasContent())) {
        return qint64(0);
    }

    return static_cast<qint64>(m_pNote->content().size());
}

qint64 NoteEditorPrivate::noteSize() const
{
    return noteContentSize() + noteResourcesSize();
}

void NoteEditorPrivate::onSpellCheckCorrectionAction()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onSpellCheckCorrectionAction"));

    if (!m_spellCheckerEnabled) {
        QNDEBUG(QStringLiteral("Not enabled, won't do anything"));
        return;
    }

    QAction * action = qobject_cast<QAction*>(sender());
    if (Q_UNLIKELY(!action)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't get the action which has toggled the spelling correction"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    const QString & correction = action->text();
    if (Q_UNLIKELY(correction.isEmpty())) {
        QNWARNING(QStringLiteral("No correction specified"));
        return;
    }

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("spellChecker.correctSpelling('") + correction + QStringLiteral("');"),
                            NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onSpellCheckCorrectionActionDone));
}

void NoteEditorPrivate::onSpellCheckIgnoreWordAction()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onSpellCheckIgnoreWordAction"));

    if (!m_spellCheckerEnabled) {
        QNDEBUG(QStringLiteral("Not enabled, won't do anything"));
        return;
    }

    if (Q_UNLIKELY(!m_pSpellChecker)) {
        QNDEBUG(QStringLiteral("Spell checker is null, won't do anything"));
        return;
    }

    m_pSpellChecker->ignoreWord(m_lastMisSpelledWord);
    m_currentNoteMisSpelledWords.removeAll(m_lastMisSpelledWord);
    applySpellCheck();

    SpellCheckIgnoreWordUndoCommand * pCommand = new SpellCheckIgnoreWordUndoCommand(*this, m_lastMisSpelledWord, m_pSpellChecker);
    QObject::connect(pCommand, QNSIGNAL(SpellCheckIgnoreWordUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::onSpellCheckAddWordToUserDictionaryAction()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onSpellCheckAddWordToUserDictionaryAction"));

    if (!m_spellCheckerEnabled) {
        QNDEBUG(QStringLiteral("Not enabled, won't do anything"));
        return;
    }

    if (Q_UNLIKELY(!m_pSpellChecker)) {
        QNDEBUG(QStringLiteral("Spell checker is null, won't do anything"));
        return;
    }

    m_pSpellChecker->addToUserWordlist(m_lastMisSpelledWord);
    m_currentNoteMisSpelledWords.removeAll(m_lastMisSpelledWord);
    applySpellCheck();

    SpellCheckAddToUserWordListUndoCommand * pCommand = new SpellCheckAddToUserWordListUndoCommand(*this, m_lastMisSpelledWord, m_pSpellChecker);
    QObject::connect(pCommand, QNSIGNAL(SpellCheckAddToUserWordListUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);
}

void NoteEditorPrivate::onSpellCheckCorrectionActionDone(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onSpellCheckCorrectionActionDone: ") << data);

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of spelling correction from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "can't parse the error of spelling correction from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "can't correct spelling");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    SpellCorrectionUndoCommand * pCommand =
        new SpellCorrectionUndoCommand(*this,
                                       NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onSpellCheckCorrectionUndoRedoFinished));

    QObject::connect(pCommand, QNSIGNAL(SpellCorrectionUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    applySpellCheck();

    convertToNote();
}

void NoteEditorPrivate::onSpellCheckCorrectionUndoRedoFinished(const QVariant & data, const QVector<QPair<QString,QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onSpellCheckCorrectionUndoRedoFinished"));

    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = data.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "Can't parse the result of spelling correction undo/redo from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        ErrorString error;

        auto errorIt = resultMap.find(QStringLiteral("error"));
        if (Q_UNLIKELY(errorIt == resultMap.end())) {
            error.base() = QT_TRANSLATE_NOOP("", "Can't parse the error of spelling correction undo/redo from JavaScript");
        }
        else {
            error.base() = QT_TRANSLATE_NOOP("", "Can't undo/redo the spelling correction");
            error.details() = errorIt.value().toString();
        }

        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    convertToNote();
}

void NoteEditorPrivate::onSpellCheckerDynamicHelperUpdate(QStringList words)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onSpellCheckerDynamicHelperUpdate: ") << words.join(QStringLiteral(";")));

    if (!m_spellCheckerEnabled) {
        QNTRACE(QStringLiteral("No spell checking is enabled, nothing to do"));
        return;
    }

    if (Q_UNLIKELY(!m_pSpellChecker)) {
        QNDEBUG(QStringLiteral("Spell checker is null, won't do anything"));
        return;
    }

    for(auto it = words.begin(), end = words.end(); it != end; ++it)
    {
        QString word = *it;
        word = word.trimmed();
        m_stringUtils.removePunctuation(word);

        if (m_pSpellChecker->checkSpell(word)) {
            QNTRACE(QStringLiteral("No misspelling detected"));
            continue;
        }

        if (!m_currentNoteMisSpelledWords.contains(word)) {
            m_currentNoteMisSpelledWords << word;
        }
    }

    QNTRACE(QStringLiteral("Current note's misspelled words: ") << m_currentNoteMisSpelledWords.join(QStringLiteral(", ")));

    applySpellCheck(/* apply to selection = */ true);
}

void NoteEditorPrivate::onSpellCheckerReady()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onSpellCheckerReady"));

    QObject::disconnect(m_pSpellChecker, QNSIGNAL(SpellChecker,ready), this, QNSLOT(NoteEditorPrivate,onSpellCheckerReady));

    if (m_spellCheckerEnabled) {
        enableSpellCheck();
    }
    else {
        disableSpellCheck();
    }

    emit spellCheckerReady();
}

void NoteEditorPrivate::onImageResourceResized(bool pushUndoCommand)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onImageResourceResized: push undo command = ")
            << (pushUndoCommand ? QStringLiteral("true") : QStringLiteral("false")));

    if (pushUndoCommand) {
        ImageResizeUndoCommand * pCommand = new ImageResizeUndoCommand(*this);
        QObject::connect(pCommand, QNSIGNAL(ImageResizeUndoCommand,notifyError,ErrorString),
                         this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
        m_pUndoStack->push(pCommand);
    }

    convertToNote();
}

void NoteEditorPrivate::copy()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::copy"));
    GET_PAGE()
    page->triggerAction(WebPage::Copy);
}

void NoteEditorPrivate::paste()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::paste"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't paste"))

    GET_PAGE()

    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        QNWARNING(QStringLiteral("Can't access the application clipboard to analyze the pasted content"));
        page->triggerAction(WebPage::Paste);
        setModified();
        return;
    }

    QString textToPaste = pClipboard->text();
    QNTRACE(QStringLiteral("Text to paste: ") << textToPaste);

    bool shouldBeHyperlink = textToPaste.startsWith(QStringLiteral("http://")) ||
                             textToPaste.startsWith(QStringLiteral("https://")) ||
                             textToPaste.startsWith(QStringLiteral("mailto:")) ||
                             textToPaste.startsWith(QStringLiteral("ftp://"));
    bool shouldBeAttachment = textToPaste.startsWith(QStringLiteral("file://"));

    if (!shouldBeHyperlink && !shouldBeAttachment) {
        QNTRACE(QStringLiteral("The pasted text doesn't appear to be a url of hyperlink or attachment"));
        page->triggerAction(WebPage::Paste);
        return;
    }

    QUrl url(textToPaste);
    if (shouldBeAttachment)
    {
        if (!url.isValid()) {
            QNTRACE(QStringLiteral("The pasted text seemed like file url but the url isn't valid after all, fallback to simple paste"));
            page->triggerAction(WebPage::Paste);
            setModified();
        }
        else {
            dropFile(url.toLocalFile());
        }

        return;
    }

    if (!url.isValid()) {
        url.setScheme(QStringLiteral("evernote"));
    }

    if (!url.isValid()) {
        QNDEBUG(QStringLiteral("It appears we don't paste a url"));
        page->triggerAction(WebPage::Paste);
        setModified();
        return;
    }

    QNDEBUG(QStringLiteral("Was able to create the url from pasted text, inserting a hyperlink"));

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    textToPaste = url.toString(QUrl::FullyEncoded);
#else
    textToPaste = url.toString(QUrl::None);
#endif

    quint64 hyperlinkId = m_lastFreeHyperlinkIdNumber++;
    setupAddHyperlinkDelegate(hyperlinkId, textToPaste);
}

void NoteEditorPrivate::pasteUnformatted()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::pasteUnformatted"));
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't paste the unformatted text"));
    GET_PAGE()
    page->triggerAction(WebPage::PasteAndMatchStyle);
    setModified();
}

void NoteEditorPrivate::selectAll()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::selectAll"));
    GET_PAGE()
    page->triggerAction(WebPage::SelectAll);
}

void NoteEditorPrivate::fontMenu()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::fontMenu"));

    bool fontWasChosen = false;
    QFont chosenFont = QFontDialog::getFont(&fontWasChosen, m_font, this);
    if (!fontWasChosen) {
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

void NoteEditorPrivate::cut()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::cut"));
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't cut note content"))
    execJavascriptCommand(QStringLiteral("cut"));
    setModified();
}

void NoteEditorPrivate::textBold()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::textBold"));
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't toggle bold text"))
    execJavascriptCommand(QStringLiteral("bold"));
    setModified();
}

void NoteEditorPrivate::textItalic()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::textItalic"));
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't toggle italic text"))
    execJavascriptCommand(QStringLiteral("italic"));
    setModified();
}

void NoteEditorPrivate::textUnderline()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::textUnderline"));
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't toggle underline text"))
    execJavascriptCommand(QStringLiteral("underline"));
    setModified();
}

void NoteEditorPrivate::textStrikethrough()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::textStrikethrough"));
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't toggle strikethrough text"))
    execJavascriptCommand(QStringLiteral("strikethrough"));
    setModified();
}

void NoteEditorPrivate::textHighlight()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::textHighlight"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't highlight text"))
    setBackgroundColor(QColor(255, 255, 127));
    setModified();
}

void NoteEditorPrivate::alignLeft()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::alignLeft"));
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't justify the text to the left"))
    execJavascriptCommand(QStringLiteral("justifyleft"));
    setModified();
}

void NoteEditorPrivate::alignCenter()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::alignCenter"));
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't justify the text to the center"))
    execJavascriptCommand(QStringLiteral("justifycenter"));
    setModified();
}

void NoteEditorPrivate::alignRight()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::alignRight"));
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't justify the text to the right"))
    execJavascriptCommand(QStringLiteral("justifyright"));
    setModified();
}

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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::findNext: ") << text << QStringLiteral("; match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    findText(text, matchCase);
}

void NoteEditorPrivate::findPrevious(const QString & text, const bool matchCase) const
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::findPrevious: ") << text << QStringLiteral("; match case = ")
            << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    findText(text, matchCase, /* search backward = */ true);
}

void NoteEditorPrivate::replace(const QString & textToReplace, const QString & replacementText, const bool matchCase)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::replace: text to replace = ") << textToReplace << QStringLiteral("; replacement text = ")
            << replacementText << QStringLiteral("; match case = ") << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    GET_PAGE()
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't replace text"))

    QString escapedTextToReplace = textToReplace;
    ENMLConverter::escapeString(escapedTextToReplace);

    QString escapedReplacementText = replacementText;
    ENMLConverter::escapeString(escapedReplacementText);

    QString javascript = QString("findReplaceManager.replace('%1', '%2', %3);").arg(escapedTextToReplace, escapedReplacementText,
                                                                                    (matchCase ? QStringLiteral("true") : QStringLiteral("false")));
    page->executeJavaScript(javascript, ReplaceCallback(this));

    ReplaceUndoCommand * pCommand = new ReplaceUndoCommand(textToReplace, matchCase, *this, ReplaceCallback(this));
    QObject::connect(pCommand, QNSIGNAL(ReplaceUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    setSearchHighlight(textToReplace, matchCase, /* force = */ true);
    findNext(textToReplace, matchCase);
}

void NoteEditorPrivate::replaceAll(const QString & textToReplace, const QString & replacementText, const bool matchCase)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::replaceAll: text to replace = ") << textToReplace << QStringLiteral("; replacement text = ")
            << replacementText << QStringLiteral("; match case = ") << (matchCase ? QStringLiteral("true") : QStringLiteral("false")));

    GET_PAGE()
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't replace all occurrences"))

    QString escapedTextToReplace = textToReplace;
    ENMLConverter::escapeString(escapedTextToReplace);

    QString escapedReplacementText = replacementText;
    ENMLConverter::escapeString(escapedReplacementText);

    QString javascript = QString("findReplaceManager.replaceAll('%1', '%2', %3);").arg(escapedTextToReplace, escapedReplacementText, (matchCase ? "true" : "false"));
    page->executeJavaScript(javascript, ReplaceCallback(this));

    ReplaceAllUndoCommand * pCommand = new ReplaceAllUndoCommand(textToReplace, matchCase, *this, ReplaceCallback(this));
    QObject::connect(pCommand, QNSIGNAL(ReplaceAllUndoCommand,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onUndoCommandError,ErrorString));
    m_pUndoStack->push(pCommand);

    setSearchHighlight(textToReplace, matchCase, /* force = */ true);
}

void NoteEditorPrivate::onReplaceJavaScriptDone(const QVariant & data)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onReplaceJavaScriptDone"));

    Q_UNUSED(data)

    setModified();
    convertToNote();
}

void NoteEditorPrivate::insertToDoCheckbox()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::insertToDoCheckbox"));

    GET_PAGE()
    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't insert checkbox"))

    QString html = ENMLConverter::toDoCheckboxHtml(/* checked = */ false, m_lastFreeEnToDoIdNumber++);
    QString javascript = QString("managedPageAction('insertHtml', '%1'); ").arg(html);
    javascript += m_setupEnToDoTagsJs;

    page->executeJavaScript(javascript);
    setModified();
}

void NoteEditorPrivate::setSpellcheck(const bool enabled)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setSpellcheck: enabled = ")
            << (enabled ? QStringLiteral("true") : QStringLiteral("false")));

    if (m_spellCheckerEnabled == enabled) {
        QNTRACE(QStringLiteral("Spell checker enabled flag didn't change"));
        return;
    }

    m_spellCheckerEnabled = enabled;
    if (m_spellCheckerEnabled) {
        enableSpellCheck();
    }
    else {
        disableSpellCheck();
    }
}

bool NoteEditorPrivate::spellCheckEnabled() const
{
    return m_spellCheckerEnabled;
}

void NoteEditorPrivate::setFont(const QFont & font)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setFont: ") << font.family()
            << QStringLiteral(", point size = ") << font.pointSize()
            << QStringLiteral(", previous font family = ") << m_font.family()
            << QStringLiteral(", previous font point size = ") << m_font.pointSize());

    if (m_font.family() == font.family()) {
        QNTRACE(QStringLiteral("Font family hasn't changed, nothing to to do"));
        return;
    }

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't change font"))

    m_font = font;
    QString fontName = font.family();
    execJavascriptCommand(QStringLiteral("fontName"), fontName);
    emit textFontFamilyChanged(font.family());

    if (hasSelection()) {
        setModified();
    }
}

void NoteEditorPrivate::setFontHeight(const int height)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setFontHeight: ") << height);

    if (height <= 0) {
        ErrorString error(QT_TRANSLATE_NOOP("", "detected incorrect font size"));
        error.details() = QString::number(height);
        QNINFO(error);
        emit notifyError(error);
        return;
    }

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't change the font height"))

    m_font.setPointSize(height);
    GET_PAGE()
    page->executeJavaScript(QStringLiteral("changeFontSizeForSelection(") + QString::number(height) + QStringLiteral(");"));
    emit textFontSizeChanged(height);

    if (hasSelection()) {
        setModified();
    }
}

void NoteEditorPrivate::setFontColor(const QColor & color)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setFontColor: ") << color.name()
            << QStringLiteral(", rgb: ") << QString::number(color.rgb(), 16));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't set the font color"))

    if (!color.isValid()) {
        ErrorString error(QT_TRANSLATE_NOOP("", "detected invalid font color"));
        error.details() = color.name();
        QNINFO(error);
        emit notifyError(error);
        return;
    }

    execJavascriptCommand(QStringLiteral("foreColor"), color.name());

    if (hasSelection()) {
        setModified();
    }
}

void NoteEditorPrivate::setBackgroundColor(const QColor & color)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::setBackgroundColor: ") << color.name()
            << QStringLiteral(", rgb: ") << QString::number(color.rgb(), 16));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't set the background color"))

    if (!color.isValid()) {
        ErrorString error(QT_TRANSLATE_NOOP("", "detected invalid background color"));
        error.details() = color.name();
        QNINFO(error);
        emit notifyError(error);
        return;
    }

    execJavascriptCommand(QStringLiteral("hiliteColor"), color.name());

    if (hasSelection()) {
        setModified();
    }
}

void NoteEditorPrivate::insertHorizontalLine()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::insertHorizontalLine"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't insert a horizontal line"))
    execJavascriptCommand(QStringLiteral("insertHorizontalRule"));
    setModified();
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::insertBulletedList"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't insert an unordered list"))
    execJavascriptCommand(QStringLiteral("insertUnorderedList"));
    setModified();
}

void NoteEditorPrivate::insertNumberedList()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::insertNumberedList"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't insert a numbered list"))
    execJavascriptCommand(QStringLiteral("insertOrderedList"));
    setModified();
}

void NoteEditorPrivate::insertTableDialog()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::insertTableDialog"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't insert a table"))
    emit insertTableDialogRequested();
}

#define CHECK_NUM_COLUMNS() \
    if (columns <= 0) { \
        ErrorString error(QT_TRANSLATE_NOOP("", "detected attempt to insert a table with negative or zero number of columns")); \
        error.details() = QString::number(columns); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

#define CHECK_NUM_ROWS() \
    if (rows <= 0) { \
        ErrorString error(QT_TRANSLATE_NOOP("", "detected attempt to insert a table with negative or zero number of rows")); \
        error.details() = QString::number(rows); \
        QNWARNING(error); \
        emit notifyError(error); \
        return; \
    }

void NoteEditorPrivate::insertFixedWidthTable(const int rows, const int columns, const int widthInPixels)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::insertFixedWidthTable: rows = ") << rows
            << QStringLiteral(", columns = ") << columns << QStringLiteral(", width in pixels = ")
            << widthInPixels);

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't insert a fixed width table"))

    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    int pageWidth = geometry().width();
    if (widthInPixels > 2 * pageWidth) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't insert table, width is too large (more than twice the page width)"));
        error.details() = QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (widthInPixels <= 0) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't insert table, bad width"));
        error.details() = QString::number(widthInPixels);
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int singleColumnWidth = widthInPixels / columns;
    if (singleColumnWidth == 0) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't insert table, bad width for specified number of columns "
                                            "(single column width is zero)"));
        error.details() = QString::number(widthInPixels);
        error.details() += QStringLiteral(", ");
        error.details() += QString::number(columns);
        error.details() += QStringLiteral("columns");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString htmlTable = composeHtmlTable(widthInPixels, singleColumnWidth, rows, columns,
                                         /* relative = */ false);
    execJavascriptCommand(QStringLiteral("insertHTML"), htmlTable);
    setModified();
    updateColResizableTableBindings();
}

void NoteEditorPrivate::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::insertRelativeWidthTable: rows = ") << rows
            << QStringLiteral(", columns = ") << columns << QStringLiteral(", relative width = ") << relativeWidth);

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't insert a relative width table"))

    CHECK_NUM_COLUMNS();
    CHECK_NUM_ROWS();

    if (relativeWidth <= 0.01)
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't insert table, relative width is too small"));
        error.details() = QString::number(relativeWidth);
        error.details() += QStringLiteral("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }
    else if (relativeWidth > 100.0 + 1.0e-9)
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't insert table, relative width is too large"));
        error.details() = QString::number(relativeWidth);
        error.details() += QStringLiteral("%");
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    double singleColumnWidth = relativeWidth / columns;
    QString htmlTable = composeHtmlTable(relativeWidth, singleColumnWidth, rows, columns,
                                         /* relative = */ true);
    execJavascriptCommand(QStringLiteral("insertHTML"), htmlTable);
    setModified();
    updateColResizableTableBindings();
}

void NoteEditorPrivate::insertTableRow()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::insertTableRow"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't insert a table row"))

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onTableActionDone);

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("tableManager.insertRow();"), callback);

    pushTableActionUndoCommand(tr("Insert row"), callback);
}

void NoteEditorPrivate::insertTableColumn()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::insertTableColumn"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't insert a table column"))

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onTableActionDone);

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("tableManager.insertColumn();"), callback);

    pushTableActionUndoCommand(tr("Insert column"), callback);
}

void NoteEditorPrivate::removeTableRow()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::removeTableRow"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't remove the table row"))

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onTableActionDone);

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("tableManager.removeRow();"), callback);

    pushTableActionUndoCommand(tr("Remove row"), callback);
}

void NoteEditorPrivate::removeTableColumn()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::removeTableColumn"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't remove the table column"))

    NoteEditorCallbackFunctor<QVariant> callback(this, &NoteEditorPrivate::onTableActionDone);

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("tableManager.removeColumn();"), callback);

    pushTableActionUndoCommand(tr("Remove column"), callback);
}

void NoteEditorPrivate::addAttachmentDialog()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::addAttachmentDialog"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't add an attachment"))

    QString addAttachmentInitialFolderPath;

    ApplicationSettings appSettings;
    QVariant lastAttachmentAddLocation = appSettings.value(QStringLiteral("LastAttachmentAddLocation"));
    if (!lastAttachmentAddLocation.isNull() && lastAttachmentAddLocation.isValid())
    {
        QNTRACE(QStringLiteral("Found last attachment add location: ") << lastAttachmentAddLocation);
        QFileInfo lastAttachmentAddDirInfo(lastAttachmentAddLocation.toString());
        if (!lastAttachmentAddDirInfo.exists()) {
            QNTRACE(QStringLiteral("Cached last attachment add directory does not exist"));
        }
        else if (!lastAttachmentAddDirInfo.isDir()) {
            QNTRACE(QStringLiteral("Cached last attachment add directory path is not a directory really"));
        }
        else if (!lastAttachmentAddDirInfo.isWritable()) {
            QNTRACE(QStringLiteral("Cached last attachment add directory path is not writable"));
        }
        else {
            addAttachmentInitialFolderPath = lastAttachmentAddDirInfo.absolutePath();
        }
    }

    QString absoluteFilePath = QFileDialog::getOpenFileName(this, tr("Add attachment..."),
                                                            addAttachmentInitialFolderPath);
    if (absoluteFilePath.isEmpty()) {
        QNTRACE(QStringLiteral("User cancelled adding the attachment"));
        return;
    }

    QNTRACE(QStringLiteral("Absolute file path of chosen attachment: ") << absoluteFilePath);

    QFileInfo fileInfo(absoluteFilePath);
    QString absoluteDirPath = fileInfo.absoluteDir().absolutePath();
    if (!absoluteDirPath.isEmpty()) {
        appSettings.setValue(QStringLiteral("LastAttachmentAddLocation"), absoluteDirPath);
        QNTRACE(QStringLiteral("Updated last attachment add location to ") << absoluteDirPath);
    }

    dropFile(absoluteFilePath);
}

void NoteEditorPrivate::saveAttachmentDialog(const QByteArray & resourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::saveAttachmentDialog"));
    onSaveResourceRequest(resourceHash);
}

void NoteEditorPrivate::saveAttachmentUnderCursor()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::saveAttachmentUnderCursor"));

    if ((m_currentContextMenuExtraData.m_contentType != QStringLiteral("ImageResource")) &&
        (m_currentContextMenuExtraData.m_contentType != QStringLiteral("NonImageResource")))
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't save attachment under cursor: wrong current context menu extra data's content type"));
        QNWARNING(error << QStringLiteral(": content type = ") << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    saveAttachmentDialog(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::openAttachment(const QByteArray & resourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::openAttachment"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't open the attachment"))
    onOpenResourceRequest(resourceHash);
}

void NoteEditorPrivate::openAttachmentUnderCursor()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::openAttachmentUnderCursor"));

    if ((m_currentContextMenuExtraData.m_contentType != QStringLiteral("ImageResource")) &&
        (m_currentContextMenuExtraData.m_contentType != QStringLiteral("NonImageResource")))
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't open attachment under cursor: wrong current context menu extra data's content type"));
        error.details() = m_currentContextMenuExtraData.m_contentType;
        QNWARNING(error << QStringLiteral(": content type = ") << m_currentContextMenuExtraData.m_contentType);
        emit notifyError(error);
        return;
    }

    openAttachment(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::copyAttachment(const QByteArray & resourceHash)
{
    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't copy the attachment: no note is set to the editor"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QList<Resource> resources = m_pNote->resources();
    int resourceIndex = resourceIndexByHash(resources, resourceHash);
    if (Q_UNLIKELY(resourceIndex < 0)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "the attachment to be copied was not found within the note"));
        QNWARNING(error << QStringLiteral(", resource hash = ") << resourceHash.toHex());
        emit notifyError(error);
        return;
    }

    const Resource & resource = resources[resourceIndex];

    if (Q_UNLIKELY(!resource.hasDataBody() && !resource.hasAlternateDataBody())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't copy the attachment as it has neither data body nor alternate data body"));
        QNWARNING(error << QStringLiteral(", resource hash = ") << resourceHash.toHex());
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!resource.hasMime())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't copy the attachment as it has no mime type"));
        QNWARNING(error << QStringLiteral(", resource hash = ") << resourceHash.toHex());
        emit notifyError(error);
        return;
    }

    const QByteArray & data = (resource.hasDataBody() ? resource.dataBody() : resource.alternateDataBody());
    const QString & mimeType = resource.mime();

    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't copy the attachment: can't get access to clipboard"));
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::copyAttachmentUnderCursor"));

    if ((m_currentContextMenuExtraData.m_contentType != QStringLiteral("ImageResource")) &&
        (m_currentContextMenuExtraData.m_contentType != QStringLiteral("NonImageResource")))
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't copy the attachment under cursor: wrong current context menu extra data's content type"));
        error.details() = m_currentContextMenuExtraData.m_contentType;
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    copyAttachment(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::removeAttachment(const QByteArray & resourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::removeAttachment: hash = ") << resourceHash.toHex());

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't remove the attachment by hash: no note is set to the editor"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't remove the attachment"))

    bool foundResourceToRemove = false;
    QList<Resource> resources = m_pNote->resources();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const Resource & resource = resources[i];
        if (resource.hasDataHash() && (resource.dataHash() == resourceHash))
        {
            m_resourceInfo.removeResourceInfo(resource.dataHash());

            RemoveResourceDelegate * delegate = new RemoveResourceDelegate(resource, *this);
            QObject::connect(delegate, QNSIGNAL(RemoveResourceDelegate,finished,Resource),
                             this, QNSLOT(NoteEditorPrivate,onRemoveResourceDelegateFinished,Resource));
            QObject::connect(delegate, QNSIGNAL(RemoveResourceDelegate,notifyError,ErrorString),
                             this, QNSLOT(NoteEditorPrivate,onRemoveResourceDelegateError,ErrorString));
            delegate->start();

            foundResourceToRemove = true;
            break;
        }
    }

    if (Q_UNLIKELY(!foundResourceToRemove)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't remove the attachment by hash: no resource with such hash was found within the note"));
        error.details() = resourceHash.toHex();
        QNWARNING(error);
        emit notifyError(error);
    }
}

void NoteEditorPrivate::removeAttachmentUnderCursor()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::removeAttachmentUnderCursor"));

    if ((m_currentContextMenuExtraData.m_contentType != QStringLiteral("ImageResource")) &&
        (m_currentContextMenuExtraData.m_contentType != QStringLiteral("NonImageResource")))
    {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't remove the attachment under cursor: wrong current context menu extra data's content type"));
        error.details() = m_currentContextMenuExtraData.m_contentType;
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    removeAttachment(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::renameAttachmentUnderCursor()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::renameAttachmentUnderCursor"));

    if (m_currentContextMenuExtraData.m_contentType != QStringLiteral("NonImageResource")) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't rename the attachment under cursor: wrong current context menu extra data's content type"));
        error.details() = m_currentContextMenuExtraData.m_contentType;
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    renameAttachment(m_currentContextMenuExtraData.m_resourceHash);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::renameAttachment(const QByteArray & resourceHash)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::renameAttachment: resource hash = ") << resourceHash.toHex());

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "Can't rename the attachment"));
    CHECK_NOTE_EDITABLE(errorPrefix)

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error = errorPrefix;
        error.additionalBases().append(QT_TRANSLATE_NOOP("", "no note is set to the editor"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int targetResourceIndex = -1;
    QList<Resource> resources = m_pNote->resources();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const Resource & resource = resources[i];
        if (!resource.hasDataHash() || (resource.dataHash() != resourceHash)) {
            continue;
        }

        targetResourceIndex = i;
        break;
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        ErrorString error = errorPrefix;
        error.additionalBases().append(QT_TRANSLATE_NOOP("", "can't find the corresponding resource in the note"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    Resource & resource = resources[targetResourceIndex];
    if (Q_UNLIKELY(!resource.hasDataBody())) {
        ErrorString error = errorPrefix;
        error.additionalBases().append(QT_TRANSLATE_NOOP("", "the resource doesn't have the data body set"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!resource.hasDataHash())) {
        ErrorString error = errorPrefix;
        error.additionalBases().append(QT_TRANSLATE_NOOP("", "the resource doesn't have the data hash set"));
        QNWARNING(error << QStringLiteral(", resource: ") << resource);
        emit notifyError(error);
        return;
    }

    RenameResourceDelegate * delegate = new RenameResourceDelegate(resource, *this, m_pGenericResourceImageManager, m_genericResourceImageFilePathsByResourceHash);
    setRenameResourceDelegateSubscriptions(*delegate);
    delegate->start();
}

void NoteEditorPrivate::rotateImageAttachment(const QByteArray & resourceHash, const Rotation::type rotationDirection)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::rotateImageAttachment: resource hash = ") << resourceHash.toHex()
            << QStringLiteral(", rotation: ") << rotationDirection);

    ErrorString errorPrefix(QT_TRANSLATE_NOOP("", "can't rotate the image attachment"));
    CHECK_NOTE_EDITABLE(errorPrefix)

    if (Q_UNLIKELY(m_pNote.isNull())) {
        ErrorString error = errorPrefix;
        error.additionalBases().append(QT_TRANSLATE_NOOP("", "no note is set to the editor"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    int targetResourceIndex = -1;
    QList<Resource> resources = m_pNote->resources();
    const int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const Resource & resource = resources[i];
        if (!resource.hasDataHash() || (resource.dataHash() != resourceHash)) {
            continue;
        }

        if (Q_UNLIKELY(!resource.hasMime())) {
            ErrorString error = errorPrefix;
            error.additionalBases().append(QT_TRANSLATE_NOOP("", "the corresponding attachment's mime type is not set"));
            QNWARNING(error << QStringLiteral(", resource: ") << resource);
            emit notifyError(error);
            return;
        }

        if (Q_UNLIKELY(!resource.mime().startsWith(QStringLiteral("image/")))) {
            ErrorString error = errorPrefix;
            error.additionalBases().append(QT_TRANSLATE_NOOP("", "the corresponding attachment's mime type indicates it is not an image"));
            error.details() = resource.mime();
            QNWARNING(error << QStringLiteral(", resource: ") << resource);
            emit notifyError(error);
            return;
        }

        targetResourceIndex = i;
        break;
    }

    if (Q_UNLIKELY(targetResourceIndex < 0)) {
        ErrorString error = errorPrefix;
        error.additionalBases().append(QT_TRANSLATE_NOOP("", "can't find the corresponding attachment within the note"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    Resource & resource = resources[targetResourceIndex];
    if (Q_UNLIKELY(!resource.hasDataBody())) {
        ErrorString error = errorPrefix;
        error.additionalBases().append(QT_TRANSLATE_NOOP("", "the attachment doesn't have the data body set"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!resource.hasDataHash())) {
        ErrorString error = errorPrefix;
        error.additionalBases().append(QT_TRANSLATE_NOOP("", "the attachment doesn't have the data hash set"));
        QNWARNING(error << QStringLiteral(", resource: ") << resource);
        emit notifyError(error);
        return;
    }

    ImageResourceRotationDelegate * delegate = new ImageResourceRotationDelegate(resource.dataHash(), rotationDirection,
                                                                                 *this, m_resourceInfo, *m_pResourceFileStorageManager,
                                                                                 m_resourceFileStoragePathsByResourceLocalUid);

    QObject::connect(delegate, QNSIGNAL(ImageResourceRotationDelegate,finished,QByteArray,QByteArray,QByteArray,QByteArray,Resource,INoteEditorBackend::Rotation::type),
                     this, QNSLOT(NoteEditorPrivate,onImageResourceRotationDelegateFinished,QByteArray,QByteArray,QByteArray,QByteArray,Resource,INoteEditorBackend::Rotation::type));
    QObject::connect(delegate, QNSIGNAL(ImageResourceRotationDelegate,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onImageResourceRotationDelegateError,ErrorString));

    delegate->start();
}

void NoteEditorPrivate::rotateImageAttachmentUnderCursor(const Rotation::type rotationDirection)
{
    QNDEBUG(QStringLiteral("INoteEditorBackend::rotateImageAttachmentUnderCursor: rotation: ") << rotationDirection);

    if (m_currentContextMenuExtraData.m_contentType != QStringLiteral("ImageResource")) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't rotate the image attachment under cursor: wrong current context menu extra data's content type"));
        error.details() = m_currentContextMenuExtraData.m_contentType;
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    rotateImageAttachment(m_currentContextMenuExtraData.m_resourceHash, rotationDirection);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::rotateImageAttachmentUnderCursorClockwise()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::rotateImageAttachmentUnderCursorClockwise"));
    rotateImageAttachmentUnderCursor(Rotation::Clockwise);
}

void NoteEditorPrivate::rotateImageAttachmentUnderCursorCounterclockwise()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::rotateImageAttachmentUnderCursorCounterclockwise"));
    rotateImageAttachmentUnderCursor(Rotation::Counterclockwise);
}

void NoteEditorPrivate::encryptSelectedText()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::encryptSelectedText"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't encrypt the selected text"))

    EncryptSelectedTextDelegate * delegate = new EncryptSelectedTextDelegate(this, m_encryptionManager, m_decryptedTextManager);
    QObject::connect(delegate, QNSIGNAL(EncryptSelectedTextDelegate,finished),
                     this, QNSLOT(NoteEditorPrivate,onEncryptSelectedTextDelegateFinished));
    QObject::connect(delegate, QNSIGNAL(EncryptSelectedTextDelegate,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onEncryptSelectedTextDelegateError,ErrorString));
    QObject::connect(delegate, QNSIGNAL(EncryptSelectedTextDelegate,cancelled),
                     this, QNSLOT(NoteEditorPrivate,onEncryptSelectedTextDelegateCancelled));
    delegate->start(m_lastSelectedHtml);
}

void NoteEditorPrivate::decryptEncryptedTextUnderCursor()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::decryptEncryptedTextUnderCursor"));

    if (Q_UNLIKELY(m_currentContextMenuExtraData.m_contentType != QStringLiteral("EncryptedText"))) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't decrypt the encrypted text under cursor: wrong current context menu extra data's content type"));
        error.details() = m_currentContextMenuExtraData.m_contentType;
        QNWARNING(error);
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
    QNDEBUG(QStringLiteral("NoteEditorPrivate::decryptEncryptedText"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't decrypt the encrypted text"))

    DecryptEncryptedTextDelegate * delegate = new DecryptEncryptedTextDelegate(enCryptIndex, encryptedText, cipher, length, hint, this,
                                                                               m_encryptionManager, m_decryptedTextManager);

    QObject::connect(delegate, QNSIGNAL(DecryptEncryptedTextDelegate,finished,QString,QString,size_t,QString,QString,QString,bool,bool),
                     this, QNSLOT(NoteEditorPrivate,onDecryptEncryptedTextDelegateFinished,QString,QString,size_t,QString,QString,QString,bool,bool));
    QObject::connect(delegate, QNSIGNAL(DecryptEncryptedTextDelegate,cancelled),
                     this, QNSLOT(NoteEditorPrivate,onDecryptEncryptedTextDelegateCancelled));
    QObject::connect(delegate, QNSIGNAL(DecryptEncryptedTextDelegate,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onDecryptEncryptedTextDelegateError,ErrorString));

    delegate->start();
}

void NoteEditorPrivate::hideDecryptedTextUnderCursor()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::hideDecryptedTextUnderCursor"));

    if (Q_UNLIKELY(m_currentContextMenuExtraData.m_contentType != QStringLiteral("GenericText"))) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't hide the decrypted text under cursor: wrong current context menu extra data's content type"));
        error.details() = m_currentContextMenuExtraData.m_contentType;
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    if (Q_UNLIKELY(!m_currentContextMenuExtraData.m_insideDecryptedText)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't hide the decrypted text under cursor: the cursor doesn't appear to be inside the decrypted text area"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    hideDecryptedText(m_currentContextMenuExtraData.m_encryptedText, m_currentContextMenuExtraData.m_decryptedText,
                      m_currentContextMenuExtraData.m_cipher, m_currentContextMenuExtraData.m_keyLength,
                      m_currentContextMenuExtraData.m_hint, m_currentContextMenuExtraData.m_id);

    m_currentContextMenuExtraData.m_contentType.resize(0);
}

void NoteEditorPrivate::hideDecryptedText(QString encryptedText, QString decryptedText, QString cipher, QString keyLength,
                                          QString hint, QString id)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::hideDecryptedText"));

    bool conversionResult = false;
    size_t keyLengthInt = static_cast<size_t>(keyLength.toInt(&conversionResult));
    if (Q_UNLIKELY(!conversionResult)) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't hide the decrypted text: can't convert the key length attribute to an integer"));
        error.details() = keyLength;
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool rememberForSession = false;
    QString originalDecryptedText;
    bool foundOriginalDecryptedText = m_decryptedTextManager->findDecryptedTextByEncryptedText(encryptedText, originalDecryptedText,
                                                                                               rememberForSession);
    if (foundOriginalDecryptedText && (decryptedText != originalDecryptedText))
    {
        QNDEBUG(QStringLiteral("The original decrypted text doesn't match the newer one, will return-encrypt the decrypted text"));
        QString newEncryptedText;
        bool reEncryptedText = m_decryptedTextManager->modifyDecryptedText(encryptedText, decryptedText, newEncryptedText);
        if (Q_UNLIKELY(!reEncryptedText)) {
            ErrorString error(QT_TRANSLATE_NOOP("", "can't hide the decrypted text: the decrypted text was modified but it failed to get return-encrypted"));
            error.details() = keyLength;
            QNWARNING(error);
            emit notifyError(error);
            return;
        }

        QNDEBUG(QStringLiteral("Old encrypted text = ") << encryptedText << QStringLiteral(", new encrypted text = ") << newEncryptedText);
        encryptedText = newEncryptedText;
    }

    quint64 enCryptIndex = m_lastFreeEnCryptIdNumber++;
    QString html = ENMLConverter::encryptedTextHtml(encryptedText, hint, cipher, keyLengthInt, enCryptIndex);
    ENMLConverter::escapeString(html);

    QString javascript = QStringLiteral("encryptDecryptManager.replaceDecryptedTextWithEncryptedText('") + id + QStringLiteral("', '") +
                         html + QStringLiteral("');");
    GET_PAGE()
    page->executeJavaScript(javascript, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onHideDecryptedTextFinished));
}

void NoteEditorPrivate::editHyperlinkDialog()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::editHyperlinkDialog"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't edit the hyperlink"))

    // NOTE: when adding the new hyperlink, the selected html can be empty, it's ok
    m_lastSelectedHtmlForHyperlink = m_lastSelectedHtml;

    QString javascript = QStringLiteral("hyperlinkManager.findSelectedHyperlinkId();");
    GET_PAGE()
    page->executeJavaScript(javascript, NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onFoundSelectedHyperlinkId));
}

void NoteEditorPrivate::copyHyperlink()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::copyHyperlink"));

    GET_PAGE()
    page->executeJavaScript(QStringLiteral("hyperlinkManager.getSelectedHyperlinkData();"),
                            NoteEditorCallbackFunctor<QVariant>(this, &NoteEditorPrivate::onFoundHyperlinkToCopy));
}

void NoteEditorPrivate::removeHyperlink()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::removeHyperlink"));

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't remove the hyperlink"))

    RemoveHyperlinkDelegate * delegate = new RemoveHyperlinkDelegate(*this);
    QObject::connect(delegate, QNSIGNAL(RemoveHyperlinkDelegate,finished),
                     this, QNSLOT(NoteEditorPrivate,onRemoveHyperlinkDelegateFinished));
    QObject::connect(delegate, QNSIGNAL(RemoveHyperlinkDelegate,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onRemoveHyperlinkDelegateError,ErrorString));
    delegate->start();
}

void NoteEditorPrivate::onNoteLoadCancelled()
{
    stop();
    QNINFO(QStringLiteral("Note load has been cancelled"));
    // TODO: add some overlay widget for NoteEditor to properly indicate visually that the note load has been cancelled
}

void NoteEditorPrivate::onTableResized()
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onTableResized"));
    convertToNote();
}

void NoteEditorPrivate::onFoundSelectedHyperlinkId(const QVariant & hyperlinkData,
                                                   const QVector<QPair<QString, QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onFoundSelectedHyperlinkId: ") << hyperlinkData);
    Q_UNUSED(extraData)

    QMap<QString,QVariant> resultMap = hyperlinkData.toMap();

    auto statusIt = resultMap.find(QStringLiteral("status"));
    if (Q_UNLIKELY(statusIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the result of the attempt to find the hyperlink data by id from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    bool res = statusIt.value().toBool();
    if (!res)
    {
        QNTRACE(QStringLiteral("No hyperlink id under cursor was found, assuming we're adding the new hyperlink to the selected text"));

        GET_PAGE()

        quint64 hyperlinkId = m_lastFreeHyperlinkIdNumber++;
        setupAddHyperlinkDelegate(hyperlinkId);
        return;
    }

    auto dataIt = resultMap.find(QStringLiteral("data"));
    if (Q_UNLIKELY(dataIt == resultMap.end())) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't parse the seemingly positive result of the attempt "
                                            "to find the hyperlink data by id from JavaScript"));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QString hyperlinkDataStr = dataIt.value().toString();

    bool conversionResult = false;
    quint64 hyperlinkId = hyperlinkDataStr.toULongLong(&conversionResult);
    if (!conversionResult) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't add or edit hyperlink under cursor: can't convert hyperlink id number to unsigned int"));
        error.details() = hyperlinkDataStr;
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QNTRACE(QStringLiteral("Will edit the hyperlink with id ") << hyperlinkId);
    EditHyperlinkDelegate * delegate = new EditHyperlinkDelegate(*this, hyperlinkId);
    QObject::connect(delegate, QNSIGNAL(EditHyperlinkDelegate,finished),
                     this, QNSLOT(NoteEditorPrivate,onEditHyperlinkDelegateFinished));
    QObject::connect(delegate, QNSIGNAL(EditHyperlinkDelegate,cancelled), this, QNSLOT(NoteEditorPrivate,onEditHyperlinkDelegateCancelled));
    QObject::connect(delegate, QNSIGNAL(EditHyperlinkDelegate,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onEditHyperlinkDelegateError,ErrorString));
    delegate->start();
}

void NoteEditorPrivate::onFoundHyperlinkToCopy(const QVariant & hyperlinkData,
                                               const QVector<QPair<QString, QString> > & extraData)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::onFoundHyperlinkToCopy: ") << hyperlinkData);
    Q_UNUSED(extraData);

    QStringList hyperlinkDataList = hyperlinkData.toStringList();
    if (hyperlinkDataList.isEmpty()) {
        QNTRACE(QStringLiteral("Hyperlink data to copy was not found"));
        return;
    }

    if (hyperlinkDataList.size() != 3) {
        ErrorString error(QT_TRANSLATE_NOOP("", "can't copy the hyperlink: can't get text and hyperlink from JavaScript"));
        error.details() = hyperlinkDataList.join(QStringLiteral(","));
        QNWARNING(error);
        emit notifyError(error);
        return;
    }

    QClipboard * pClipboard = QApplication::clipboard();
    if (Q_UNLIKELY(!pClipboard)) {
        QNWARNING(QStringLiteral("Unable to get window system clipboard"));
    }
    else {
        pClipboard->setText(hyperlinkDataList[1]);
    }
}

void NoteEditorPrivate::dropFile(const QString & filePath)
{
    QNDEBUG(QStringLiteral("NoteEditorPrivate::dropFile: ") << filePath);

    CHECK_NOTE_EDITABLE(QT_TRANSLATE_NOOP("", "Can't add the attachment via drag'n'drop"))

    AddResourceDelegate * delegate = new AddResourceDelegate(filePath, *this, m_pResourceFileStorageManager,
                                                             m_pFileIOThreadWorker, m_pGenericResourceImageManager,
                                                             m_genericResourceImageFilePathsByResourceHash);

    QObject::connect(delegate, QNSIGNAL(AddResourceDelegate,finished,Resource,QString),
                     this, QNSLOT(NoteEditorPrivate,onAddResourceDelegateFinished,Resource,QString));
    QObject::connect(delegate, QNSIGNAL(AddResourceDelegate,notifyError,ErrorString),
                     this, QNSLOT(NoteEditorPrivate,onAddResourceDelegateError,ErrorString));

    delegate->start();
}

} // namespace quentier

void initNoteEditorResources()
{
    Q_INIT_RESOURCE(underline);
    Q_INIT_RESOURCE(css);
    Q_INIT_RESOURCE(checkbox_icons);
    Q_INIT_RESOURCE(encrypted_area_icons);
    Q_INIT_RESOURCE(generic_resource_icons);
    Q_INIT_RESOURCE(jquery);
    Q_INIT_RESOURCE(colResizable);
    Q_INIT_RESOURCE(debounce);
    Q_INIT_RESOURCE(rangy);
    Q_INIT_RESOURCE(scripts);
    Q_INIT_RESOURCE(hilitor);

    QNDEBUG(QStringLiteral("Initialized NoteEditor's resources"));
}
