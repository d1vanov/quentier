#include "QuteNoteTextEdit.h"
#include "ToDoCheckboxTextObject.h"
#include "../evernote_client/enml/ENMLConverter.h"
#include "../evernote_client/Note.h"
#include <QMimeData>
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

QuteNoteTextEdit::QuteNoteTextEdit(QWidget * parent) :
    QTextEdit(parent),
    m_droppedImageCounter(0),
    m_pNote(nullptr)
{}

bool QuteNoteTextEdit::canInsertFromMimeData(const QMimeData * source) const
{
    return (source->hasImage() || QTextEdit::canInsertFromMimeData(source));
}

void QuteNoteTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (source->hasImage()) {
        QUrl url(QString("dropped_image_%1").arg(m_droppedImageCounter++));
        dropImage(url, qvariant_cast<QImage>(source->imageData()));
    }
    else {
        QTextEdit::insertFromMimeData(source);
    }
}

void QuteNoteTextEdit::changeIndentation(const bool increase)
{
    QTextCursor cursor = QTextEdit::textCursor();
    cursor.beginEditBlock();

    if (cursor.currentList())
    {
        QTextListFormat listFormat = cursor.currentList()->format();

        if (increase) {
            listFormat.setIndent(listFormat.indent() + 1);
        }
        else {
            listFormat.setIndent(std::max(listFormat.indent() - 1, 0));
        }

        cursor.createList(listFormat);
    }
    else
    {
        int start = cursor.anchor();
        int end = cursor.position();
        if (start > end)
        {
            start = cursor.position();
            end = cursor.anchor();
        }

        QList<QTextBlock> blocksList;
        QTextBlock block = QTextEdit::document()->begin();
        while (block.isValid())
        {
            block = block.next();
            if ( ((block.position() >= start) && (block.position() + block.length() <= end) ) ||
                 block.contains(start) || block.contains(end) )
            {
                blocksList << block;
            }
        }

        foreach(QTextBlock block, blocksList)
        {
            QTextCursor cursor(block);
            QTextBlockFormat blockFormat = cursor.blockFormat();

            if (increase) {
                blockFormat.setIndent(blockFormat.indent() + 1);
            }
            else {
                blockFormat.setIndent(std::max(blockFormat.indent() - 1, 0));
            }

            cursor.setBlockFormat(blockFormat);
        }
    }

    cursor.endEditBlock();
}

void QuteNoteTextEdit::keyPressEvent(QKeyEvent * pEvent)
{
    if ((pEvent->key() == Qt::Key_Enter) || (pEvent->key() == Qt::Key_Return))
    {
        // check whether current line is a horizontal line
        bool atHorizontalLine = false;
        QTextCursor cursor = QTextEdit::textCursor();
        cursor.beginEditBlock();
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
            cursor.endEditBlock();
            return;
        }
        else if (cursor.currentList())
        {
            QTextList * pList = cursor.currentList();
            // check whether cursor is on the last element of the list
            int numElements = pList->count();
            QTextBlock block = cursor.block();
            if ( (pList->itemNumber(block) == (numElements - 1)) &&
                 QString(block.text().trimmed()).isEmpty() )
            {
                QTextBlockFormat format;
                cursor.setBlockFormat(format);
                QTextEdit::setTextCursor(cursor);
                cursor.endEditBlock();
                return;
            }
        }

        cursor.endEditBlock();
    }
    else if (pEvent->key() == Qt::Key_Backspace)
    {
        QTextCursor cursor = QTextEdit::textCursor();
        cursor.beginEditBlock();
        if (cursor.selection().isEmpty() && cursor.currentList())
        {
            QTextList * pList = cursor.currentList();
            QTextBlock block = cursor.block();
            int itemNumber = pList->itemNumber(block) + 1;
            int numItems = pList->count();
            if ( (pList->format().style() == QTextListFormat::ListDecimal) &&
                 (itemNumber != numItems) &&
                 (QString(block.text().trimmed()).isEmpty()) )
            {
                // ordered list + not the last item + empty item, specialized logic is required:
                // 1. Count the number of lines to process further
                int nLinesToLastItem = 0;
                if (!QString(pList->itemText(pList->item(numItems - 1)).trimmed()).isEmpty())
                {
                    QTextCursor cursorCurrentPosition(cursor);
                    while(!cursorCurrentPosition.atEnd()) {
                        ++nLinesToLastItem;
                        cursorCurrentPosition.movePosition(QTextCursor::Down);
                        QTextBlock regularBlock = cursorCurrentPosition.block();
                        if (pList->itemNumber(regularBlock) == numItems) {
                            cursorCurrentPosition.movePosition(QTextCursor::Down);
                            break;
                        }
                    }
                }
                // 2. Remove all the items including the explicitly removed one and all the following ones
                // NOTE: the text is NOT deleted here, only the ordered list items
                for(int i = numItems; i >= itemNumber; --i) {
                    pList->removeItem(i);
                }
                // 3. Insert empty block of text at the position where the cursor was
                QTextBlockFormat format;
                format.setIndent(std::max(format.indent() - 1, 0));
                cursor.setBlockFormat(format);
                QTextEdit::setTextCursor(cursor);
                // 4. Fix indentation of the remaining contents of the list
                QTextCursor cursorIndentFixer(cursor);
                for(int i = 0; i < nLinesToLastItem; ++i) {
                    cursorIndentFixer.movePosition(QTextCursor::Down);
                    QTextBlockFormat format = cursorIndentFixer.block().blockFormat();
                    format.setIndent(format.indent() - 1);
                    cursorIndentFixer.mergeBlockFormat(format);
                }
                // 5. Create a new list and fill it with saved items
                cursor.movePosition(QTextCursor::Down);
                QTextList * pNewList = cursor.createList(QTextListFormat::ListDecimal);
                Q_CHECK_PTR(pNewList);

                for(int i = 1; i < nLinesToLastItem; ++i) {
                    cursor.movePosition(QTextCursor::Down);
                    if (cursor.currentList()) {
                        // increase indentation
                        QTextBlockFormat format = cursor.blockFormat();
                        format.setIndent(format.indent() + 1);
                        cursor.mergeBlockFormat(format);
                        continue;
                    }
                    else {
                        pNewList->add(cursor.block());
                    }
                }

                cursor.endEditBlock();
                return;
            }
        }

        cursor.endEditBlock();
    }
    else if (pEvent->key() == Qt::Key_Tab)
    {
        QTextCursor cursor = QTextEdit::textCursor();
        if (cursor.selection().isEmpty() && cursor.atBlockStart()) {
            changeIndentation(true);
            return;
        }
    }

    QTextEdit::keyPressEvent(pEvent);
}

void QuteNoteTextEdit::mousePressEvent(QMouseEvent * pEvent)
{
    QTextCursor cursor = cursorForPosition(pEvent->pos());
    QTextCharFormat format = cursor.charFormat();
    if (format.objectType() == TODO_CHKBOX_TXT_FMT_UNCHECKED)
    {
        QString checkboxCheckedImgFileName(":/format_text_elements/checkbox_checked.gif");
        QFile checkboxCheckedImgFile(checkboxCheckedImgFileName);
        if (!checkboxCheckedImgFile.exists()) {
            QMessageBox::warning(this, tr("Error Opening File"),
                                 tr("Could not open '%1'").arg(checkboxCheckedImgFileName));
        }

        QImage checkboxCheckedImg(checkboxCheckedImgFileName);
        format.setObjectType(TODO_CHKBOX_TXT_FMT_CHECKED);
        format.setProperty(TODO_CHKBOX_TXT_DATA_CHECKED, checkboxCheckedImg);

        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::WordUnderCursor);
        }

        cursor.mergeCharFormat(format);
        QTextEdit::mergeCurrentCharFormat(format);
    }
    else if (format.objectType() == TODO_CHKBOX_TXT_FMT_CHECKED)
    {
        format.setObjectType(TODO_CHKBOX_TXT_FMT_UNCHECKED);

        if (!cursor.hasSelection()) {
            cursor.select(QTextCursor::WordUnderCursor);
        }

        cursor.mergeCharFormat(format);
        QTextEdit::mergeCurrentCharFormat(format);
    }
    else {
        QTextEdit::mousePressEvent(pEvent);
    }
}

void QuteNoteTextEdit::mouseMoveEvent(QMouseEvent * pEvent)
{
    QTextCursor cursor = cursorForPosition(pEvent->pos());
    QTextCharFormat format = cursor.charFormat();
    if ( (format.objectType() == TODO_CHKBOX_TXT_FMT_CHECKED) ||
         (format.objectType() == TODO_CHKBOX_TXT_FMT_UNCHECKED) )
    {
        QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    }
    else {
        QApplication::restoreOverrideCursor();
    }

    QTextEdit::mouseMoveEvent(pEvent);
}

void QuteNoteTextEdit::dropImage(const QUrl & url, const QImage & image)
{
    if (!image.isNull())
    {
        QTextEdit::document()->addResource(QTextDocument::ImageResource, url, image);
        QTextEdit::textCursor().insertImage(url.toString());
    }
}

void QuteNoteTextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat & format)
{
    QTextCursor cursor = QTextEdit::textCursor();
    cursor.beginEditBlock();

    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
    }
    cursor.mergeCharFormat(format);
    cursor.clearSelection();
    QTextEdit::mergeCurrentCharFormat(format);

    cursor.endEditBlock();
}

const qute_note::Note * QuteNoteTextEdit::getNotePtr() const
{
    return m_pNote;
}

void QuteNoteTextEdit::setNote(const qute_note::Note & note)
{
    m_pNote = &note;
}

void QuteNoteTextEdit::noteRichTextToENML(QString & ENML) const
{
    qute_note::RichTextToENML(*this, ENML);
}
