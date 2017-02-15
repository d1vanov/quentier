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

#ifndef LIB_QUENTIER_NOTE_EDITOR_I_NOTE_EDITOR_BACKEND_H
#define LIB_QUENTIER_NOTE_EDITOR_I_NOTE_EDITOR_BACKEND_H

#include <quentier/utility/Macros.h>
#include <quentier/utility/Linkage.h>
#include <quentier/utility/Printable.h>
#include <quentier/types/Note.h>
#include <quentier/types/Notebook.h>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QUndoStack)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Account)
QT_FORWARD_DECLARE_CLASS(NoteEditor)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)
QT_FORWARD_DECLARE_CLASS(SpellChecker)

class QUENTIER_EXPORT INoteEditorBackend
{
public:
    virtual ~INoteEditorBackend();

    virtual void initialize(FileIOThreadWorker & fileIOThreadWorker, SpellChecker & spellChecker) = 0;

    virtual QObject * object() = 0;   // provide QObject interface
    virtual QWidget * widget() = 0;   // provide QWidget interface

    virtual void setAccount(const Account & account) = 0;
    virtual void setUndoStack(QUndoStack * pUndoStack) = 0;

    virtual void setBlankPageHtml(const QString & html) = 0;

    virtual bool isNoteLoaded() const = 0;

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
    virtual void alignFull() = 0;

    virtual QString selectedText() const = 0;
    virtual bool hasSelection() const = 0;

    virtual void findNext(const QString & text, const bool matchCase) const = 0;
    virtual void findPrevious(const QString & text, const bool matchCase) const = 0;
    virtual void replace(const QString & textToReplace, const QString & replacementText, const bool matchCase) = 0;
    virtual void replaceAll(const QString & textToReplace, const QString & replacementText, const bool matchCase) = 0;

    virtual void insertToDoCheckbox() = 0;

    virtual void setSpellcheck(const bool enabled) = 0;
    virtual bool spellCheckEnabled() const = 0;

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
    virtual void insertTableRow() = 0;
    virtual void insertTableColumn() = 0;
    virtual void removeTableRow() = 0;
    virtual void removeTableColumn() = 0;

    virtual void addAttachmentDialog() = 0;
    virtual void saveAttachmentDialog(const QByteArray & resourceHash) = 0;
    virtual void saveAttachmentUnderCursor() = 0;
    virtual void openAttachment(const QByteArray & resourceHash) = 0;
    virtual void openAttachmentUnderCursor() = 0;
    virtual void copyAttachment(const QByteArray & resourceHash) = 0;
    virtual void copyAttachmentUnderCursor() = 0;
    virtual void removeAttachment(const QByteArray & resourceHash) = 0;
    virtual void removeAttachmentUnderCursor() = 0;
    virtual void renameAttachment(const QByteArray & resourceHash) = 0;
    virtual void renameAttachmentUnderCursor() = 0;

    struct Rotation
    {
        enum type
        {
            Clockwise = 0,
            Counterclockwise
        };

        friend QTextStream & operator<<(QTextStream & strm, const type & rotation);
    };

    virtual void rotateImageAttachment(const QByteArray & resourceHash, const Rotation::type rotationDirection) = 0;
    virtual void rotateImageAttachmentUnderCursor(const Rotation::type rotationDirection) = 0;

    virtual void encryptSelectedText() = 0;

    virtual void decryptEncryptedTextUnderCursor() = 0;
    virtual void decryptEncryptedText(QString encryptedText, QString cipher, QString keyLength, QString hint, QString enCryptIndex) = 0;

    virtual void hideDecryptedTextUnderCursor() = 0;
    virtual void hideDecryptedText(QString encryptedText, QString decryptedText, QString cipher, QString keyLength, QString hint, QString enDecryptedIndex) = 0;

    virtual void editHyperlinkDialog() = 0;
    virtual void copyHyperlink() = 0;
    virtual void removeHyperlink() = 0;

    virtual void onNoteLoadCancelled() = 0;

    virtual void setNoteAndNotebook(const Note & note, const Notebook & notebook) = 0;

    virtual void clear() = 0;

    virtual bool isModified() const = 0;

    virtual void setFocusToEditor() = 0;

protected:
    INoteEditorBackend(NoteEditor * parent);
    NoteEditor *    m_pNoteEditor;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_I_NOTE_EDITOR_BACKEND_H
