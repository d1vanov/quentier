#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_H

#include <tools/qt4helper.h>
#include <tools/Linkage.h>
#include <QWebView>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactory)
QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)

class QUTE_NOTE_EXPORT NoteEditor: public QWebView
{
    Q_OBJECT
public:

public:
    explicit NoteEditor(QWidget * parent = nullptr);
    virtual ~NoteEditor();

    void setNoteAndNotebook(const Note & note, const Notebook & notebook);
    const Note * getNote() const;
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

    void onNoteLoadCancelled();

private:
    virtual void dropEvent(QDropEvent * pEvent) Q_DECL_OVERRIDE;

private:
    NoteEditorPrivate * const   d_ptr;
    Q_DECLARE_PRIVATE(NoteEditor)
};

} // namespace qute_note

#endif //__QUTE_NOTE__CORE__NOTE_EDITOR__NOTE_EDITOR_H
