#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_P_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_P_H

#include "ResourceInfo.h"
#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/enml/ENMLConverter.h>
#include <qute_note/utility/EncryptionManager.h>
#include <QObject>
#include <QMimeType>
#include <QFont>
#include <QColor>

#ifdef USE_QT_WEB_ENGINE
#include <QImage>
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

#ifdef USE_QT_WEB_ENGINE
QT_FORWARD_DECLARE_CLASS(PageMutationHandler)
QT_FORWARD_DECLARE_CLASS(EnCryptElementOnClickHandler)
QT_FORWARD_DECLARE_CLASS(GenericResourceOpenAndSaveButtonsOnClickHandler)
QT_FORWARD_DECLARE_CLASS(GenericResourceImageWriter)
QT_FORWARD_DECLARE_CLASS(GenericResourceImageJavaScriptHandler)
#endif

class NoteEditorPrivate: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorPrivate(NoteEditor & noteEditor);
    virtual ~NoteEditorPrivate();

#ifndef USE_QT_WEB_ENGINE
    QVariant execJavascriptCommandWithResult(const QString & command);
    QVariant execJavascriptCommandWithResult(const QString & command, const QString & args);
#endif

    void execJavascriptCommand(const QString & command);
    void execJavascriptCommand(const QString & command, const QString & args);

    void setNoteAndNotebook(const Note & note, const Notebook & notebook);
    const Note * getNote();
    const Notebook * getNotebook() const;

    void convertToNote();

Q_SIGNALS:
    void convertedToNote(Note note);
    void cantConvertToNote(QString errorDescription);

    void notifyError(QString error);

    void noteEditorHtmlUpdated(QString html);

public:
    bool isModified() const;

#ifndef USE_QT_WEB_ENGINE
    const NoteEditorPluginFactory & pluginFactory() const;
    NoteEditorPluginFactory & pluginFactory();
#endif

    void onDropEvent(QDropEvent * pEvent);
    void dropFile(QString & filepath);

    // Returns the local guid of the new resource
    QString attachResourceToNote(const QByteArray & data, const QByteArray &dataHash,
                                 const QMimeType & mimeType, const QString & filename);

    template <typename T>
    QString composeHtmlTable(const T width, const T singleColumnWidth, const int rows,
                             const int columns, const bool relative);

public:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void pasteUnformatted();
    void selectAll();
    void fontMenu();
    void textBold();
    void textItalic();
    void textUnderline();
    void textStrikethrough();
    void textHighlight();
    void alignLeft();
    void alignCenter();
    void alignRight();
    void insertToDoCheckbox();
    void setSpellcheck(bool enabled);
    void setFont(const QFont & font);
    void setFontHeight(const int height);
    void setFontColor(const QColor & color);
    void setBackgroundColor(const QColor & color);
    void insertHorizontalLine();
    void changeIndentation(const bool increase);
    void insertBulletedList();
    void insertNumberedList();
    void insertTableDialog();
    void insertFixedWidthTable(const int rows, const int columns, const int widthInPixels);
    void insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth);
    void addAttachmentDialog();
    void encryptSelectedTextDialog();
    void encryptSelectedText(const QString & passphrase, const QString & hint);
    void addHyperlinkDialog();
    void editHyperlinkDialog();
    void copyHyperlink();
    void removeHyperlink();

    void onNoteLoadCancelled();

// private signals:
Q_SIGNALS:
    void saveResourceToStorage(QString localGuid, QByteArray data, QByteArray dataHash,
                               QString fileStoragePath, QUuid requestId);
    void readDroppedFileData(QString absoluteFilePath, QUuid requestId);
    void writeNoteHtmlToFile(QString absoluteFilePath, QByteArray html, QUuid requestId);
    void writeImageResourceToFile(QString absoluteFilePath, QByteArray imageData, QUuid requestId);
#ifdef USE_QT_WEB_ENGINE
    void saveResourceToFile(QString absoluteFilePath, QByteArray resourceData, QUuid requestId);
    void saveGenericResourceImageToFile(const QString resourceLocalGuid, const QByteArray resourceImageData,
                                        const QString resourceFileSuffix, const QByteArray resourceActualHash,
                                        const QString resourceDisplayName, const QUuid requestId);
#endif

    void textCursorPositionBoldState(bool state);
    void textCursorPositionItalicState(bool state);
    void textCursorPositionUnderlineState(bool state);
    void textCursorPositionStrikethgouthState(bool state);
    void textCursorPositionAlignLeftState(bool state);
    void textCursorPositionAlignCenterState(bool state);
    void textCursorPositionAlignRightState(bool state);
    void textCursorPositionInsideOrderedListState(bool state);
    void textCursorPositionInsideUnorderedListState(bool state);
    void textCursorPositionInsideTableState(bool state);

private Q_SLOTS:
    void onEncryptedAreaDecryption(QString encryptedText, QString decryptedText, bool rememberForSession);
    void onNoteLoadFinished(bool ok);
    void onContentChanged();

    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash, QString fileStoragePath,
                                  int errorCode, QString errorDescription);
    void onDroppedFileRead(bool success, QString errorDescription, QByteArray data, QUuid requestId);

#ifdef USE_QT_WEB_ENGINE
    void onGenericResourceImageSaved(const bool success, const QByteArray resourceActualHash,
                                     const QString filePath, const QString errorDescription,
                                     const QUuid requestId);

    void onEnCryptElementClicked(QString encryptedText, QString cipher, QString length, QString hint);

    void onOpenResourceButtonClicked(const QString & resourceHash);
    void onSaveResourceButtonClicked(const QString & resourceHash);

    void onJavaScriptLoaded();
#endif

    void contextMenuEvent(QContextMenuEvent * pEvent);
    void onContextMenuEventReply(QString contentType, quint64 sequenceNumber);

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

    void onWriteFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);

private:
    virtual void timerEvent(QTimerEvent * event) Q_DECL_OVERRIDE;

private:
    void clearEditorContent();
    void noteToEditorContent();
    void updateColResizableTableBindings();

    bool htmlToNoteContent(QString & errorDescription);

    void saveNoteResourcesToLocalFiles();
    void provideSrcForResourceImgTags();

#ifdef USE_QT_WEB_ENGINE
    void provideSrcAndOnClickScriptForImgEnCryptTags();

    void manualSaveResourceToFile(const IResource & resource);
    void openResource(const QString & resourceAbsoluteFilePath);

    void setupGenericResourceImages();

    // Returns true if the resource image gets built and is being saved to a file asynchronously
    bool findOrBuildGenericResourceImage(const IResource & resource);
    QImage buildGenericResourceImage(const IResource & resource);
    void saveGenericResourceImage(const IResource & resource, const QImage & image);
    void provideSrcForGenericResourceImages();
    void setupGenericResourceOnClickHandler();

    void setupWebSocketServer();
    void setupJavaScriptObjects();
    void setupTextCursorPositionTracking();
#endif

    void setupGenericTextContextMenu();
    void setupImageResourceContextMenu();
    void setupNonImageResourceContextMenu();
    void setupEncryptedTextContextMenu();
    void setupActionShortcut(const QString & key, QAction & action);

    void setupFileIO();
    void setupScripts();
    void setupNoteEditorPage();
    void setupTextCursorPositionJavaScriptHandlerConnections();

    void determineStatesForCurrentTextCursorPosition();
    void determineContextMenuEventTarget();

    bool isPageEditable() const { return m_isPageEditable; }
    void setPageEditable(const bool editable);

    bool checkContextMenuSequenceNumber(const quint64 sequenceNumber) const;

    void onPageHtmlReceived(const QString & html, const QVector<QPair<QString,QString> > & extraData = QVector<QPair<QString,QString> >());
    void onPageSelectedHtmlForEncryptionReceived(const QVariant & selectedHtmlData,
                                                 const QVector<QPair<QString,QString> > & extraData);

    template <class T>
    class HtmlRetrieveFunctor
    {
    public:
        HtmlRetrieveFunctor(NoteEditorPrivate * editor,
                            void (NoteEditorPrivate::* method)(const T & result,
                                                               const QVector<QPair<QString,QString> > & extraData),
                            const QVector<QPair<QString,QString> > & extraData = QVector<QPair<QString,QString> >()) :
            m_editor(editor), m_method(method), m_extraData(extraData)
        {}

        HtmlRetrieveFunctor(const HtmlRetrieveFunctor<T> & other) : m_editor(other.m_editor), m_method(other.m_method), m_extraData(other.m_extraData) {}
        HtmlRetrieveFunctor & operator=(const HtmlRetrieveFunctor<T> & other)
        { if (this != &other) { m_editor = other.m_editor; m_method = other.m_method; m_extraData = other.m_extraData; } return *this; }

        void operator()(const T & result) { (m_editor->*m_method)(result, m_extraData); }

    private:
        NoteEditorPrivate * m_editor;
        void (NoteEditorPrivate::* m_method)(const T & result, const QVector<QPair<QString,QString> > & extraData);
        QVector<QPair<QString, QString> > m_extraData;
    };

    friend class HtmlRetrieveFunctor<QString>;
    friend class HtmlRetrieveFunctor<QVariant>;

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

private:
    QString     m_noteEditorPageFolderPath;
    QString     m_noteEditorPagePath;
    QString     m_noteEditorImageResourcesStoragePath;

    QFont       m_font;
    QColor      m_fontColor;
    QColor      m_backgroundColor;

    // JavaScript scripts
    QString     m_jQueryJs;
    QString     m_resizableTableColumnsJs;
    QString     m_onFixedWidthTableResizeJs;
    QString     m_getSelectionHtmlJs;
    QString     m_snapSelectionToWordJs;
    QString     m_replaceSelectionWithHtmlJs;
    QString     m_provideSrcForResourceImgTagsJs;
    QString     m_setupEnToDoTagsJs;
    QString     m_onResourceInfoReceivedJs;
    QString     m_determineStatesForCurrentTextCursorPositionJs;
    QString     m_determineContextMenuEventTargetJs;

#ifndef USE_QT_WEB_ENGINE
    QString     m_qWebKitSetupJs;
#else
    QString     m_provideSrcForGenericResourceImagesJs;
    QString     m_onGenericResourceImageReceivedJs;
    QString     m_provideSrcAndOnClickScriptForEnCryptImgTagsJs;
    QString     m_qWebChannelJs;
    QString     m_qWebChannelSetupJs;
    QString     m_pageMutationObserverJs;
    QString     m_notifyTextCursorPositionChangedJs;
    QString     m_genericResourceOnClickHandlerJs;
    QString     m_setupGenericResourceOnClickHandlerJs;

    QWebSocketServer * m_pWebSocketServer;
    WebSocketClientWrapper * m_pWebSocketClientWrapper;
    QWebChannel * m_pWebChannel;
    PageMutationHandler * m_pPageMutationHandler;
    EnCryptElementOnClickHandler * m_pEnCryptElementClickHandler;
    GenericResourceOpenAndSaveButtonsOnClickHandler * m_pGenericResourceOpenAndSaveButtonsOnClickHandler;
    GenericResourceImageWriter * m_pGenericResourceImageWriter;

    quint16     m_webSocketServerPort;
#endif

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

    QString     m_enmlCachedMemory;   // Cached memory for HTML to ENML conversions
    QString     m_htmlCachedMemory;   // Cached memory for ENML from Note -> HTML conversions
    QString     m_errorCachedMemory;  // Cached memory for various errors

    QThread *   m_pIOThread;
    ResourceFileStorageManager *    m_pResourceFileStorageManager;
    FileIOThreadWorker *            m_pFileIOThreadWorker;

    ResourceInfo                    m_resourceInfo;
    ResourceInfoJavaScriptHandler * m_pResourceInfoJavaScriptHandler;

    QString                         m_resourceLocalFileStorageFolder;

    QHash<QUuid, QString>           m_genericResourceLocalGuidBySaveToStorageRequestIds;
    QHash<QString, QString>         m_resourceFileStoragePathsByResourceLocalGuid;
#ifdef USE_QT_WEB_ENGINE
    QSet<QString>                   m_localGuidsOfResourcesWantedToBeSaved;
    QSet<QString>                   m_localGuidsOfResourcesWantedToBeOpened;

    QHash<QString, QStringList>     m_fileSuffixesForMimeType;
    QHash<QString, QString>         m_fileFilterStringForMimeType;

    QSet<QUuid>                     m_manualSaveResourceToFileRequestIds;

    QHash<QByteArray, QString>      m_genericResourceImageFilePathsByResourceHash;
    GenericResourceImageJavaScriptHandler *  m_pGenericResoureImageJavaScriptHandler;

    QSet<QUuid>                     m_saveGenericResourceImageToFileRequestIds;
#endif

    QHash<QUuid, QPair<QString, QMimeType> >   m_droppedFilePathsAndMimeTypesByReadRequestIds;

    NoteEditor * const q_ptr;
    Q_DECLARE_PUBLIC(NoteEditor)
};

} // namespace qute_note

void __initNoteEditorResources();

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_P_H
