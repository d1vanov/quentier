#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__I_NOTE_EDITOR_BACKEND_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__I_NOTE_EDITOR_BACKEND_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/Linkage.h>
#include <qute_note/types/Note.h>
#include <qute_note/types/Notebook.h>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditor)

class QUTE_NOTE_EXPORT INoteEditorBackend
{
public:
    virtual ~INoteEditorBackend();

    virtual QObject * object() = 0;   // provide QObject interface
    virtual QWidget * widget() = 0;   // provide QWidget interface

    virtual void setUndoStack(QUndoStack * pUndoStack) = 0;

    virtual void convertToNote() = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual void cut() = 0;
    virtual void copy() = 0;
    virtual void paste() = 0;
    virtual void pasteUnformatted() = 0;
    virtual void selectAll() = 0;

    virtual void fontMenu() = 0;
    virtual void textBold() = 0;
    virtual void textItalic() = 0;
    virtual void textUnderline() = 0;
    virtual void textStrikethrough() = 0;
    virtual void textHighlight() = 0;

    virtual void alignLeft() = 0;
    virtual void alignCenter() = 0;
    virtual void alignRight() = 0;

    virtual void insertToDoCheckbox() = 0;

    virtual void setSpellcheck(const bool enabled) = 0;

    virtual void setFont(const QFont & font) = 0;
    virtual void setFontHeight(const int height) = 0;
    virtual void setFontColor(const QColor & color) = 0;
    virtual void setBackgroundColor(const QColor & color) = 0;

    virtual void insertHorizontalLine() = 0;

    virtual void increaseFontSize() = 0;
    virtual void decreaseFontSize() = 0;

    virtual void increaseIndentation() = 0;
    virtual void decreaseIndentation() = 0;

    virtual void insertBulletedList() = 0;
    virtual void insertNumberedList() = 0;

    virtual void insertTableDialog() = 0;
    virtual void insertFixedWidthTable(const int rows, const int columns, const int widthInPixels) = 0;
    virtual void insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth) = 0;

    virtual void addAttachmentDialog() = 0;
    virtual void saveAttachmentDialog(const QString & resourceHash) = 0;
    virtual void saveAttachmentUnderCursor() = 0;
    virtual void openAttachment(const QString & resourceHash) = 0;
    virtual void openAttachmentUnderCursor() = 0;
    virtual void copyAttachment(const QString & resourceHash) = 0;
    virtual void copyAttachmentUnderCursor() = 0;
    virtual void removeAttachment(const QString & resourceHash) = 0;
    virtual void removeAttachmentUnderCursor() = 0;

    struct Rotation
    {
        enum type
        {
            Clockwise = 0,
            Counterclockwise
        };

        friend std::ostream & operator<<(const type rotation, std::ostream & strm);
    };

    virtual void rotateImageAttachment(const QString & resourceHash, const Rotation::type rotationDirection) = 0;
    virtual void rotateImageAttachmentUnderCursor(const Rotation::type rotationDirection) = 0;

    virtual void encryptSelectedTextDialog() = 0;
    virtual void decryptEncryptedTextUnderCursor() = 0;
    virtual void decryptEncryptedText(QString encryptedText, QString cipher, QString keyLength, QString hint) = 0;

    virtual void editHyperlinkDialog() = 0;
    virtual void copyHyperlink() = 0;
    virtual void removeHyperlink() = 0;

    virtual void onNoteLoadCancelled() = 0;

    virtual void setNoteAndNotebook(const Note & note, const Notebook & notebook) = 0;

protected:
    INoteEditorBackend(NoteEditor * parent);
    NoteEditor *    m_pNoteEditor;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__I_NOTE_EDITOR_BACKEND_H
