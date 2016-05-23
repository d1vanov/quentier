#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_NOTE_EDITOR_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_NOTE_EDITOR_H

#include <qute_note/types/Note.h>
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/Linkage.h>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(INoteEditorBackend)

class QUTE_NOTE_EXPORT NoteEditor: public QWidget
{
    Q_OBJECT
public:
    explicit NoteEditor(QWidget * parent = Q_NULLPTR, Qt::WindowFlags flags = 0);
    virtual ~NoteEditor();

    void setBackend(INoteEditorBackend * backend);

    const QUndoStack * undoStack() const;
    void setUndoStack(QUndoStack * pUndoStack);

    void setNoteAndNotebook(const Note & note, const Notebook & notebook);

Q_SIGNALS:
    void contentChanged();
    void notifyError(QString error);

    void convertedToNote(Note note);
    void cantConvertToNote(QString error);

    void noteEditorHtmlUpdated(QString html);

    void currentNoteChanged(Note note);

    void spellCheckerNotReady();
    void spellCheckerReady();

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

public Q_SLOTS:
    void convertToNote();
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

    QString selectedText() const;
    bool hasSelection() const;

    void findNext(const QString & text, const bool matchCase) const;
    void findPrevious(const QString & text, const bool matchCase) const;
    void replace(const QString & textToReplace, const QString & replacementText, const bool matchCase);
    void replaceAll(const QString & textToReplace, const QString & replacementText, const bool matchCase);

    void insertToDoCheckbox();

    void setSpellcheck(const bool enabled);
    virtual bool spellCheckEnabled() const;

    void setFont(const QFont & font);
    void setFontHeight(const int height);
    void setFontColor(const QColor & color);
    void setBackgroundColor(const QColor & color);

    void insertHorizontalLine();

    void increaseFontSize();
    void decreaseFontSize();

    void increaseIndentation();
    void decreaseIndentation();

    void insertBulletedList();
    void insertNumberedList();

    void insertTableDialog();
    void insertFixedWidthTable(const int rows, const int columns, const int widthInPixels);
    void insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth);
    void insertTableRow();
    void insertTableColumn();
    void removeTableRow();
    void removeTableColumn();

    void addAttachmentDialog();
    void saveAttachmentDialog(const QByteArray & resourceHash);
    void saveAttachmentUnderCursor();
    void openAttachment(const QByteArray & resourceHash);
    void openAttachmentUnderCursor();
    void copyAttachment(const QByteArray & resourceHash);
    void copyAttachmentUnderCursor();

    void encryptSelectedText();
    void decryptEncryptedTextUnderCursor();

    void editHyperlinkDialog();
    void copyHyperlink();
    void removeHyperlink();

    void onNoteLoadCancelled();

    void setFocus();

protected:
    virtual void dragMoveEvent(QDragMoveEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent * pEvent) Q_DECL_OVERRIDE;

private:
    INoteEditorBackend * m_backend;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_NOTE_EDITOR_H
