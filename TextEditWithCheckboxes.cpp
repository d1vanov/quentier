#include "TextEditWithCheckboxes.h"
#include "ToDoCheckboxTextObject.h"
#include "NoteRichTextEditor.h"
#include <QMouseEvent>
#include <QTextCursor>
#include <QDebug>

TextEditWithCheckboxes::TextEditWithCheckboxes(QWidget *parent) :
    QTextEdit(parent)
{}

void TextEditWithCheckboxes::mousePressEvent(QMouseEvent * pEvent)
{
    qDebug() << "Processing mousePressEvent";
    QTextCursor cursor = cursorForPosition(pEvent->pos());
    QTextCharFormat format = cursor.charFormat();
    if (format.objectType() == NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_UNCHECKED)
    {
        qDebug() << "Unchecked checkbox under mouse cursor";
        format.setObjectType(NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_CHECKED);

        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::WordUnderCursor);
        }

        cursor.mergeCharFormat(format);
        QTextEdit::mergeCurrentCharFormat(format);

        // FIXME: for debug purposes, cut out before merging with master branch
        qDebug() << "Changed char format from unchecked to checked checkbox, verifying...";
        format = cursor.charFormat();
        if (format.objectType() == NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_CHECKED) {
            qDebug() << "Char format successfully changed to checked checkbox";
        }
    }
    else if (format.objectType() == NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_CHECKED) {
        qDebug() << "Checked checkbox under mouse cursor";
        format.setObjectType(NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_UNCHECKED);

        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::WordUnderCursor);
        }

        cursor.mergeCharFormat(format);
        QTextEdit::mergeCurrentCharFormat(format);

        // FIXME: for debug purposes, cut out before merging with master branch
        qDebug() << "Changed char format from checked to unchecked checkbox, verifying...";
        format = cursor.charFormat();
        if (format.objectType() == NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_UNCHECKED) {
            qDebug() << "Char format successfully changed to unchecked checkbox";
        }
    }
}
