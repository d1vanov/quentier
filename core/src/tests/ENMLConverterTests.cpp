#include "ENMLConverterTests.h"
#include <client/types/Note.h>
#include <client/enml/ENMLConverter.h>
#include <logging/QuteNoteLogger.h>

namespace qute_note {
namespace test {

bool convertSimpleNoteToHtmlAndBack(QString & error)
{
    Note testNote;
    testNote.setTitle("My test note");
    QString originalNoteContent = "<en-note>"
                                  "<span style=\"font-weight:bold;color:red;\">Here's some bold red text!</span>"
                                  "<div>Hickory, dickory, dock,</div>"
                                  "<div>The mouse ran up the clock.</div>"
                                  "<div>The clock struck one,</div>"
                                  "<div>The mouse ran down,</div>"
                                  "<div>Hickory, dickory, dock.</div>"
                                  "<div><br/></div>"
                                  "<div>-- Author unknown</div>"
                                  "</en-note>";
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

    // TODO: think of a better way to compare the note content - maybe using QXMLReader?
    if (testNote.content() != originalNoteContent) {
        error = "Note content after conversion to HTML and backwards doesn't match the original content";
        QNWARNING(error << ": original note content: " << originalNoteContent
                  << "\nNew note content: " << testNote.content());
        return false;
    }

    return true;
}

} // namespace test
} // namespace qute_note
