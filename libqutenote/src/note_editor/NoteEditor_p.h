#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_P_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_P_H

#include "ResourceInfo.h"
#include "NoteEditorPage.h"
#include "undo_stack/PreliminaryUndoCommandQueue.h"
#include <qute_note/note_editor/INoteEditorBackend.h>
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/enml/ENMLConverter.h>
#include <qute_note/utility/EncryptionManager.h>
#include <qute_note/utility/LimitedStack.h>
#include <QObject>
#include <QMimeType>
#include <QFont>
#include <QColor>
#include <QImage>

#ifdef USE_QT_WEB_ENGINE
#include <QWebEngineView>
typedef QWebEngineView WebView;
typedef QWebEnginePage WebPage;
#else
#include "NoteEditorPluginFactory.h"
#include <QWebView>
typedef QWebView WebView;
typedef QWebPage WebPage;
#endif

QT_FORWARD_DECLARE_CLASS(QByteArray)
QT_FORWARD_DECLARE_CLASS(QMimeType)
QT_FORWARD_DECLARE_CLASS(QImage)
QT_FORWARD_DECLARE_CLASS(QThread)

#ifdef USE_QT_WEB_ENGINE
QT_FORWARD_DECLARE_CLASS(QWebChannel)
QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(WebSocketClientWrapper)
#endif

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ResourceInfoJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)
QT_FORWARD_DECLARE_CLASS(TextCursorPositionJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(ContextMenuEventJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(PageMutationHandler)
QT_FORWARD_DECLARE_CLASS(ToDoCheckboxOnClickHandler)

#ifdef USE_QT_WEB_ENGINE
QT_FORWARD_DECLARE_CLASS(EnCryptElementOnClickHandler)
QT_FORWARD_DECLARE_CLASS(GenericResourceOpenAndSaveButtonsOnClickHandler)
QT_FORWARD_DECLARE_CLASS(GenericResourceImageWriter)
QT_FORWARD_DECLARE_CLASS(GenericResourceImageJavaScriptHandler)
QT_FORWARD_DECLARE_CLASS(HyperlinkClickJavaScriptHandler)
#endif

class NoteEditorPrivate: public WebView, public INoteEditorBackend
{
    Q_OBJECT
public:
    explicit NoteEditorPrivate(NoteEditor & noteEditor);
    virtual ~NoteEditorPrivate();

#ifndef USE_QT_WEB_ENGINE
    QVariant execJavascriptCommandWithResult(const QString & command);
    QVariant execJavascriptCommandWithResult(const QString & command, const QString & args);
#endif

    void execJavascriptCommand(const QString & command, NoteEditorPage::Callback callback = 0);
    void execJavascriptCommand(const QString & command, const QString & args, NoteEditorPage::Callback = 0);

Q_SIGNALS:
    void contentChanged();
    void notifyError(QString error);

    void convertedToNote(Note note);
    void cantConvertToNote(QString error);

    void noteEditorHtmlUpdated(QString html);

    // Signals to notify anyone interested of the formatting at the current cursor position
    void textBoldState(bool state);
    void textItalicState(bool state);
    void textUnderlineState(bool state);
    void textStrikethroughState(bool state);
    void textAlignLeftState(bool state);
    void textAlignCenterState(bool state);
    void textAlignRightState(bool state);
    void textInsideOrderedListState(bool state);
    void textInsideUnorderedListState(bool state);
    void textInsideTableState(bool state);

    void textFontFamilyChanged(QString fontFamily);
    void textFontSizeChanged(int fontSize);

    void insertTableDialogRequested();

public:
    // Force the conversion from ENML to HTML
    void updateFromNote();

    // Resets the note's HTML to the given one
    void setNoteHtml(const QString & html);

    const ResourceWrapper attachResourceToNote(const QByteArray & data, const QByteArray & dataHash,
                                               const QMimeType & mimeType, const QString & filename);
    void addResourceToNote(const ResourceWrapper & resource);
    void removeResourceFromNote(const ResourceWrapper & resource);
    void replaceResourceInNote(const ResourceWrapper & resource);
    void setNoteResources(const QList<ResourceWrapper> & resources);


    QImage buildGenericResourceImage(const IResource & resource);
    void saveGenericResourceImage(const IResource & resource, const QImage & image);
    void provideSrcForGenericResourceImages();
    void setupGenericResourceOnClickHandler();

    void switchEditorPage(const bool shouldConvertFromNote = true);
    void popEditorPage();

    void skipPushingUndoCommandOnNextContentChange();
    void skipNextContentChange();

    void setNotePageHtmlAfterEncryption(const QString & html);
    void undoLastEncryption();

    void setNotePageHtmlAfterAddingHyperlink(const QString & html);

    void replaceHyperlinkContent(const quint64 hyperlinkId, const QString & link, const QString & text);

    bool isModified() const;

    const QString & noteEditorPagePath() const { return m_noteEditorPagePath; }
    const QString & imageResourcesStoragePath() const { return m_noteEditorImageResourcesStoragePath; }
    const QString & resourceLocalFileStoragePath() const { return m_resourceLocalFileStorageFolder; }

    void onDropEvent(QDropEvent * pEvent);
    void dropFile(QString & filepath);

public Q_SLOTS:
    virtual QObject * object() Q_DECL_OVERRIDE { return this; }
    virtual QWidget * widget() Q_DECL_OVERRIDE { return this; }
    virtual void setUndoStack(QUndoStack * pUndoStack) Q_DECL_OVERRIDE;

    virtual void undo() Q_DECL_OVERRIDE;
    virtual void redo() Q_DECL_OVERRIDE;
    virtual void cut() Q_DECL_OVERRIDE;
    virtual void copy() Q_DECL_OVERRIDE;
    virtual void paste() Q_DECL_OVERRIDE;
    virtual void pasteUnformatted() Q_DECL_OVERRIDE;
    virtual void selectAll() Q_DECL_OVERRIDE;
    virtual void fontMenu() Q_DECL_OVERRIDE;
    virtual void textBold() Q_DECL_OVERRIDE;
    virtual void textItalic() Q_DECL_OVERRIDE;
    virtual void textUnderline() Q_DECL_OVERRIDE;
    virtual void textStrikethrough() Q_DECL_OVERRIDE;
    virtual void textHighlight() Q_DECL_OVERRIDE;
    virtual void alignLeft() Q_DECL_OVERRIDE;
    virtual void alignCenter() Q_DECL_OVERRIDE;
    virtual void alignRight() Q_DECL_OVERRIDE;
    virtual void insertToDoCheckbox() Q_DECL_OVERRIDE;
    virtual void setSpellcheck(const bool enabled) Q_DECL_OVERRIDE;
    virtual void setFont(const QFont & font) Q_DECL_OVERRIDE;
    virtual void setFontHeight(const int height) Q_DECL_OVERRIDE;
    virtual void setFontColor(const QColor & color) Q_DECL_OVERRIDE;
    virtual void setBackgroundColor(const QColor & color) Q_DECL_OVERRIDE;
    virtual void insertHorizontalLine() Q_DECL_OVERRIDE;
    virtual void increaseFontSize() Q_DECL_OVERRIDE;
    virtual void decreaseFontSize() Q_DECL_OVERRIDE;
    virtual void increaseIndentation() Q_DECL_OVERRIDE;
    virtual void decreaseIndentation() Q_DECL_OVERRIDE;
    virtual void insertBulletedList() Q_DECL_OVERRIDE;
    virtual void insertNumberedList() Q_DECL_OVERRIDE;
    virtual void insertTableDialog() Q_DECL_OVERRIDE;
    virtual void insertFixedWidthTable(const int rows, const int columns, const int widthInPixels) Q_DECL_OVERRIDE;
    virtual void insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth) Q_DECL_OVERRIDE;
    virtual void addAttachmentDialog() Q_DECL_OVERRIDE;
    virtual void saveAttachmentDialog(const QString & resourceHash) Q_DECL_OVERRIDE;
    virtual void saveAttachmentUnderCursor() Q_DECL_OVERRIDE;
    virtual void openAttachment(const QString & resourceHash) Q_DECL_OVERRIDE;
    virtual void openAttachmentUnderCursor() Q_DECL_OVERRIDE;
    virtual void copyAttachment(const QString & resourceHash) Q_DECL_OVERRIDE;
    virtual void copyAttachmentUnderCursor() Q_DECL_OVERRIDE;

    virtual void encryptSelectedTextDialog() Q_DECL_OVERRIDE;
    void doEncryptSelectedTextDialog(bool * pCancelled = Q_NULLPTR);

    virtual void decryptEncryptedTextUnderCursor() Q_DECL_OVERRIDE;

    virtual void editHyperlinkDialog() Q_DECL_OVERRIDE;
    virtual void copyHyperlink() Q_DECL_OVERRIDE;
    virtual void removeHyperlink() Q_DECL_OVERRIDE;

    void doRemoveHyperlink(const bool shouldTrackDelegate, const quint64 hyperlinkIdToRemove);

    virtual void onNoteLoadCancelled() Q_DECL_OVERRIDE;

    virtual void setNoteAndNotebook(const Note & note, const Notebook & notebook) Q_DECL_OVERRIDE;
    virtual void convertToNote() Q_DECL_OVERRIDE;

    void undoPageAction();
    void redoPageAction();

    void flipEnToDoCheckboxState(const quint64 enToDoIdNumber);

    void onEncryptedAreaDecryption(QString cipher, size_t keyLength, QString encryptedText,
                                   QString passphrase, QString decryptedText,
                                   bool rememberForSession, bool decryptPermanently,
                                   bool createDecryptUndoCommand = true);

// private signals:
Q_SIGNALS:
    void saveResourceToStorage(QString localGuid, QByteArray data, QByteArray dataHash,
                               QString fileStoragePath, QUuid requestId);
    void readDroppedFileData(QString absoluteFilePath, QUuid requestId);
    void writeNoteHtmlToFile(QString absoluteFilePath, QByteArray html, QUuid requestId);
    void writeImageResourceToFile(QString absoluteFilePath, QByteArray imageData, QUuid requestId);
    void saveResourceToFile(QString absoluteFilePath, QByteArray resourceData, QUuid requestId);
#ifdef USE_QT_WEB_ENGINE
    void saveGenericResourceImageToFile(const QString resourceLocalGuid, const QByteArray resourceImageData,
                                        const QString resourceFileSuffix, const QByteArray resourceActualHash,
                                        const QString resourceDisplayName, const QUuid requestId);
#endif

private Q_SLOTS:
    void onFoundSelectedHyperlinkId(const QVariant & hyperlinkData,
                                    const QVector<QPair<QString, QString> > & extraData);
    void onFoundHyperlinkToCopy(const QVariant & hyperlinkData,
                                const QVector<QPair<QString, QString> > & extraData);

    void onSelectedTextEncryption(QString selectedText, QString encryptedText,
                                  QString passphrase, QString cipher, size_t keyLength,
                                  QString hint, bool rememberForSession);
    void onNoteLoadFinished(bool ok);
    void onContentChanged();

    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash, QString fileStoragePath,
                                  int errorCode, QString errorDescription);
    void onDroppedFileRead(bool success, QString errorDescription, QByteArray data, QUuid requestId);

#ifdef USE_QT_WEB_ENGINE
    void onGenericResourceImageSaved(const bool success, const QByteArray resourceActualHash,
                                     const QString filePath, const QString errorDescription,
                                     const QUuid requestId);

    void onHyperlinkClicked(QString url);
#else
    void onHyperlinkClicked(QUrl url);
#endif

    void onToDoCheckboxClicked(quint64 enToDoCheckboxId);
    void onToDoCheckboxClickHandlerError(QString error);

    void onJavaScriptLoaded();

    void onOpenResourceRequest(const QString & resourceHash);
    void onSaveResourceRequest(const QString & resourceHash);

    void onEnCryptElementClicked(QString encryptedText, QString cipher, QString length,
                                 QString hint, bool * pCancelled = Q_NULLPTR);

    void contextMenuEvent(QContextMenuEvent * pEvent) Q_DECL_OVERRIDE;
    void onContextMenuEventReply(QString contentType, QString selectedHtml, bool insideDecryptedTextFragment,
                                 QStringList extraData, quint64 sequenceNumber);

    void onTextCursorPositionChange();

    void onTextCursorBoldStateChanged(bool state);
    void onTextCursorItalicStateChanged(bool state);
    void onTextCursorUnderlineStateChanged(bool state);
    void onTextCursorStrikethgouthStateChanged(bool state);
    void onTextCursorAlignLeftStateChanged(bool state);
    void onTextCursorAlignCenterStateChanged(bool state);
    void onTextCursorAlignRightStateChanged(bool state);
    void onTextCursorInsideOrderedListStateChanged(bool state);
    void onTextCursorInsideUnorderedListStateChanged(bool state);
    void onTextCursorInsideTableStateChanged(bool state);

    void onTextCursorOnImageResourceStateChanged(bool state, QString resourceHash);
    void onTextCursorOnNonImageResourceStateChanged(bool state, QString resourceHash);
    void onTextCursorOnEnCryptTagStateChanged(bool state, QString encryptedText, QString cipher, QString length);

    void onTextCursorFontNameChanged(QString fontName);
    void onTextCursorFontSizeChanged(int fontSize);

    void onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);

    // Slots for delegates
    void onEncryptSelectedTextDelegateFinished();
    void onEncryptSelectedTextDelegateError(QString error);

    void onAddHyperlinkToSelectedTextDelegateFinished();
    void onAddHyperlinkToSelectedTextDelegateCancelled();
    void onAddHyperlinkToSelectedTextDelegateError(QString error);

    void onEditHyperlinkDelegateFinished(quint64 hyperlinkId, QString previousText, QString previousUrl, QString newText, QString newUrl);
    void onEditHyperlinkDelegateCancelled();
    void onEditHyperlinkDelegateError(QString error);

    void onRemoveHyperlinkDelegateFinished(quint64 removedHyperlinkId);
    void onRemoveHyperlinkDelegateError(QString error);

private:
    // Helper methods for undo stack
    void pushNoteContentEditUndoCommand();
    void pushDecryptUndoCommand(const QString & cipher, const size_t keyLength,
                                const QString & encryptedText, const QString & decryptedText,
                                const QString & passphrase, const bool rememberForSession,
                                const bool decryptPermanently);

private:
    template <typename T>
    QString composeHtmlTable(const T width, const T singleColumnWidth, const int rows,
                             const int columns, const bool relative);

    void changeFontSize(const bool increase);
    void changeIndentation(const bool increase);

    void replaceSelectedTextWithEncryptedOrDecryptedText(const QString & selectedText, const QString & encryptedText,
                                                         const QString & hint, const bool rememberForSession);

    void clearEditorContent();
    void noteToEditorContent();
    void updateColResizableTableBindings();

    bool htmlToNoteContent(QString & errorDescription);

    void saveNoteResourcesToLocalFiles();
    void provideSrcForResourceImgTags();

    void manualSaveResourceToFile(const IResource & resource);
    void openResource(const QString & resourceAbsoluteFilePath);

#ifdef USE_QT_WEB_ENGINE
    void provideSrcAndOnClickScriptForImgEnCryptTags();

    void setupGenericResourceImages();

    // Returns true if the resource image gets built and is being saved to a file asynchronously
    bool findOrBuildGenericResourceImage(const IResource & resource);

    void setupWebSocketServer();
    void setupJavaScriptObjects();
    void setupTextCursorPositionTracking();
#endif

    void setupGenericTextContextMenu(const QString &selectedHtml, bool insideDecryptedTextFragment);
    void setupImageResourceContextMenu(const QString & resourceHash);
    void setupNonImageResourceContextMenu();
    void setupEncryptedTextContextMenu(const QString & cipher, const QString & keyLength,
                                       const QString & encryptedText, const QString & hint);

    void setupActionShortcut(const int key, const QString & context, QAction & action);

    void setupFileIO();
    void setupScripts();
    void setupGeneralSignalSlotConnections();
    void setupNoteEditorPage();
    void setupNoteEditorPageConnections(NoteEditorPage * page);
    void setupTextCursorPositionJavaScriptHandlerConnections();
    void setupSkipRulesForHtmlToEnmlConversion();

    void determineStatesForCurrentTextCursorPosition();
    void determineContextMenuEventTarget();

    bool isPageEditable() const { return m_isPageEditable; }
    void setPageEditable(const bool editable);

    bool checkContextMenuSequenceNumber(const quint64 sequenceNumber) const;

    void onPageHtmlReceived(const QString & html, const QVector<QPair<QString,QString> > & extraData = QVector<QPair<QString,QString> >());
    void onSelectedTextEncryptionDone(const QVariant & dummy, const QVector<QPair<QString,QString> > & extraData);

    int resourceIndexByHash(const QList<ResourceAdapter> & resourceAdapters,
                            const QString & resourceHash) const;

    void updateNoteEditorPagePath(const quint32 index);

private:
    // Overrides for some Qt's virtual methods
    virtual void timerEvent(QTimerEvent * pEvent) Q_DECL_OVERRIDE;

    virtual void dragMoveEvent(QDragMoveEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent * pEvent) Q_DECL_OVERRIDE;

private:
    template <class T>
    class NoteEditorCallbackFunctor
    {
    public:
        NoteEditorCallbackFunctor(NoteEditorPrivate * editor,
                                  void (NoteEditorPrivate::* method)(const T & result,
                                                                     const QVector<QPair<QString,QString> > & extraData),
                                  const QVector<QPair<QString,QString> > & extraData = QVector<QPair<QString,QString> >()) :
            m_editor(editor), m_method(method), m_extraData(extraData)
        {}

        NoteEditorCallbackFunctor(const NoteEditorCallbackFunctor<T> & other) : m_editor(other.m_editor), m_method(other.m_method), m_extraData(other.m_extraData) {}
        NoteEditorCallbackFunctor & operator=(const NoteEditorCallbackFunctor<T> & other)
        { if (this != &other) { m_editor = other.m_editor; m_method = other.m_method; m_extraData = other.m_extraData; } return *this; }

        void operator()(const T & result) { (m_editor->*m_method)(result, m_extraData); }

    private:
        NoteEditorPrivate * m_editor;
        void (NoteEditorPrivate::* m_method)(const T & result, const QVector<QPair<QString,QString> > & extraData);
        QVector<QPair<QString, QString> > m_extraData;
    };

    friend class NoteEditorCallbackFunctor<QString>;
    friend class NoteEditorCallbackFunctor<QVariant>;

    struct Alignment
    {
        enum type {
            Left = 0,
            Center,
            Right
        };
    };

    struct TextFormattingState
    {
        TextFormattingState() :
            m_bold(false),
            m_italic(false),
            m_underline(false),
            m_strikethrough(false),
            m_alignment(Alignment::Left),
            m_insideOrderedList(false),
            m_insideUnorderedList(false),
            m_insideTable(false),
            m_onImageResource(false),
            m_onNonImageResource(false),
            m_resourceHash(),
            m_onEnCryptTag(false),
            m_encryptedText(),
            m_cipher(),
            m_length()
        {}

        bool m_bold;
        bool m_italic;
        bool m_underline;
        bool m_strikethrough;

        Alignment::type m_alignment;

        bool m_insideOrderedList;
        bool m_insideUnorderedList;
        bool m_insideTable;

        bool m_onImageResource;
        bool m_onNonImageResource;
        QString m_resourceHash;

        bool m_onEnCryptTag;
        QString m_encryptedText;
        QString m_cipher;
        QString m_length;
    };

    // Holds some data required for certain context menu actions, like the encrypted text data for its decryption,
    // the hash of the resource under cursor for which the action is toggled etc.
    struct CurrentContextMenuExtraData
    {
        QString     m_contentType;

        // Encrypted text extra data
        QString     m_encryptedText;
        QString     m_keyLength;
        QString     m_cipher;
        QString     m_hint;

        // Resource extra data
        QString     m_resourceHash;
    };

private:
    QString     m_noteEditorPageFolderPath;
    QString     m_noteEditorPagePath;
    QString     m_noteEditorImageResourcesStoragePath;

    QFont       m_font;

    // JavaScript scripts
    QString     m_jQueryJs;
    QString     m_resizableTableColumnsJs;
    QString     m_debounceJs;
    QString     m_onFixedWidthTableResizeJs;
    QString     m_getSelectionHtmlJs;
    QString     m_snapSelectionToWordJs;
    QString     m_replaceSelectionWithHtmlJs;
    QString     m_findSelectedHyperlinkElementJs;
    QString     m_findSelectedHyperlinkIdJs;
    QString     m_setHyperlinkToSelectionJs;
    QString     m_getHyperlinkFromSelectionJs;
    QString     m_getHyperlinkDataJs;
    QString     m_removeHyperlinkJs;
    QString     m_replaceHyperlinkContentJs;
    QString     m_provideSrcForResourceImgTagsJs;
    QString     m_setupEnToDoTagsJs;
    QString     m_flipEnToDoCheckboxStateJs;
    QString     m_onResourceInfoReceivedJs;
    QString     m_determineStatesForCurrentTextCursorPositionJs;
    QString     m_determineContextMenuEventTargetJs;
    QString     m_changeFontSizeForSelectionJs;
    QString     m_decryptEncryptedTextPermanentlyJs;
    QString     m_pageMutationObserverJs;

#ifndef USE_QT_WEB_ENGINE
    QString     m_qWebKitSetupJs;
#else
    QString     m_provideSrcForGenericResourceImagesJs;
    QString     m_onGenericResourceImageReceivedJs;
    QString     m_provideSrcAndOnClickScriptForEnCryptImgTagsJs;
    QString     m_qWebChannelJs;
    QString     m_qWebChannelSetupJs;
    QString     m_notifyTextCursorPositionChangedJs;
    QString     m_setupTextCursorPositionTrackingJs;
    QString     m_genericResourceOnClickHandlerJs;
    QString     m_setupGenericResourceOnClickHandlerJs;
    QString     m_clickInterceptorJs;

    QWebSocketServer * m_pWebSocketServer;
    WebSocketClientWrapper * m_pWebSocketClientWrapper;
    QWebChannel * m_pWebChannel;
    EnCryptElementOnClickHandler * m_pEnCryptElementClickHandler;
    GenericResourceOpenAndSaveButtonsOnClickHandler * m_pGenericResourceOpenAndSaveButtonsOnClickHandler;
    GenericResourceImageWriter * m_pGenericResourceImageWriter;
    HyperlinkClickJavaScriptHandler * m_pHyperlinkClickJavaScriptHandler;

    quint16     m_webSocketServerPort;
#endif

    ToDoCheckboxOnClickHandler * m_pToDoCheckboxClickHandler;
    PageMutationHandler * m_pPageMutationHandler;

    QUndoStack * m_pUndoStack;
    PreliminaryUndoCommandQueue * m_pPreliminaryUndoCommandQueue;

    quint64     m_contextMenuSequenceNumber;
    QPoint      m_lastContextMenuEventGlobalPos;
    QPoint      m_lastContextMenuEventPagePos;
    ContextMenuEventJavaScriptHandler * m_pContextMenuEventJavaScriptHandler;

    TextCursorPositionJavaScriptHandler * m_pTextCursorPositionJavaScriptHandler;

    TextFormattingState     m_currentTextFormattingState;

    QUuid       m_writeNoteHtmlToFileRequestId;

    bool        m_isPageEditable;
    bool        m_pendingConversionToNote;
    bool        m_pendingNotePageLoad;
    bool        m_pendingIndexHtmlWritingToFile;
    bool        m_pendingJavaScriptExecution;

    bool        m_skipPushingUndoCommandOnNextContentChange;
    bool        m_skipNextContentChange;

    Note *      m_pNote;
    Notebook *  m_pNotebook;

    bool        m_modified;

    // These two bools implement a cheap scheme of watching
    // for changes in note editor since some particular moment in time.
    // For example, the conversion of note from HTML into ENML happens
    // in the background mode, when the editor is idle for at least N seconds.
    // How can such idle state be determined? Create a timer for N seconds,
    // as it begins, set m_watchingForContentChange to true and
    // m_contentChangedSinceWatchingStart to false. On every next content change
    // m_contentChangedSinceWatchingStart would be set to true. When the timer ends,
    // it can check the state of m_contentChangedSinceWatchingStart.
    // If it's true, it means the editing is still in progress and it's not nice
    // to block the GUI thread by HTML to ENML conversion. So drop this
    // variable into false again and wait for another N seconds. And only
    // if there were no further edits during N seconds, convert note editor's page
    // to ENML
    bool        m_watchingForContentChange;
    bool        m_contentChangedSinceWatchingStart;

    int         m_secondsToWaitBeforeConversionStart;

    int         m_pageToNoteContentPostponeTimerId;

    QSharedPointer<EncryptionManager>       m_encryptionManager;
    DecryptedTextManager                    m_decryptedTextManager;

    ENMLConverter                           m_enmlConverter;

#ifndef USE_QT_WEB_ENGINE
    NoteEditorPluginFactory *               m_pluginFactory;
#endif

    QMenu *                                 m_pGenericTextContextMenu;
    QMenu *                                 m_pImageResourceContextMenu;
    QMenu *                                 m_pNonImageResourceContextMenu;
    QMenu *                                 m_pEncryptedTextContextMenu;

    const QString     m_pagePrefix;

    QString     m_lastSelectedHtml;
    QString     m_lastSelectedHtmlForEncryption;
    QString     m_lastSelectedHtmlForHyperlink;

    QString     m_enmlCachedMemory;   // Cached memory for HTML to ENML conversions
    QString     m_htmlCachedMemory;   // Cached memory for ENML from Note -> HTML conversions
    QString     m_errorCachedMemory;  // Cached memory for various errors

    QVector<ENMLConverter::SkipHtmlElementRule>     m_skipRulesForHtmlToEnmlConversion;

    QThread *   m_pIOThread;
    ResourceFileStorageManager *    m_pResourceFileStorageManager;
    FileIOThreadWorker *            m_pFileIOThreadWorker;

    ResourceInfo                    m_resourceInfo;
    ResourceInfoJavaScriptHandler * m_pResourceInfoJavaScriptHandler;

    QString                         m_resourceLocalFileStorageFolder;

    QHash<QUuid, QString>           m_genericResourceLocalGuidBySaveToStorageRequestIds;
    QHash<QString, QString>         m_resourceFileStoragePathsByResourceLocalGuid;

    QSet<QString>                   m_localGuidsOfResourcesWantedToBeSaved;
    QSet<QString>                   m_localGuidsOfResourcesWantedToBeOpened;

    QSet<QUuid>                     m_manualSaveResourceToFileRequestIds;

    QHash<QString, QStringList>     m_fileSuffixesForMimeType;
    QHash<QString, QString>         m_fileFilterStringForMimeType;

#ifdef USE_QT_WEB_ENGINE
    QHash<QByteArray, QString>      m_genericResourceImageFilePathsByResourceHash;
    GenericResourceImageJavaScriptHandler *  m_pGenericResoureImageJavaScriptHandler;

    QSet<QUuid>                     m_saveGenericResourceImageToFileRequestIds;
#endif

    CurrentContextMenuExtraData     m_currentContextMenuExtraData;
    QHash<QUuid, QPair<QString, QMimeType> >   m_droppedFilePathsAndMimeTypesByReadRequestIds;

    quint64     m_lastFreeEnToDoIdNumber;
    quint64     m_lastFreeHyperlinkIdNumber;
    quint64     m_lastFreeEnCryptIdNumber;
    quint64     m_lastFreeEnDecryptedIdNumber;

    QString     m_lastEncryptedText;

    LimitedStack<NoteEditorPage*>    m_pagesStack;
    quint32      m_lastNoteEditorPageFreeIndex;

    NoteEditor * const q_ptr;
    Q_DECLARE_PUBLIC(NoteEditor)
};

} // namespace qute_note

void __initNoteEditorResources();

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_P_H
