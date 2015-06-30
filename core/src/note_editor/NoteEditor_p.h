#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PRIVATE_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PRIVATE_H

#include "NoteEditor.h"
#include <client/enml/ENMLConverter.h>
#include <tools/EncryptionManager.h>
#include <QObject>
#include <QCache>

QT_FORWARD_DECLARE_CLASS(QByteArray)
QT_FORWARD_DECLARE_CLASS(QMimeType)
QT_FORWARD_DECLARE_CLASS(QImage)
QT_FORWARD_DECLARE_CLASS(QThread)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

class NoteEditorPrivate: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorPrivate(NoteEditor & noteEditor);
    virtual ~NoteEditorPrivate();

    QVariant execJavascriptCommandWithResult(const QString & command);
    void execJavascriptCommand(const QString & command);

    QVariant execJavascriptCommandWithResult(const QString & command, const QString & args);
    void execJavascriptCommand(const QString & command, const QString & args);

    void setNoteAndNotebook(const Note & note, const Notebook & notebook);
    const Note * getNote();
    const Notebook * getNotebook() const;

    bool isModified() const;

    const NoteEditorPluginFactory & pluginFactory() const;
    NoteEditorPluginFactory & pluginFactory();

    void onDropEvent(QDropEvent * pEvent);
    void dropFile(QString & filepath);
    void attachResourceToNote(const QByteArray & data, const QByteArray &dataHash,
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

Q_SIGNALS:
    void notifyError(QString error);

    // private signals:
    void saveResourceToStorage(QString localGuid, QByteArray data, QByteArray dataHash, QUuid requestId);

private Q_SLOTS:
    void onNoteLoadFinished(bool ok);
    void onContentChanged();
    void onResourceSavedToStorage(QUuid requestId, int errorCode, QString errorDescription);

private:
    virtual void timerEvent(QTimerEvent * event) Q_DECL_OVERRIDE;

private:
    void clearContent();
    void updateColResizableTableBindings();

    bool htmlToNoteContent(QString & errorDescription);

    void checkResourceLocalFilesAndProvideSrcForImgResources(const QString & noteContentHtml);
    void provideScrForImgResourcesFromCache();

private:
    // JavaScript scripts
    QString     m_jQuery;
    QString     m_resizableColumnsPlugin;
    QString     m_onFixedWidthTableResize;
    QString     m_getSelectionHtml;
    QString     m_replaceSelectionWithHtml;

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

    NoteEditorPluginFactory *               m_pluginFactory;

    const QString   m_pagePrefix;

    QString     m_htmlCachedMemory;   // Cached memory for HTML from Note -> HTML conversions
    QString     m_errorCachedMemory;  // Cached memory for various errors

    QThread *   m_pIOThread;
    ResourceFileStorageManager *    m_pResourceFileStorageManager;
    FileIOThreadWorker *            m_pFileIOThreadWorker;

    struct ResourceLocalFileInfo {
        QString m_resourceHash;
        QString m_resourceLocalFilePath;
    };

    QHash<QString, ResourceLocalFileInfo> m_resourceLocalFileInfoCache;
    QString     m_resourceLocalFileStorageFolder;

    QHash<QString, QPair<QString, QByteArray> > m_resourceLocalGuidAndDataHashBySaveToStorageRequestIds;

    NoteEditor * const q_ptr;
    Q_DECLARE_PUBLIC(NoteEditor)
};

} // namespace qute_note

void __initNoteEditorResources();

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PRIVATE_H
