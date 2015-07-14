#include "ENMLConverterTests.h"
#include <client/types/Note.h>
#include <client/enml/ENMLConverter.h>
#include <logging/QuteNoteLogger.h>
#include <QXmlStreamReader>

namespace qute_note {
namespace test {

bool convertNoteToHtmlAndBackImpl(const QString & noteContent, QString & error);
bool compareEnml(const QString & original, const QString & processed, QString & error);

bool convertSimpleNoteToHtmlAndBack(QString & error)
{
    QString noteContent = "<en-note>"
                          "<span style=\"font-weight:bold;color:red;\">Here's some bold red text!</span>"
                          "<div>Hickory, dickory, dock,</div>"
                          "<div>The mouse ran up the clock.</div>"
                          "<div>The clock struck one,</div>"
                          "<div>The mouse ran down,</div>"
                          "<div>Hickory, dickory, dock.</div>"
                          "<div><br/></div>"
                          "<div>-- Author unknown</div>"
                          "</en-note>";
    return convertNoteToHtmlAndBackImpl(noteContent, error);
}

bool convertNoteWithToDoTagsToHtmlAndBack(QString & error)
{
    QString noteContent = "<en-note>"
                          "<h1>Hello, world!</h1>"
                          "<div>Here's the note with some todo tags</div>"
                          "<en-todo/>An item that I haven't completed yet"
                          "<br/>"
                          "<en-todo checked=\"true\"/>A completed item"
                          "<br/>"
                          "<en-todo checked=\"false\"/>Another not yet completed item"
                          "</en-note>";
    return convertNoteToHtmlAndBackImpl(noteContent, error);
}

bool convertNoteToHtmlAndBackImpl(const QString & noteContent, QString & error)
{
    Note testNote;
    testNote.setTitle("My test note");
    QString originalNoteContent = noteContent;
    testNote.setContent(originalNoteContent);

    ENMLConverter converter;
    QString html;
    bool res = converter.noteContentToHtml(testNote, html, error);
    if (!res) {
        error.prepend("Unable to convert the note content to HTML: ");
        QNWARNING(error);
        return false;
    }

    res = converter.htmlToNoteContent(html, testNote, error);
    if (!res) {
        error.prepend("Unable to convert HTML to note content: ");
        QNWARNING(error);
        return false;
    }

    if (!testNote.hasContent()) {
        error = "Test note doesn't have its content set after the conversion from HTML";
        QNWARNING(error);
        return false;
    }

    res = compareEnml(originalNoteContent, testNote.content(), error);
    if (!res) {
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

        if (readerOriginal.isStartDocument() && !readerProcessed.isStartDocument()) {
            error = "QXmlStreamReader of the original ENML is at the start of the document while the reader of the processed ENML is not";
            QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
            return false;
        }

        if (readerOriginal.isStartElement())
        {
            if (!readerProcessed.isStartElement()) {
                error = "QXmlStreamReader of the original ENML is at the start of the element while the reader of the processed ENML is not";
                QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified
                          << "\ncurrent original token: " << readerOriginal.tokenString() << "\ncurrent processed token: "
                          << readerProcessed.tokenString() << "\nOriginal element text: " << readerOriginal.readElementText()
                          << "\nProcessed element text: " << readerProcessed.readElementText());
                return false;
            }

            QStringRef originalName = readerOriginal.name();
            QStringRef processedName = readerProcessed.name();
            if (originalName != processedName) {
                error = "Found a tag in the original ENML which name doesn't match the name of the corresponding element in the processed ENML";
                QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
                return false;
            }

            QXmlStreamAttributes originalAttributes = readerOriginal.attributes();
            QXmlStreamAttributes processedAttributes = readerProcessed.attributes();

            if (originalName == "en-todo")
            {
                bool originalChecked = false;
                if (originalAttributes.hasAttribute("checked")) {
                    QStringRef originalCheckedStr = originalAttributes.value("checked");
                    if (originalCheckedStr == "true") {
                        originalChecked = true;
                    }
                }

                bool processedChecked = false;
                if (processedAttributes.hasAttribute("checked")) {
                    QStringRef processedCheckedStr = processedAttributes.value("checked");
                    if (processedCheckedStr == "true") {
                        processedChecked = true;
                    }
                }

                if (originalChecked != processedChecked) {
                    error = "Checked state of ToDo item from the original ENML doesn't match the state of the item from "
                            "the processed ENML";
                    QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
                    return false;
                }
            }
            else
            {
                const int numOriginalAttributes = originalAttributes.size();
                const int numProcessedAttributes = processedAttributes.size();

                if (numOriginalAttributes != numProcessedAttributes) {
                    error = "The number of attributes in tag " + originalName.toString() + " doesn't match in the original and the processed ENMLs";
                    QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
                    return false;
                }

                for(int i = 0; i < numOriginalAttributes; ++i)
                {
                    const QXmlStreamAttribute & originalAttribute = originalAttributes[i];
                    const QXmlStreamAttribute & processedAttribute = processedAttributes[i];

                    if (originalAttribute != processedAttribute) {
                        error = "The corresponding attributes within the original and the processed ENMLs do not match";
                        QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified
                                  << ", index within attributes = " << i << "\nOriginal attribute: name = "
                                  << originalAttribute.name() << ", namespace uri = " << originalAttribute.namespaceUri()
                                  << ", qualified name = " << originalAttribute.qualifiedName() << ", is default = "
                                  << (originalAttribute.isDefault() ? "true" : "false") << ", value = " << originalAttribute.value()
                                  << ", prefix = " << originalAttribute.prefix() << "\nProcessed attribute: name = "
                                  << processedAttribute.name() << ", namespace uri = " << processedAttribute.namespaceUri()
                                  << ", qualified name = " << processedAttribute.qualifiedName() << ", is default = "
                                  << (processedAttribute.isDefault() ? "true" : "false") << ", value = " << processedAttribute.value()
                                  << ", prefix = " << processedAttribute.prefix());
                        return false;
                    }
                }
            }
        }

        if (readerOriginal.isEndElement() && !readerProcessed.isEndElement()) {
            error = "QXmlStreamReader of the original ENML is at the end of the element while the reader of the processed ENML is not";
            QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
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

                error = "QXmlStreamReader of the original ENML points to characters while the reader of the processed ENML does not";
                QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
                return false;
            }

            if (readerOriginal.isCDATA())
            {
                if (!readerProcessed.isCDATA()) {
                    error = "QXmlStreamReader of the original ENML points to CDATA while the reader of the processed ENML does not";
                    QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
                    return false;
                }
            }

            QString textOriginal = readerOriginal.text().toString().simplified();
            QString textProcessed = readerProcessed.text().toString().simplified();

            if (textOriginal != textProcessed) {
                error = "The text extracted from the corresponding elements of both the original ENML and the processed ENML does not match";
                QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified
                          << "\nOriginal element text: " << textOriginal << "\nProcessed element text: " << textProcessed);
                return false;
            }
        }

        if (readerOriginal.isEndDocument() && !readerProcessed.isEndDocument()) {
            error = "QXmlStreamReader of the original ENML is at the end of the document while the reader of the processed ENML is not";
            QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
            return false;
        }
    }

    if (readerOriginal.atEnd() != readerProcessed.atEnd()) {
        error = "QXmlStreamReaders for the original ENML and the processed ENML have not both came to their ends after the checking loop";
        QNWARNING(error << "; original ENML: " << originalSimplified << "\nProcessed ENML: " << processedSimplified);
        return false;
    }

    return true;
}

} // namespace test
} // namespace qute_note
