#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_P_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_P_H

#include <qute_note/note_editor/NoteEditor.h>
#include <qute_note/enml/ENMLConverter.h>
#include <qute_note/utility/EncryptionManager.h>
#include <QObject>
#include <QCache>
#include <QMimeType>

QT_FORWARD_DECLARE_CLASS(QByteArray)
QT_FORWARD_DECLARE_CLASS(QMimeType)
QT_FORWARD_DECLARE_CLASS(QImage)
QT_FORWARD_DECLARE_CLASS(QThread)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

typedef QHash<QString, QString> ResourceLocalFileInfoCache;

// The instance of this class would be exposed to JavaScript code to provide
// read-only access to resource hashes and local file paths by resource local guids;
//
// Unfortunately, metaobject features are not supported for nested classes,
// so here is the non-nested helper class for exposure to JavaScript
//
class ResourceLocalFileInfoJavaScriptHandler: public QObject
{
    Q_OBJECT
public:
    explicit ResourceLocalFileInfoJavaScriptHandler(const ResourceLocalFileInfoCache & cache,
                                                    QObject * parent = nullptr) :
        QObject(parent),
        m_cache(cache)
    {}

public Q_SLOTS:
    QString getResourceLocalFilePath(const QString & resourceHash) const;

private:
    const ResourceLocalFileInfoCache & m_cache;
};

#ifdef USE_QT_WEB_ENGINE

// The instance of this class would be exposed to JavaScript call
// in order to emulate the contentsChanged signal that way because - surprise! -
// QWebEnginePage doesn't provide such a signal natively!
class PageMutationHandler: public QObject
{
    Q_OBJECT
public:
    explicit PageMutationHandler(QObject * parent = nullptr) :
        QObject(parent)
    {}

Q_SIGNALS:
    void contentsChanged();

public Q_SLOTS:
    void onPageMutation() { emit contentsChanged(); }
};

// The instance of this class would be exposed to JavaScript call in order to propagate
// the click on en-crypt element to the C++ code
class EnCryptElementClickHandler: public QObject
{
    Q_OBJECT
public:
    explicit EnCryptElementClickHandler(QObject * parent = nullptr) :
        QObject(parent)
    {}

Q_SIGNALS:
    void decrypt(QString encryptedText, QString cipher, QString length, QString hint);

public Q_SLOTS:
    void onClick(QString encryptedText, QString cipher, QString length, QString hint)
    { emit decrypt(encryptedText, cipher, length, hint); }
};

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

    void insertToDoCheckbox();

    void setFont(const QFont & font);
    void setFontHeight(const int height);
    void setFontColor(const QColor & color);
    void setBackgroundColor(const QColor & color);
    void insertHorizontalLine();
    void changeIndentation(const bool increase);
    void insertBulletedList();
    void insertNumberedList();
    void insertFixedWidthTable(const int rows, const int columns, const int widthInPixels);
    void insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth);
    void encryptSelectedText(const QString & passphrase, const QString & hint);
    void onEncryptedAreaDecryption(QString encryptedText, QString decryptedText, bool rememberForSession);
    void onNoteLoadCancelled();

// private signals:
Q_SIGNALS:
    void saveResourceToStorage(QString localGuid, QByteArray data, QByteArray dataHash, QUuid requestId);
    void readDroppedFileData(QString absoluteFilePath, QUuid requestId);

private Q_SLOTS:
    void onNoteLoadFinished(bool ok);
    void onContentChanged();
    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                  int errorCode, QString errorDescription);
    void onDroppedFileRead(bool success, QString errorDescription, QByteArray data, QUuid requestId);

#ifdef USE_QT_WEB_ENGINE
    void onEnCryptElementClicked(QString encryptedText, QString cipher, QString length, QString hint);
#endif

private:
    virtual void timerEvent(QTimerEvent * event) Q_DECL_OVERRIDE;

private:
    void clearEditorContent();
    void noteToEditorContent();
    void updateColResizableTableBindings();

    bool htmlToNoteContent(QString & errorDescription);

    void checkResourceLocalFilesAndProvideSrcForImgResources(const QString & noteContentHtml);
    void provideScrForImgResourcesFromCache();

#ifdef USE_QT_WEB_ENGINE
    void checkEnCryptIconAndProvideSrcForEnCryptTags();
    void provideSrcForImgEnCryptTags();

    bool isPageEditable() const { return m_isPageEditable; }
    void setPageEditable(const bool editable);
#endif

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

private:
    // JavaScript scripts
    QString     m_jQuery;
    QString     m_resizableColumnsPlugin;
    QString     m_onFixedWidthTableResize;
    QString     m_getSelectionHtml;
    QString     m_replaceSelectionWithHtml;
    QString     m_provideSrcForResourceImgTags;

#ifdef USE_QT_WEB_ENGINE
    QString     m_provideSrcForEnCryptImgTags;
    quint16     m_webSocketServerPort;
    bool        m_isPageEditable;
#endif

    bool        m_pendingConversionToNote;

    Note *      m_pNote;
    Notebook *  m_pNotebook;

    bool        m_modified;

    // These two bools implement a cheap scheme of watching
    // for changes in note editor since some particular moment in time.
    // For example, the conversion of note from HTML into ENML happens
    // in the background mode, when the editor is idle for at least 5 seconds.
    // How can such idle state be determined? Create a timer for 5 seconds,
    // as it begins, set m_watchingForContentChange to true and
    // m_contentChangedSinceWatchingStart to false. On every next content change
    // m_contentChangedSinceWatchingStart would be set to true. When the timer ends,
    // it can check the state of m_contentChangedSinceWatchingStart.
    // If it's true, it means the editing is still in progress and it's not nice
    // to block the GUI thread by HTML to ENML conversion. So drop this
    // variable into false again and wait for another 5 seconds. And only
    // if there were no further edits during 5 seconds, convert note editor's page
    // to ENML
    bool        m_watchingForContentChange;
    bool        m_contentChangedSinceWatchingStart;

    int         m_pageToNoteContentPostponeTimerId;

    QSharedPointer<EncryptionManager>       m_encryptionManager;
    DecryptedTextCachePtr                   m_decryptedTextCache;

    ENMLConverter                           m_enmlConverter;

#ifndef USE_QT_WEB_ENGINE
    NoteEditorPluginFactory *               m_pluginFactory;
#endif

    const QString     m_pagePrefix;

    QString     m_enmlCachedMemory;   // Cached memory for HTML to ENML conversions
    QString     m_htmlCachedMemory;   // Cached memory for ENML from Note -> HTML conversions
    QString     m_errorCachedMemory;  // Cached memory for various errors

    QThread *   m_pIOThread;
    ResourceFileStorageManager *    m_pResourceFileStorageManager;
    FileIOThreadWorker *            m_pFileIOThreadWorker;

    ResourceLocalFileInfoCache      m_resourceLocalFileInfoCache;

    QString     m_resourceLocalFileStorageFolder;

    QHash<QUuid, QString> m_resourceLocalGuidBySaveToStorageRequestIds;

    QHash<QUuid, QPair<QString, QMimeType> >   m_droppedFileNamesAndMimeTypesByReadRequestIds;
    QSet<QUuid> m_saveNewResourcesToStorageRequestIds;

    NoteEditor * const q_ptr;
    Q_DECLARE_PUBLIC(NoteEditor)
};

} // namespace qute_note

void __initNoteEditorResources();

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_P_H
