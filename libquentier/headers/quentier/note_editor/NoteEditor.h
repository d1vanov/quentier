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

#ifndef LIB_QUENTIER_NOTE_EDITOR_NOTE_EDITOR_H
#define LIB_QUENTIER_NOTE_EDITOR_NOTE_EDITOR_H

#include <quentier/types/Note.h>
#include <quentier/utility/Macros.h>
#include <quentier/utility/Linkage.h>
#include <quentier/types/ErrorString.h>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)
QT_FORWARD_DECLARE_CLASS(Notebook)
QT_FORWARD_DECLARE_CLASS(INoteEditorBackend)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)
QT_FORWARD_DECLARE_CLASS(SpellChecker)

/**
 * @brief The NoteEditor class is a widget encapsulating all the functionality necessary for showing and editing notes
 */
class QUENTIER_EXPORT NoteEditor: public QWidget
{
    Q_OBJECT
public:
    explicit NoteEditor(QWidget * parent = Q_NULLPTR, Qt::WindowFlags flags = 0);
    virtual ~NoteEditor();

    /**
     * NoteEditor requires FileIOThreadWorker and SpellChecker for its work but due to the particularities of Qt's .ui
     * files processing these can't be passed right inside the constructor, hence here's a special initialization method
     */
    void initialize(FileIOThreadWorker & fileIOThreadWorker, SpellChecker & spellChecker);

    /**
     * This method can be used to set the backend to the note editor; the note editor has the default backend
     * so this method is not obligatory to be called
     */
    void setBackend(INoteEditorBackend * backend);

    /**
     * Set the current account to the note editor
     */
    void setAccount(const Account & account);

    /**
     * Get the undo stack serving to the note editor
     */
    const QUndoStack * undoStack() const;

    /**
     * Set the undo stack for the note editor to use
     */
    void setUndoStack(QUndoStack * pUndoStack);

    /**
     * Set the html to be displayed when the note is not set to the editor
     */
    void setBlankPageHtml(const QString & html);

    /**
     * Get the local uid of the note currently set to the note editor
     */
    QString currentNoteLocalUid() const;

    /**
     * Set the note and its respective notebook to the note editor
     */
    void setNoteAndNotebook(const Note & note, const Notebook & notebook);

    /**
     * Clear the contents of the note editor
     */
    void clear();

    /**
     * @return true if there's content within the editor not yet converted to note, false otherwise
     */
    bool isModified() const;

    /**
     * @return true if the note last set to the editor has been fully loaded already,
     * false otherwise
     */
    bool isNoteLoaded() const;

    /**
     * Sets the focus to the backend note editor widget
     */
    void setFocus();

Q_SIGNALS:
    /**
     * @brief contentChanged signal is emitted when the note's content (text) gets modified via manual editing
     * (i.e. not any action like paste or cut)
     */
    void contentChanged();

    /**
     * @brief noteModified signal is emitted when the note's content within the editor gets modified via some way -
     * either via manual editing or via some action (like paste or cut)
     */
    void noteModified();

    /**
     * @brief notifyError signal is emitted when NoteEditor encounters some problem worth letting the user to know about
     */
    void notifyError(ErrorString error);

    void convertedToNote(Note note);
    void cantConvertToNote(ErrorString error);

    void noteEditorHtmlUpdated(QString html);

    void currentNoteChanged(Note note);

    void spellCheckerNotReady();
    void spellCheckerReady();

    void noteLoaded();

    // Signals to notify anyone interested of the formatting at the current cursor position
    void textBoldState(bool state);
    void textItalicState(bool state);
    void textUnderlineState(bool state);
    void textStrikethroughState(bool state);
    void textAlignLeftState(bool state);
    void textAlignCenterState(bool state);
    void textAlignRightState(bool state);
    void textAlignFullState(bool state);
    void textInsideOrderedListState(bool state);
    void textInsideUnorderedListState(bool state);
    void textInsideTableState(bool state);

    void textFontFamilyChanged(QString fontFamily);
    void textFontSizeChanged(int fontSize);

    void insertTableDialogRequested();

public Q_SLOTS:

    /**
     * Invoke this slot to launch the asynchronous procedure of converting the current contents of the note editor
     * to note; the @link convertedToNote @endlink signal would be emitted in response when the conversion is done
     */
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
    void alignFull();

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

protected:
    virtual void dragMoveEvent(QDragMoveEvent * pEvent) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent * pEvent) Q_DECL_OVERRIDE;

private:
    INoteEditorBackend * m_backend;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_NOTE_EDITOR_H
