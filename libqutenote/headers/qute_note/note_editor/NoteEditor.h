#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/Linkage.h>

#ifdef USE_QT_WEB_ENGINE
#include <QWebEngineView>
#else
#include <QWebView>
#endif

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactory)
QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

#ifdef USE_QT_WEB_ENGINE
class QUTE_NOTE_EXPORT NoteEditor: public QWebEngineView
#else
class QUTE_NOTE_EXPORT NoteEditor: public QWebView
#endif
{
    Q_OBJECT
public:

public:
    explicit NoteEditor(QWidget * parent = nullptr);
    virtual ~NoteEditor();

    void setNoteAndNotebook(const Note & note, const Notebook & notebook);
    const Note * getNote();
    const Notebook * getNotebook() const;

    bool isModified() const;

    const NoteEditorPluginFactory & pluginFactory() const;
    NoteEditorPluginFactory & pluginFactory();

Q_SIGNALS:
    void contentChanged();
    void notifyError(QString error);

public Q_SLOTS:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void textBold();
    void textItalic();
    void textUnderline();
    void textStrikethrough();
    void alignLeft();
    void alignCenter();
    void alignRight();
    void insertToDoCheckbox();
    void setSpellcheck(const bool enabled);
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
    void onEncryptedAreaDecryption();
    void onNoteLoadCancelled();

private:
    virtual void dropEvent(QDropEvent * pEvent) Q_DECL_OVERRIDE;

private:
    NoteEditorPrivate * const   d_ptr;
    Q_DECLARE_PRIVATE(NoteEditor)
};

} // namespace qute_note

#endif //__LIB_QUTE_NOTE__NOTE_EDITOR__NOTE_EDITOR_H
