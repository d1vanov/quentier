#include "TextEditWithCheckboxes.h"
#include "ToDoCheckboxTextObject.h"
#include "NoteRichTextEditor.h"
#include <QMouseEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextList>
#include <QTextDocumentFragment>
#include <QMessageBox>
#include <QApplication>
#include <QUrl>
#include <QString>
#include <QImage>
#include <QDebug>

TextEditWithCheckboxes::TextEditWithCheckboxes(QWidget * parent) :
    QTextEdit(parent),
    m_droppedImageCounter(0)
{}

bool TextEditWithCheckboxes::canInsertFromMimeData(const QMimeData * source) const
{
    return (source->hasImage() || QTextEdit::canInsertFromMimeData(source));
}

void TextEditWithCheckboxes::insertFromMimeData(const QMimeData *source)
{
    if (source->hasImage()) {
        QUrl url(QString("dropped_image_%1").arg(m_droppedImageCounter++));
        dropImage(url, qvariant_cast<QImage>(source->imageData()));
    }
}

void TextEditWithCheckboxes::keyPressEvent(QKeyEvent * pEvent)
{
    if ((pEvent->key() == Qt::Key_Enter) || (pEvent->key() == Qt::Key_Return))
    {
        // check whether current line is a horizontal line
        bool atHorizontalLine = false;
        QTextCursor cursor = QTextEdit::textCursor();
        QTextBlockFormat initialFormat = cursor.blockFormat();
        if (cursor.block().userData()) {
            atHorizontalLine = true;
        }

        if (atHorizontalLine) {
            cursor.insertBlock();
            QTextBlockFormat format;
            format.setLineHeight(initialFormat.lineHeight(), initialFormat.lineHeightType());
            cursor.setBlockFormat(format);
            QTextEdit::setTextCursor(cursor);
        }
        else if (cursor.currentList())
        {
            QTextList * pList = cursor.currentList();
            // check whether cursor is on the last element of the list
            int numElements = pList->count();
            QTextBlock block = cursor.block();
            if ( (pList->itemNumber(block) == (numElements - 1)) &&
                 QString(block.text().trimmed().toAscii()).isEmpty() )
            {
                QTextBlockFormat format;
                cursor.setBlockFormat(format);
                QTextEdit::setTextCursor(cursor);
            }
            else {
                QTextEdit::keyPressEvent(pEvent);
            }
        }
        else {
            QTextEdit::keyPressEvent(pEvent);
        }
    }
    else if (pEvent->key() == Qt::Key_Backspace)
    {
        QTextCursor cursor = QTextEdit::textCursor();
        if (cursor.currentList())
        {
            QTextList * pList = cursor.currentList();
            QTextBlock block = cursor.block();
            int itemNumber = pList->itemNumber(block);
            int numItems = pList->count();
            if ( (pList->format().style() == QTextListFormat::ListDecimal) &&
                 (itemNumber != (numItems - 1)) &&
                 (QString(block.text().trimmed().toAscii()).isEmpty()) )
            {
                // ordered list + not the last item + empty item, specialized logic is required:
                // 1. Backup all the elements following the one being removed
                QVector<QTextBlock> buffer;
                buffer.reserve(numItems - itemNumber - 1);
                for(int i = itemNumber + 1; i < numItems; ++i) {
                    buffer.push_back(pList->item(i));
                }
                // 2. Remove all the elements following the removed one and it
                for(int i = numItems - 1; i >= itemNumber; --i) {
                    pList->removeItem(i);
                }
                // NOTE: this action only removes list items but not blocks of text from the editor
                // 3. Insert empty block of text at the position where the cursor was
                QTextBlockFormat format;
                cursor.setBlockFormat(format);
                QTextEdit::setTextCursor(cursor);
                // 4. Fix indentation of the remained contents of the list
                QTextCursor cursorIndentFixer(cursor);
                for(int i =itemNumber; i < numItems; ++i) {
                    cursorIndentFixer.movePosition(QTextCursor::Down);
                    QTextBlockFormat format = cursorIndentFixer.block().blockFormat();
                    format.setIndent(format.indent() - 1);
                    cursorIndentFixer.mergeBlockFormat(format);
                }
                // 5. Create a new list and fill it with saved items
                QTextList * pNewList = cursor.createList(QTextListFormat::ListDecimal);
                Q_CHECK_PTR(pNewList);

                int nBufferedItems = buffer.size();
                for(int i = 0; i < nBufferedItems; ++i) {
                    pNewList->add(buffer[i]);
                }
                // Don't understand why but QTextList::add doesn't account for the very first item,
                // so I need to remove it manually
                pNewList->removeItem(0);
            }
            else {
                QTextEdit::keyPressEvent(pEvent);
            }
        }
        else {
            QTextEdit::keyPressEvent(pEvent);
        }
    }
    else {
        QTextEdit::keyPressEvent(pEvent);
    }
}

void TextEditWithCheckboxes::mousePressEvent(QMouseEvent * pEvent)
{
    QTextCursor cursor = cursorForPosition(pEvent->pos());
    QTextCharFormat format = cursor.charFormat();
    if (format.objectType() == NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_UNCHECKED)
    {
        QString checkboxCheckedImgFileName(":/format_text_elements/checkbox_checked.gif");
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

void TextEditWithCheckboxes::mouseMoveEvent(QMouseEvent * pEvent)
{
    QTextCursor cursor = cursorForPosition(pEvent->pos());
    QTextCharFormat format = cursor.charFormat();
    if ( (format.objectType() == NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_CHECKED) ||
         (format.objectType() == NoteRichTextEditor::CHECKBOX_TEXT_FORMAT_UNCHECKED) )
    {
        QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    }
    else {
        QApplication::restoreOverrideCursor();
    }
}

void TextEditWithCheckboxes::dropImage(const QUrl & url, const QImage & image)
{
    if (!image.isNull())
    {
        QTextEdit::document()->addResource(QTextDocument::ImageResource, url, image);
        QTextEdit::textCursor().insertImage(url.toString());
    }
}
