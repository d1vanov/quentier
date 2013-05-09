#include "TextEditWithCheckboxes.h"
#include "ToDoCheckboxTextObject.h"
#include "NoteRichTextEditor.h"
#include <QMouseEvent>
#include <QTextCursor>
#include <QMessageBox>
#include <QDebug>

TextEditWithCheckboxes::TextEditWithCheckboxes(QWidget *parent) :
    QTextEdit(parent)
{}

void TextEditWithCheckboxes::mousePressEvent(QMouseEvent * pEvent)
{
    QTextCursor cursor = cursorForPosition(pEvent->pos());
    QTextCharFormat format = cursor.charFormat();
    if (format.objectType() == NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_UNCHECKED)
    {
        QString checkboxCheckedImgFileName(":/format_text_elements/checkbox_checked.png");
        QFile checkboxCheckedImgFile(checkboxCheckedImgFileName);
        if (!checkboxCheckedImgFile.exists()) {
            QMessageBox::warning(this, tr("Error Opening File"),
                                 tr("Could not open '%1'").arg(checkboxCheckedImgFileName));
        }

        QImage checkboxCheckedImg(checkboxCheckedImgFileName);
        format.setObjectType(NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_CHECKED);
        format.setProperty(NoteRichTextEditor::CHECKBOX_TEXT_DATA_CHECKED, checkboxCheckedImg);

        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::WordUnderCursor);
        }

        cursor.mergeCharFormat(format);
        QTextEdit::mergeCurrentCharFormat(format);
    }
    else if (format.objectType() == NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_CHECKED)
    {
        format.setObjectType(NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_UNCHECKED);

        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::WordUnderCursor);
        }

        cursor.mergeCharFormat(format);
        QTextEdit::mergeCurrentCharFormat(format);
    }
}
