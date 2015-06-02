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

namespace qute_note {


class NoteEditorPrivate: public QObject
{
    Q_OBJECT
public:
    explicit NoteEditorPrivate(NoteEditor & noteEditor);

    QVariant execJavascriptCommandWithResult(const QString & command);
    void execJavascriptCommand(const QString & command);

    QVariant execJavascriptCommandWithResult(const QString & command, const QString & args);
    void execJavascriptCommand(const QString & command, const QString & args);

    void setNote(const Note & note);
    const Note * getNote() const;

    bool isModified() const;

    const NoteEditorPluginFactory & pluginFactory() const;
    NoteEditorPluginFactory & pluginFactory();

    void onDropEvent(QDropEvent * pEvent);
    void dropFile(QString & filepath);
    void attachResourceToNote(const QByteArray & data, const QString & dataHash, const QMimeType & mimeType);

    template <typename T>
    QString composeHtmlTable(const T width, const T singleColumnWidth, const int rows, const int columns,
                             const bool relative, const size_t tableId);

    void insertImage(const QByteArray & data,  const QString & dataHash, const QMimeType & mimeType);
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

private Q_SLOTS:
    void onNoteLoadFinished(bool ok);

private:
    // JavaScript scripts
    QString     m_jQuery;
    QString     m_resizableColumnsPlugin;
    QString     m_onFixedWidthTableResize;
    QString     m_getSelectionHtml;
    QString     m_replaceSelectionWithHtml;

    Note *      m_pNote;
    bool        m_modified;
    size_t      m_lastFreeId;

    QSharedPointer<EncryptionManager>       m_encryptionManager;
    DecryptedTextCachePtr                   m_decryptedTextCache;

    ENMLConverter                           m_enmlConverter;

    NoteEditorPluginFactory *               m_pluginFactory;

    NoteEditor * const q_ptr;
    Q_DECLARE_PUBLIC(NoteEditor)
};

} // namespace qute_note

void __initNoteEditorResources();

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_PRIVATE_H
