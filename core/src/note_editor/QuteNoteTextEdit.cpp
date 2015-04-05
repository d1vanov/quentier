#include "QuteNoteTextEdit.h"
#include "ToDoCheckboxTextObject.h"
#include <tools/QuteNoteCheckPtr.h>
#include <logging/QuteNoteLogger.h>
#include <QMimeData>
#include <QMouseEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextList>
#include <QTextDocumentFragment>
#include <QMessageBox>
#include <QApplication>
#include <QUrl>
#include <QImage>
#include <QBuffer>
#include <QImageReader>
#include <QMovie>

using namespace qevercloud;

QuteNoteTextEdit::QuteNoteTextEdit(QWidget * parent) :
    QTextEdit(parent),
    m_droppedImageCounter(0),
    m_converter()
{}

QuteNoteTextEdit::~QuteNoteTextEdit()
{}

bool QuteNoteTextEdit::canInsertFromMimeData(const QMimeData * source) const
{
    return (source->hasImage() || QTextEdit::canInsertFromMimeData(source));
}

void QuteNoteTextEdit::insertFromMimeData(const QMimeData * source)
{
    // TODO: add proper handling of filetypes
    if (source->hasImage())
    {
        insertImageOrMovie(*source);
    }
    else
    {
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
                QUTE_NOTE_CHECK_PTR(pNewList);

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
        format.setObjectType(TODO_CHKBOX_TXT_FMT_CHECKED);

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

    QTextCharFormat charFormat = cursor.charFormat();
    if ( (charFormat.objectType() == TODO_CHKBOX_TXT_FMT_CHECKED) ||
         (charFormat.objectType() == TODO_CHKBOX_TXT_FMT_UNCHECKED) )
    {
        QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    }
    else {
        QApplication::restoreOverrideCursor();
    }

    QTextEdit::mouseMoveEvent(pEvent);
}

void QuteNoteTextEdit::insertImage(const QUrl & url, const QImage & image)
{
    if (image.isNull()) {
        QNINFO("Null image detected, won't insert into document");
        return;
    }

    QTextEdit::document()->addResource(QTextDocument::ImageResource, url, image);
    QTextEdit::textCursor().insertImage(url.toString());
}

void QuteNoteTextEdit::insertMovie(const QUrl & url, QMovie & movie)
{
    if (!movie.isValid()) {
        QNINFO("Movie is not valid, won't insert it into document");
        return;
    }

    QImage currentImage = movie.currentImage();
    QTextEdit::document()->addResource(QTextDocument::ImageResource, url, currentImage);
    QTextEdit::textCursor().insertImage(currentImage);

    QObject::connect(&movie, SIGNAL(frameChanged(int)), this, SLOT(animate()));
    movie.start();
}

void QuteNoteTextEdit::insertImageOrMovie(const QMimeData & source)
{
    QImage image = qvariant_cast<QImage>(source.imageData());
    if (image.isNull()) {
        QNWARNING("Found null image in the input mime data even though it said it has image");
        return;
    }

    const QStringList availableImageFormats = source.formats();

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::ReadWrite);

    QString usedImageFormat;
    bool saved = false;
    if (availableImageFormats.isEmpty()) {
        QNINFO("Found no available image formats in input data, trying to save without format");
        saved = image.save(&buffer);
    }
    else
    {
        QStringList::const_iterator formatsEnd = availableImageFormats.constEnd();
        for(QStringList::const_iterator it = availableImageFormats.constBegin(); it != formatsEnd; ++it)
        {
            usedImageFormat = *it;
            if (!usedImageFormat.startsWith("image/")) {
                QNTRACE("Format " << usedImageFormat << " does not appear to be an image format, trying the next one");
                continue;
            }
            else {
                usedImageFormat.remove(0, 6);
                usedImageFormat = usedImageFormat.toUpper();
            }

            saved = image.save(&buffer, usedImageFormat.toLocal8Bit().data());
            if (saved) {
                break;
            }

            QNDEBUG("Can't save the image to buffer using format " << usedImageFormat
                    << ", trying the next one");
        }

        if (!saved)
        {
            usedImageFormat.clear();
            QNDEBUG("Wasn't able to save the image with any of the available formats, "
                    "will try to save without any format");

            saved = image.save(&buffer);
        }
    }

    if (!saved) {
        QNWARNING("Can't save the image into buffer, image format = "
                  << (usedImageFormat.isEmpty() ? "<empty>" : usedImageFormat));
        return;
    }

    QUrl url(QString("dropped_image_%1").arg(m_droppedImageCounter++));

    QImageReader imageReader(&buffer);
    if (imageReader.supportsAnimation() && (imageReader.imageCount() > 0)) {
        QNDEBUG("Input image supports animation, will insert QMovie based on this image");
        QMovie * pMovie = new QMovie(&buffer, QByteArray(), this);
        insertMovie(url, *pMovie);
    }
    else {
        QNDEBUG("Input image does not support animation, will insert QImage");
        insertImage(url, image);
    }
}

void QuteNoteTextEdit::insertToDoCheckbox(QTextCursor cursor, const bool checked)
{
    QTextCharFormat todoCheckboxCharFormat;
    if (checked) {
        todoCheckboxCharFormat.setObjectType(TODO_CHKBOX_TXT_FMT_CHECKED);
    }
    else {
        todoCheckboxCharFormat.setObjectType(TODO_CHKBOX_TXT_FMT_UNCHECKED);
    }

    cursor.beginEditBlock();

    cursor.insertText(QString(QChar::ObjectReplacementCharacter), todoCheckboxCharFormat);
    cursor.insertText(" ", QTextCharFormat());

    cursor.endEditBlock();

    setTextCursor(cursor);
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

bool QuteNoteTextEdit::noteRichTextToENML(QString & ENML, QString & errorDescription) const
{
    return m_converter.richTextToNoteContent(*this, ENML, errorDescription);
}

void QuteNoteTextEdit::insertCheckedToDoCheckboxAtCursor(QTextCursor cursor)
{
    insertToDoCheckbox(cursor, /* checked = */ true);
}

void QuteNoteTextEdit::insertUncheckedToDoCheckboxAtCursor(QTextCursor cursor)
{
    insertToDoCheckbox(cursor, /* checked = */ false);
}

void QuteNoteTextEdit::insertFixedWidthTable(const int rows, const int columns, const int fixedWidth)
{
    insertTable(rows, columns, /* fixed width flag = */ true, fixedWidth, /* relative width = */ 0.0);
}

void QuteNoteTextEdit::insertRelativeWidthTable(const int rows, const int columns, const double relativeWidth)
{
    insertTable(rows, columns, /* fixed width flag = */ false, /* fixed width = */ 0, relativeWidth);
}

void QuteNoteTextEdit::insertTable(const int rows, const int columns, const bool fixedWidthFlag,
                                   const int fixedWidth, const double relativeWidth)
{
    QTextCursor cursor = textCursor();

    QString approxColumnWidth;
    QString htmlTable = "<table width=\"";
    if (fixedWidthFlag) {
        htmlTable += QString::number(fixedWidth);
        htmlTable += "px\"";
        approxColumnWidth = QString::number(fixedWidth / std::max(columns, 0));
    }
    else {
        htmlTable += QString::number(static_cast<int>(relativeWidth));
        htmlTable += "%\"";
    }

    htmlTable += " border=\"1\" cellspacing=\"1\" cellpadding=\"1\">\n";
    for(int i = 0; i < rows; ++i)
    {
        htmlTable += "  <tr>\n";

        if (fixedWidthFlag)
        {
            for(int j = 0; j < columns; ++j) {
                htmlTable += "      <td width=\"";
                htmlTable += approxColumnWidth;
                htmlTable += "\"></td>\n";
            }
        }
        else
        {
            for(int j = 0; j < columns; ++j) {
                htmlTable += "      <td></td>\n";
            }
        }

        htmlTable += "  </tr>\n";
    }
    htmlTable += "</table>";

    QNDEBUG("Inserting html table: " << htmlTable);
    cursor.insertHtml(htmlTable);
}

void QuteNoteTextEdit::animate()
{
    if (isHidden()) {
        repaint();
    }
}
