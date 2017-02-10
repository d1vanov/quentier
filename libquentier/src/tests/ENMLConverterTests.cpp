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

#include "ENMLConverterTests.h"
#include <quentier/enml/ENMLConverter.h>
#include <quentier/note_editor/DecryptedTextManager.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/types/ErrorString.h>
#include <QXmlStreamReader>
#include <QFile>

void initENMLConversionTestResources();

namespace quentier {
namespace test {

bool convertNoteToHtmlAndBackImpl(const QString & noteContent, DecryptedTextManager & decryptedTextManager, QString & error);
bool compareEnml(const QString & original, const QString & processed, QString & error);

bool convertSimpleNoteToHtmlAndBack(QString & error)
{
    QString noteContent = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                         "<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">"
                                         "<en-note>"
                                         "<span style=\"font-weight:bold;color:red;\">Here's some bold red text!</span>"
                                         "<div>Hickory, dickory, dock,</div>"
                                         "<div>The mouse ran up the clock.</div>"
                                         "<div>The clock struck one,</div>"
                                         "<div>The mouse ran down,</div>"
                                         "<div>Hickory, dickory, dock.</div>"
                                         "<div><br/></div>"
                                         "<div>-- Author unknown</div>"
                                         "</en-note>");
    DecryptedTextManager decryptedTextManager;
    return convertNoteToHtmlAndBackImpl(noteContent, decryptedTextManager, error);
}

bool convertNoteWithToDoTagsToHtmlAndBack(QString & error)
{
    QString noteContent = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                         "<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">"
                                         "<en-note>"
                                         "<h1>Hello, world!</h1>"
                                         "<div>Here's the note with some todo tags</div>"
                                         "<en-todo/>An item that I haven't completed yet"
                                         "<br/>"
                                         "<en-todo checked=\"true\"/>A completed item"
                                         "<br/>"
                                         "<en-todo checked=\"false\"/>Another not yet completed item"
                                         "</en-note>");
    DecryptedTextManager decryptedTextManager;
    return convertNoteToHtmlAndBackImpl(noteContent, decryptedTextManager, error);
}

bool convertNoteWithEncryptionToHtmlAndBack(QString & error)
{
    QString noteContent = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                         "<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">"
                                         "<en-note>"
                                         "<h3>This note contains encrypted text</h3>"
                                         "<br/>"
                                         "<div>Here's the encrypted text containing only the hint attribute</div>"
                                         "<en-crypt hint=\"this is my rifle, this is my gun\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                         "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                         "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                         "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                         "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                         "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                         "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                                         "<br/><div>Here's the encrypted text containing only the cipher attribute</div>"
                                         "<en-crypt cipher=\"AES\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                         "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                         "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                         "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                         "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                         "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                         "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                                         "<br/><div>Here's the encrypted text containing only the length attribute</div>"
                                         "<en-crypt length=\"128\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                         "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                         "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                         "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                         "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                         "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                         "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                                         "<br/><div>Here's the encrypted text containing cipher and length attributes</div>"
                                         "<en-crypt cipher=\"AES\" length=\"128\">RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                         "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                         "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                         "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                         "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                         "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                         "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                                         "<br/><div>Here's the encrypted text containing cipher and hint attributes</div>"
                                         "<en-crypt hint=\"this is my rifle, this is my gun\" cipher=\"AES\">"
                                         "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                         "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                         "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                         "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                         "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                         "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                         "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                                         "<br/><div>Here's the encrypted text containing length and hint attributes</div>"
                                         "<en-crypt hint=\"this is my rifle, this is my gun\" length=\"128\">"
                                         "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                         "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                         "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                         "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                         "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                         "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                         "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                                         "<br/><div>Here's the encrypted text containing cipher, length and hint attributes</div>"
                                         "<en-crypt hint=\"this is my rifle, this is my gun\" cipher=\"AES\" length=\"128\">"
                                         "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                         "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                         "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                         "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                         "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                         "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                         "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt>"
                                         "<div>Here's the text encrypted with RC2 which should reside in decrypted text cache</div>"
                                         "<en-crypt hint=\"my_own_encryption_key_1988\">"
                                         "K+sUXSxI2Mt075+pSDxR/gnCNIEnk5XH1P/D0Eie17"
                                         "JIWgGnNo5QeMo3L0OeBORARGvVtBlmJx6vJY2Ij/2En"
                                         "MVy6/aifSdZXAxRlfnTLvI1IpVgHpTMzEfy6zBVMo+V"
                                         "Bt2KglA+7L0iSjA0hs3GEHI6ZgzhGfGj</en-crypt>"
                                         "<div>Here's the text encrypted with AES which should reside in decrypted text cache</div>"
                                         "<en-crypt hint=\"MyEncryptionPassword\">"
                                         "RU5DMBwXjfKR+x9ksjSJhtiF+CxfwXn2Hf/WqdVwLwJDX9YX5R34Z5SBMSCIOFF"
                                         "r1MUeNkzHGVP5fHEppUlIExDG/Vpjh9KK1uu0VqTFoUWA0IXAAMA5eHnbxhBrjvL"
                                         "3CoTQV7prRqJVLpUX77Q0vbNims1quxVWaf7+uVeK60YoiJnSOHvEYptoOs1FVfZ"
                                         "AwnDDBoCUOsAb2nCh2UZ6LSFneb58xQ/6WeoQ7QDDHLSoUIXn</en-crypt>"
                                         "</en-note>");
    DecryptedTextManager decryptedTextManager;
    decryptedTextManager.addEntry(QStringLiteral("K+sUXSxI2Mt075+pSDxR/gnCNIEnk5XH1P/D0Eie17"
                                                 "JIWgGnNo5QeMo3L0OeBORARGvVtBlmJx6vJY2Ij/2En"
                                                 "MVy6/aifSdZXAxRlfnTLvI1IpVgHpTMzEfy6zBVMo+V"
                                                 "Bt2KglA+7L0iSjA0hs3GEHI6ZgzhGfGj"),
                                  QStringLiteral("<span style=\"display: inline !important; float: none; \">"
                                                 "Ok, here's a piece of text I'm going to encrypt now</span>"),
                                  /* remember for session = */ true,
                                  QStringLiteral("my_own_encryption_key_1988"), QStringLiteral("RC2"), 64);
    decryptedTextManager.addEntry(QStringLiteral("RU5DMBwXjfKR+x9ksjSJhtiF+CxfwXn2Hf/WqdVwLwJDX9YX5R34Z5SBMSCIOFF"
                                                 "r1MUeNkzHGVP5fHEppUlIExDG/Vpjh9KK1uu0VqTFoUWA0IXAAMA5eHnbxhBrjvL"
                                                 "3CoTQV7prRqJVLpUX77Q0vbNims1quxVWaf7+uVeK60YoiJnSOHvEYptoOs1FVfZ"
                                                 "AwnDDBoCUOsAb2nCh2UZ6LSFneb58xQ/6WeoQ7QDDHLSoUIXn"),
                                  QStringLiteral("Sample text said to be the decrypted one"),
                                  /* remember for session = */ true,
                                  QStringLiteral("MyEncryptionPassword"), QStringLiteral("AES"), 128);
    return convertNoteToHtmlAndBackImpl(noteContent, decryptedTextManager, error);
}

bool convertNoteWithResourcesToHtmlAndBack(QString & error)
{
    QString noteContent = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                         "<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">"
                                         "<en-note>"
                                         "<div>Here's the note with some embedded resources</div>"
                                         "<br/>"
                                         "<div>The first resource: simple image</div>"
                                         "<en-media width=\"640\" height=\"480\" align=\"right\" type=\"image/jpeg\" hash=\"f03c1c2d96bc67eda02968c8b5af9008\"/>"
                                         "<div>The second resource: embedded pdf</div>"
                                         "<en-media width=\"600\" height=\"800\" title=\"My cool pdf\" type=\"application/pdf\" hash=\"6051a24c8677fd21c65c1566654c228\"/>"
                                         "</en-note>");
    DecryptedTextManager decryptedTextManager;
    return convertNoteToHtmlAndBackImpl(noteContent, decryptedTextManager, error);
}

bool convertComplexNoteToHtmlAndBack(QString & error)
{
    initENMLConversionTestResources();

    QFile file(QStringLiteral(":/tests/complexNote1.txt"));
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Can't open the resource with complex note #1 for reading");
        return false;
    }

    QString noteContent = file.readAll();
    DecryptedTextManager decryptedTextManager;
    return convertNoteToHtmlAndBackImpl(noteContent, decryptedTextManager, error);
}

bool convertComplexNote2ToHtmlAndBack(QString & error)
{
    initENMLConversionTestResources();

    QFile file(QStringLiteral(":/tests/complexNote2.txt"));
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Can't open the resource with complex note #2 for reading");
        return false;
    }

    QString noteContent = file.readAll();
    DecryptedTextManager decryptedTextManager;
    return convertNoteToHtmlAndBackImpl(noteContent, decryptedTextManager, error);
}

bool convertComplexNote3ToHtmlAndBack(QString & error)
{
    initENMLConversionTestResources();

    QFile file(QStringLiteral(":/tests/complexNote3.txt"));
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Can't open the resource with complex note #3 for reading");
        return false;
    }

    QString noteContent = file.readAll();
    DecryptedTextManager decryptedTextManager;
    return convertNoteToHtmlAndBackImpl(noteContent, decryptedTextManager, error);
}

bool convertComplexNote4ToHtmlAndBack(QString &error)
{
    initENMLConversionTestResources();

    QFile file(QStringLiteral(":/tests/complexNote4.txt"));
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Can't open the resource with complex note #4 for reading");
        return false;
    }

    QString noteContent = file.readAll();
    DecryptedTextManager decryptedTextManager;
    return convertNoteToHtmlAndBackImpl(noteContent, decryptedTextManager, error);
}

bool convertNoteToHtmlAndBackImpl(const QString & noteContent, DecryptedTextManager & decryptedTextManager, QString & error)
{
    QString originalNoteContent = noteContent;

    ENMLConverter converter;
    QString html;
    ENMLConverter::NoteContentToHtmlExtraData extraData;
    ErrorString errorMessage;
    bool res = converter.noteContentToHtml(originalNoteContent, html, errorMessage, decryptedTextManager, extraData);
    if (!res) {
        error = QStringLiteral("Unable to convert the note content to HTML: ");
        error += errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    html.prepend(QStringLiteral("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
                                "<html><head>"
                                "<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"UTF-8\" />"
                                "<title></title></head>"));
    html.append(QStringLiteral("</html>"));

    QString processedNoteContent;
    errorMessage.clear();
    res = converter.htmlToNoteContent(html, processedNoteContent, decryptedTextManager, errorMessage);
    if (!res) {
        error = QStringLiteral("Unable to convert HTML to note content: ");
        error += errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    res = compareEnml(originalNoteContent, processedNoteContent, error);
    if (!res) {
        QNWARNING(QStringLiteral("\n\nHTML: ") << html);
        return false;
    }

    return true;
}

bool compareEnml(const QString & original, const QString & processed, QString & error)
{
    QString originalSimplified = original.simplified();
    QString processedSimplified = processed.simplified();

    QXmlStreamReader readerOriginal(originalSimplified);
    QXmlStreamReader readerProcessed(processedSimplified);

#define PRINT_WARNING(err) \
    QNWARNING(err << QStringLiteral("\n\nContext in the original ENML: <") << readerOriginal.name() << QStringLiteral(">: ")  \
              << readerOriginal.readElementText() << QStringLiteral("\n\nContext in the processed ENML: <") << readerProcessed.name() \
              << QStringLiteral(">: ") << readerProcessed.readElementText() << QStringLiteral("\n\nFull simplified original ENML: ") \
              << originalSimplified << QStringLiteral("\n\nFull simplified processed ENML: ") << processedSimplified);

    while(!readerOriginal.atEnd() && !readerProcessed.atEnd())
    {
        Q_UNUSED(readerOriginal.readNext());
        Q_UNUSED(readerProcessed.readNext());

        bool checkForEmptyCharacters = true;
        while(checkForEmptyCharacters)
        {
            if (readerOriginal.isCharacters())
            {
                QString textOriginal = readerOriginal.readElementText();
                if (textOriginal.simplified().isEmpty()) {
                    Q_UNUSED(readerOriginal.readNext());
                    continue;
                }
            }

            checkForEmptyCharacters = false;
        }

        checkForEmptyCharacters = true;
        while(checkForEmptyCharacters)
        {
            if (readerProcessed.isCharacters())
            {
                QString textProcessed = readerProcessed.readElementText();
                if (textProcessed.simplified().isEmpty()) {
                    Q_UNUSED(readerProcessed.readNext());
                    continue;
                }
            }

            checkForEmptyCharacters = false;
        }

        bool checkForEntityReference = true;
        while(checkForEntityReference)
        {
            if (readerOriginal.isEntityReference()) {
                Q_UNUSED(readerOriginal.readNext());
                continue;
            }
            else {
                checkForEntityReference = false;
            }
        }

        if (readerOriginal.isStartDocument() && !readerProcessed.isStartDocument()) {
            error = QStringLiteral("QXmlStreamReader of the original ENML is at the start of the document "
                                   "while the reader of the processed ENML is not");
            PRINT_WARNING(error);
            return false;
        }

        if (readerOriginal.isStartElement())
        {
            if (!readerProcessed.isStartElement()) {
                error = QStringLiteral("QXmlStreamReader of the original ENML is at the start of the element "
                                       "while the reader of the processed ENML is not");
                PRINT_WARNING(error << QStringLiteral("\n\nchecking the state of processed ENML reader: ")
                              << QStringLiteral("isStartDocument: ") << (readerProcessed.isStartDocument() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isDTD: ") << (readerProcessed.isDTD() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isCDATA: ") << (readerProcessed.isCDATA() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isCharacters: ") << (readerProcessed.isCharacters() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isComment: ") << (readerProcessed.isComment() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isEndElement: ") << (readerProcessed.isEndElement() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isEndDocument: ") << (readerProcessed.isEndDocument() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isEntityReference: ") << (readerProcessed.isEntityReference() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isProcessingInstruction: ") << (readerProcessed.isProcessingInstruction() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isStandaloneDocument: ") << (readerProcessed.isStandaloneDocument() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isStartDocument: ") << (readerProcessed.isStartDocument() ? QStringLiteral("true") : QStringLiteral("false"))
                              << QStringLiteral(", isWhitespace: ") << (readerProcessed.isWhitespace() ? QStringLiteral("true") : QStringLiteral("false")));
                return false;
            }

            QStringRef originalName = readerOriginal.name();
            QStringRef processedName = readerProcessed.name();
            if (originalName != processedName) {
                error = QStringLiteral("Found a tag in the original ENML which name doesn't match the name "
                                       "of the corresponding element in the processed ENML");
                PRINT_WARNING(error);
                return false;
            }

            QXmlStreamAttributes originalAttributes = readerOriginal.attributes();
            QXmlStreamAttributes processedAttributes = readerProcessed.attributes();

            if (originalName == QStringLiteral("en-todo"))
            {
                bool originalChecked = false;
                if (originalAttributes.hasAttribute(QStringLiteral("checked"))) {
                    QStringRef originalCheckedStr = originalAttributes.value(QStringLiteral("checked"));
                    if (originalCheckedStr == QStringLiteral("true")) {
                        originalChecked = true;
                    }
                }

                bool processedChecked = false;
                if (processedAttributes.hasAttribute(QStringLiteral("checked"))) {
                    QStringRef processedCheckedStr = processedAttributes.value(QStringLiteral("checked"));
                    if (processedCheckedStr == "true") {
                        processedChecked = true;
                    }
                }

                if (originalChecked != processedChecked) {
                    error = QStringLiteral("Checked state of ToDo item from the original ENML doesn't match the state of the item from "
                                           "the processed ENML");
                    PRINT_WARNING(error);
                    return false;
                }
            }
            else if (originalName == "td")
            {
                const int numOriginalAttributes = originalAttributes.size();
                const int numProcessedAttributes = processedAttributes.size();

                if (numOriginalAttributes != numProcessedAttributes) {
                    error = QStringLiteral("The number of attributes in tag ") + originalName.toString() +
                            QStringLiteral(" doesn't match in the original and the processed ENMLs");
                    PRINT_WARNING(error);
                    return false;
                }

                for(int i = 0; i < numOriginalAttributes; ++i)
                {
                    const QXmlStreamAttribute & originalAttribute = originalAttributes[i];
                    if (originalAttribute.name() == QStringLiteral("style")) {
                        QNTRACE(QStringLiteral("Won't compare the style attribute for td tag as it's known to be slightly "
                                               "modified by the web engine so it's just not easy to compare it"));
                        continue;
                    }

                    if (!processedAttributes.contains(originalAttribute)) {
                        error = QStringLiteral("The corresponding attributes within the original and the processed ENMLs do not match");
                        QNWARNING(error << QStringLiteral(": the original attribute was not found within the processed attributes; ")
                                  << QStringLiteral("original ENML: ") << originalSimplified << QStringLiteral("\nProcessed ENML: ")
                                  << processedSimplified << QStringLiteral(", index within attributes = ") << i << QStringLiteral("\nOriginal attribute: name = ")
                                  << originalAttribute.name() << QStringLiteral(", namespace uri = ") << originalAttribute.namespaceUri()
                                  << QStringLiteral(", qualified name = ") << originalAttribute.qualifiedName() << QStringLiteral(", is default = ")
                                  << (originalAttribute.isDefault() ? QStringLiteral("true") : QStringLiteral("false")) << QStringLiteral(", value = ")
                                  << originalAttribute.value() << QStringLiteral(", prefix = ") << originalAttribute.prefix());
                        return false;
                    }
                }
            }
            else
            {
                const int numOriginalAttributes = originalAttributes.size();
                const int numProcessedAttributes = processedAttributes.size();

                if (numOriginalAttributes != numProcessedAttributes) {
                    error = QStringLiteral("The number of attributes in tag ") + originalName.toString() +
                            QStringLiteral(" doesn't match in the original and the processed ENMLs");
                    PRINT_WARNING(error);
                    return false;
                }

                for(int i = 0; i < numOriginalAttributes; ++i)
                {
                    const QXmlStreamAttribute & originalAttribute = originalAttributes[i];

                    if (!processedAttributes.contains(originalAttribute)) {
                        error = QStringLiteral("The corresponding attributes within the original and the processed ENMLs do not match");
                        QNWARNING(error << QStringLiteral(": the original attribute was not found within the processed attributes; ")
                                  << QStringLiteral("original ENML: ") << originalSimplified << QStringLiteral("\nProcessed ENML: ")
                                  << processedSimplified << QStringLiteral(", index within attributes = ") << i << QStringLiteral("\nOriginal attribute: name = ")
                                  << originalAttribute.name() << QStringLiteral(", namespace uri = ") << originalAttribute.namespaceUri()
                                  << QStringLiteral(", qualified name = ") << originalAttribute.qualifiedName() << QStringLiteral(", is default = ")
                                  << (originalAttribute.isDefault() ? QStringLiteral("true") : QStringLiteral("false"))
                                  << QStringLiteral(", value = ") << originalAttribute.value() << QStringLiteral(", prefix = ")
                                  << originalAttribute.prefix());
                        return false;

                    }
                }
            }
        }

        if (readerOriginal.isEndElement() && !readerProcessed.isEndElement()) {
            error = QStringLiteral("QXmlStreamReader of the original ENML is at the end of the element while "
                                   "the reader of the processed ENML is not");
            PRINT_WARNING(error);
            return false;
        }

        if (readerOriginal.isCharacters())
        {
            if (!readerProcessed.isCharacters())
            {
                QStringRef textOriginal = readerOriginal.text();
                if (textOriginal.toString().simplified().isEmpty()) {
                    continue;
                }

                error = QStringLiteral("QXmlStreamReader of the original ENML points to characters "
                                       "while the reader of the processed ENML does not");
                QNWARNING(error << QStringLiteral("; original ENML: ") << originalSimplified << QStringLiteral("\nProcessed ENML: ")
                          << processedSimplified);
                return false;
            }

            if (readerOriginal.isCDATA())
            {
                if (!readerProcessed.isCDATA()) {
                    error = QStringLiteral("QXmlStreamReader of the original ENML points to CDATA while the reader "
                                           "of the processed ENML does not");
                    QNWARNING(error << QStringLiteral("; original ENML: ") << originalSimplified << QStringLiteral("\nProcessed ENML: ")
                              << processedSimplified);
                    return false;
                }
            }

            QString textOriginal = readerOriginal.text().toString().simplified();
            QString textProcessed = readerProcessed.text().toString().simplified();

            if (textOriginal != textProcessed) {
                error = QStringLiteral("The text extracted from the corresponding elements of both the original ENML "
                                       "and the processed ENML does not match");
                QNWARNING(error << QStringLiteral("; original ENML: ") << originalSimplified << QStringLiteral("\nProcessed ENML: ")
                          << processedSimplified << QStringLiteral("\nOriginal element text: ") << textOriginal
                          << QStringLiteral("\nProcessed element text: ") << textProcessed);
                return false;
            }
        }

        if (readerOriginal.isEndDocument() && !readerProcessed.isEndDocument()) {
            error = QStringLiteral("QXmlStreamReader of the original ENML is at the end of the document while the reader of the processed ENML is not");
            QNWARNING(error << QStringLiteral("; original ENML: ") << originalSimplified << QStringLiteral("\nProcessed ENML: ") << processedSimplified);
            return false;
        }
    }

    if (readerOriginal.atEnd() != readerProcessed.atEnd()) {
        error = QStringLiteral("QXmlStreamReaders for the original ENML and the processed ENML have not both came to their ends after the checking loop");
        QNWARNING(error << QStringLiteral("; original ENML: ") << originalSimplified << QStringLiteral("\nProcessed ENML: ") << processedSimplified);
        return false;
    }

    return true;
}

bool convertHtmlWithModifiedDecryptedTextToEnml(QString & error)
{
    QString originalENML = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                          "<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">"
                                          "<en-note>"
                                          "<h3>This note contains encrypted text</h3>"
                                          "<br/>"
                                          "<div>Here's the encrypted text containing only the hint attribute</div>"
                                          "<en-crypt hint=\"this is my rifle, this is my gun\">"
                                          "RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                          "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                          "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                          "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                          "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                          "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                          "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E=</en-crypt></en-note>");

    QString expectedReturnENML = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                                "<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">"
                                                "<en-note>"
                                                "<h3>This note contains encrypted text</h3>"
                                                "<br/>"
                                                "<div>Here's the encrypted text containing only the hint attribute</div>"
                                                "<en-crypt hint=\"this is my rifle, this is my gun\">"
                                                "RU5DMGurZ1T6vW4yZ3HNWU7ymzW/xBXG6c7AomFCebMy2R57ekc/kT94"
                                                "6aGUAJxSsCsWmX39wsyqGBG03dqSiVKw6lyHHEY0qA/r9M8fGYaiOugS"
                                                "OQRmyS7dN+k8hdJWsTwes/L1Q6d89J4nJX1cVV8rwVsr0/dsC/GIeIcV"
                                                "Or1PLyRjFieujCKah4Tlm7svcYFwruXtpyng4TjEnspT5AtWYGxQCY8L"
                                                "WhLRhf8b4ONmC+PEDpw5KazuMDnRQml56j5JcezD/kKHWKIp9cUAeOAb"
                                                "zIkoFDorRc/Yg777olCNLHYS2shkD8PrWOZTF2D1L1VP5vNBnSFS5QFXT"
                                                "+tkQfd1yark0//ID+PtiCroPCO8ayqJynqlmPkzHA6Z6FuGwwXBJFE6K7"
                                                "vFOmbMWfEp235c1eVb5QbvbLjPujjQsWrVpS1qyTOHoTHpiBeFtTZ8ka2"
                                                "FytyHtv7YdoR8M6sfR+20xWgfnbdO87iOn2Fec/lvLvRohl68QhwCuKRL"
                                                "0IZUuDKjm/OWiGzoEzCTvmz3m5x89dAIUJ10lV56uhGTp9of+yPI"
                                                "</en-crypt></en-note>");

    DecryptedTextManager decryptedTextManager;
    decryptedTextManager.addEntry(QStringLiteral("RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                                 "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                                 "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                                 "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                                 "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                                 "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                                 "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E="),
                                  QStringLiteral("<span style=\"display: inline !important; float: none; \">"
                                                 "Ok, here's some really long text. I can type and type it "
                                                 "on and on and it will not stop any time soon just yet. "
                                                 "The password is going to be long also.</span>"),
                                  /* remember for session = */ false,
                                  QStringLiteral("thisismyriflethisismygunthisisforfortunethisisforfun"),
                                  QStringLiteral("AES"), 128);
    // NOTE: the text in decrypted text cache is different from that in html meaning that encrypted text was modified;
    // The conversion to ENML should re-calculate the hash of the encrypted text
    // and ensure this entry is present in decrypted text cache if this decryption needs to be remembered for the whole session

    QString html;
    ENMLConverter converter;
    ENMLConverter::NoteContentToHtmlExtraData extraData;
    ErrorString errorMessage;
    bool res = converter.noteContentToHtml(originalENML, html, errorMessage, decryptedTextManager, extraData);
    if (!res) {
        error = QStringLiteral("Unable to convert the note content to HTML: ");
        error += errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    html.prepend(QStringLiteral("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
                                "<html><head>"
                                "<meta http-equiv=\"Content-Type\" content=\"text/html\" charset=\"UTF-8\" />"
                                "<title></title></head>"));
    html.append(QStringLiteral("</html>"));

    decryptedTextManager.addEntry(QStringLiteral("RU5DMI1mnQ7fKjBk9f0a57gSc9Nfbuw3uuwMKs32Y+wJGLZa0N8PcTzf7pu3"
                                                 "/2VOBqZMvfkKGh4mnJuGy45ZT2TwOfqt+ey8Tic7BmhGg7b4n+SpJFHntkeL"
                                                 "glxFWJt6oIG14i7IpamIuYyE5XcBRkOQs2cr7rg730d1hxx6sW/KqIfdr+0rF4k"
                                                 "+rqP7tpI5ha/ALkhaZAuDbIVic39aCRcu6uve6mHHHPA03olCbi7ePVwO7e94mp"
                                                 "uvcg2lGTJyDw/NoZmjFycjXESRJgLIr+gGfyD17jYNGcPBLR8Rb0M9vGK1tG9haG"
                                                 "+Vem1pTWgRfYXF70mMduEmAd4xXy1JqV6XNUYDddW9iPpffWTZgD409LK9wIZM5C"
                                                 "W2rbM2lwM/R0IEnoK7N5X8lCOzqkA9H/HF+8E="),
                                  QStringLiteral("<span style=\"display: inline !important; float: none; \">"
                                                 "Ok, here's some really long text. I can type and type it "
                                                 "on and on and it will not stop any time soon just yet. "
                                                 "The password is going to be long also. And I can <b>edit</b> "
                                                 "it as well without too much trying.&nbsp;"
                                                 "</span>"),
                                  /* remember for session = */ false,
                                  QStringLiteral("thisismyriflethisismygunthisisforfortunethisisforfun"),
                                  QStringLiteral("AES"), 128);
    // NOTE: specifying the entry with the same hash as above but with different decrypted text
    // to trigger the re-evaluation of decrypted text hash in the following conversion

    QString processedENML;
    errorMessage.clear();
    res = converter.htmlToNoteContent(html, processedENML, decryptedTextManager, errorMessage);
    if (!res) {
        error = QStringLiteral("Unable to convert HTML to note content: ");
        error += errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    res = compareEnml(processedENML, expectedReturnENML, error);
    if (!res) {
        QNWARNING("\n\nHTML: " << html);
        return false;
    }

    return true;
}

bool convertHtmlWithTableHelperTagsToEnml(QString & error)
{
    QFile file(QStringLiteral(":/tests/complexNote3.txt"));
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Can't open the resource with complex note #3 for reading");
        return false;
    }

    QString noteContent = file.readAll();
    file.close();

    file.setFileName(QStringLiteral(":/tests/complexNoteHtmlWithHelperTags.txt"));
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Can't open the resource with complex note html with helper tags for reading");
        return false;
    }

    QString html = file.readAll();
    file.close();

    ENMLConverter converter;
    DecryptedTextManager decryptedTextManager;

    ENMLConverter::SkipHtmlElementRule skipRule;
    skipRule.m_attributeValueToSkip = QStringLiteral("JCLRgrip");
    skipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::StartsWith;
    skipRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;

    QVector<ENMLConverter::SkipHtmlElementRule> skipRules;
    skipRules << skipRule;

    QString processedNoteContent;
    ErrorString errorMessage;
    bool res = converter.htmlToNoteContent(html, processedNoteContent, decryptedTextManager, errorMessage, skipRules);
    if (!res) {
        error = QStringLiteral("Unable to convert HTML to note content: ");
        error += errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    res = compareEnml(noteContent, processedNoteContent, error);
    if (!res) {
        QNWARNING(QStringLiteral("\n\nHTML: ") << html);
        return false;
    }

    return true;
}

bool convertHtmlWithTableAndHilitorHelperTagsToEnml(QString & error)
{
    QFile file(QStringLiteral(":/tests/complexNote3.txt"));
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Can't open the resource with complex note #3 for reading");
        return false;
    }

    QString noteContent = file.readAll();
    file.close();

    file.setFileName(QStringLiteral(":/tests/complexNoteHtmlWithTableAndHilitorHelperTags.txt"));
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Can't open the resource with complex note html with helper tags for reading");
        return false;
    }

    QString html = file.readAll();
    file.close();

    ENMLConverter converter;
    DecryptedTextManager decryptedTextManager;

    ENMLConverter::SkipHtmlElementRule tableSkipRule;
    tableSkipRule.m_attributeValueToSkip = QStringLiteral("JCLRgrip");
    tableSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::StartsWith;
    tableSkipRule.m_attributeValueCaseSensitivity = Qt::CaseSensitive;

    ENMLConverter::SkipHtmlElementRule hilitorSkipRule;
    hilitorSkipRule.m_includeElementContents = true;
    hilitorSkipRule.m_attributeValueToSkip = QStringLiteral("hilitorHelper");
    hilitorSkipRule.m_attributeValueCaseSensitivity = Qt::CaseInsensitive;
    hilitorSkipRule.m_attributeValueComparisonRule = ENMLConverter::SkipHtmlElementRule::Contains;

    QVector<ENMLConverter::SkipHtmlElementRule> skipRules;
    skipRules << tableSkipRule;
    skipRules << hilitorSkipRule;

    QString processedNoteContent;
    ErrorString errorMessage;
    bool res = converter.htmlToNoteContent(html, processedNoteContent, decryptedTextManager, errorMessage, skipRules);
    if (!res) {
        error = QStringLiteral("Unable to convert HTML to note content: ");
        error += errorMessage.nonLocalizedString();
        QNWARNING(error);
        return false;
    }

    res = compareEnml(noteContent, processedNoteContent, error);
    if (!res) {
        QNWARNING(QStringLiteral("\n\nHTML: ") << html);
        return false;
    }

    return true;
}

} // namespace test
} // namespace quentier

void initENMLConversionTestResources()
{
    Q_INIT_RESOURCE(test_resources);
}

