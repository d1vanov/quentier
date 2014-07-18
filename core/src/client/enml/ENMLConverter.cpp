#include "ENMLConverter.h"
#include <note_editor/QuteNoteTextEdit.h>
#include <tools/QuteNoteCheckPtr.h>
#include <QEverCloud.h>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QDomDocument>
#include <QMimeData>
#include <QDebug>

namespace qute_note {

ENMLConverter::ENMLConverter()
{
    fillTagsLists();
}

ENMLConverter::ENMLConverter(const ENMLConverter & other) :
    m_forbiddenXhtmlTags(other.m_forbiddenXhtmlTags),
    m_forbiddenXhtmlAttributes(other.m_forbiddenXhtmlAttributes),
    m_evernoteSpecificXhtmlTags(other.m_evernoteSpecificXhtmlTags),
    m_allowedXhtmlTags(other.m_allowedXhtmlTags)
{}

ENMLConverter & ENMLConverter::operator=(const ENMLConverter & other)
{
    if (this != &other) {
        m_forbiddenXhtmlTags = other.m_forbiddenXhtmlTags;
        m_forbiddenXhtmlAttributes = other.m_forbiddenXhtmlAttributes;
        m_evernoteSpecificXhtmlTags = other.m_evernoteSpecificXhtmlTags;
        m_allowedXhtmlTags = other.m_allowedXhtmlTags;
    }

    return *this;
}

bool ENMLConverter::richTextToNoteContent(const QuteNoteTextEdit & noteEditor,
                                          QString & ENML, QString & errorDescription) const
{
    const QTextDocument * pNoteDoc = noteEditor.document();
    QUTE_NOTE_CHECK_PTR(pNoteDoc, QT_TR_NOOP("Null QTextDocument pointer received from QuteNoteTextEdit"));

    ENML.append("<en-note>");

    QTextBlock noteDocEnd = pNoteDoc->end();
    for(QTextBlock block = pNoteDoc->begin(); block != noteDocEnd; block = block.next())
    {
        QString blockText = block.text();
        if (blockText.isEmpty()) {
            ENML.append("<div><br /></div>");
            continue;
        }

        for(QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it)
        {
            ENML.append("<div>");
            QTextFragment currentFragment = it.fragment();
            QTextCharFormat currentFragmentCharFormat = currentFragment.charFormat();
            int currentFragmentCharFormatObjectType = currentFragmentCharFormat.objectType();
            if (currentFragmentCharFormatObjectType == QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_CHECKED) {
                ENML.append("<en-todo checked=\"true\"/>");
            }
            else if (currentFragmentCharFormatObjectType == QuteNoteTextEdit::TODO_CHKBOX_TXT_FMT_UNCHECKED) {
                ENML.append("<en-todo checked=\"false\"/>");
            }
            else if (currentFragmentCharFormatObjectType == QuteNoteTextEdit::MEDIA_RESOURCE_TXT_FORMAT)
            {
                bool res = addEnMediaFromCharFormat(currentFragmentCharFormat, ENML,
                                                    errorDescription);
                if (!res) {
                    return false;
                }
            }
            else if (currentFragment.isValid())
            {
                QString encodedCurrentFragment;
                bool res = encodeFragment(currentFragment, encodedCurrentFragment,
                                          errorDescription);
                if (!res) {
                    errorDescription.prepend(QT_TR_NOOP("ENML converter: can't encode fragment: "));
                    return false;
                }
                else {
                    ENML.append(encodedCurrentFragment);
                }
            }
            else {
                errorDescription = QT_TR_NOOP("Found invalid QTextFragment during "
                                              "encoding note content to ENML: ");
                errorDescription.append(currentFragment.text());
                return false;
            }
            ENML.append(("</div>"));
        }
    }

    return true;
}

bool ENMLConverter::noteToRichText(const qevercloud::Note & note, QuteNoteTextEdit & noteEditor,
                                   QString & errorMessage) const
{
    const QScopedPointer<QuteNoteTextEdit> pFakeNoteEditor(new QuteNoteTextEdit());
    QTextDocument * pNoteEditorDoc = noteEditor.document();
    pFakeNoteEditor->setDocument(pNoteEditorDoc);
    QTextCursor cursor(pNoteEditorDoc);

    // Now we have fake note editor object which we can use to iterate through
    // document using QTextCursor

    QDomDocument enXmlDomDoc;
    int errorLine = -1, errorColumn = -1;
    bool res = enXmlDomDoc.setContent(note.content, &errorMessage, &errorLine, &errorColumn);
    if (!res) {
        // TRANSLATOR Explaining the error of XML parsing
        errorMessage += QT_TR_NOOP(". Error happened at line ") +
                        QString::number(errorLine) + QT_TR_NOOP(", at column ") +
                        QString::number(errorColumn);
        return false;
    }

    QDomElement docElem = enXmlDomDoc.documentElement();
    QString rootTag = docElem.tagName();
    if (rootTag != QString("en-note")) {
        errorMessage = QT_TR_NOOP("Bad note content: wrong root tag, should be \"en-note\", instead: ");
        errorMessage += rootTag;
        return false;
    }

    QList<qevercloud::Resource> resources;
    if (note.resources.isSet()) {
        resources = note.resources.ref();
    }

    QDomNode nextNode = docElem.firstChild();
    while(!nextNode.isNull())
    {
        QDomElement element = nextNode.toElement();
        if (!element.isNull())
        {
            QString tagName = element.tagName();
            if (isForbiddenXhtmlTag(tagName)) {
                errorMessage = QT_TR_NOOP("Found forbidden XHTML tag in ENML: ");
                errorMessage.append(tagName);
                return false;
            }
            else if (isEvernoteSpecificXhtmlTag(tagName))
            {
                if (tagName == "en-todo")
                {
                    QString checked = element.attribute("checked", "false");
                    if (checked == "true") {
                        pFakeNoteEditor->insertCheckedToDoCheckboxAtCursor(cursor);
                    }
                    else {
                        pFakeNoteEditor->insertUncheckedToDoCheckboxAtCursor(cursor);
                    }
                }
                else if (tagName == "en-crypt")
                {
                    // TODO: support encrypted note content
                    errorMessage = QT_TR_NOOP("Encrypted note content is not supported in QuteNote yet");
                    return false;
                }
                else if (tagName == "en-note")
                {
                    errorMessage = QT_TR_NOOP("Internal error: en-note should be the root node of note\'s ENML");
                    return false;
                }
                else if (tagName == "en-media")
                {
                    QString hashFromENML = element.attribute("hash");
                    if (hashFromENML.isEmpty()) {
                        errorMessage = QT_TR_NOOP("\"en-media\" tag has empty \"hash\" attribute");
                        return false;
                    }

                    int resourceIndex = indexOfResourceByHash(resources, hashFromENML.toUtf8());
                    if (resourceIndex < 0) {
                        errorMessage = QT_TR_NOOP("Internal error: unable to find note's resource "
                                                   "with specified hash: ");
                        errorMessage += hashFromENML;
                        return false;
                    }

                    const qevercloud::Resource & resource = resources.at(resourceIndex);
                    QString resourceHash;
                    if (resource.data.isSet() && resource.data->bodyHash.isSet()) {
                        resourceHash = QString(resource.data->bodyHash);
                    }
                    else if (resource.recognition.isSet() && resource.recognition->bodyHash.isSet()) {
                        resourceHash = QString(resource.recognition->bodyHash);
                    }
                    else if (resource.alternateData.isSet() && resource.alternateData->bodyHash.isSet()) {
                        resourceHash = QString(resource.alternateData->bodyHash);
                    }

                    if (resourceHash.isEmpty()) {
                        errorMessage = QT_TR_NOOP("No relevant data hash found in the resource");
                        return false;
                    }

                    if (!resource.mime.isSet()) {
                        errorMessage = QT_TR_NOOP("Found resource without mime type set");
                        return false;
                    }
                    QString mimeType = resource.mime;

                    // Images have special treatment
                    if (mimeType.contains("image/"))
                    {
                        QMimeData resourceMimeData;
                        if (resource.data.isSet() && resource.data->body.isSet()) {
                            resourceMimeData.setData(mimeType, resource.data->body.ref());
                        }
                        else if (resource.recognition.isSet() && resource.recognition->body.isSet()) {
                            resourceMimeData.setData(mimeType, resource.recognition->body.ref());
                        }
                        else if (resource.alternateData.isSet() && resource.alternateData->body.isSet()) {
                            resourceMimeData.setData(mimeType, resource.alternateData->body.ref());
                        }

                        if (!resourceMimeData.hasImage()) {
                            errorMessage = QT_TR_NOOP("Internal error: mime type of the resource "
                                                      "was marked as the image one but resource's "
                                                      "mime data does not have an image.");
                            // TODO: print resource
                            return false;
                        }

                        QImage resourceImg = qvariant_cast<QImage>(resourceMimeData.imageData());
                        QTextImageFormat resourceImgFormat;
                        resourceImgFormat.setWidth(resourceImg.width());
                        resourceImgFormat.setHeight(resourceImg.height());

                        QTextCursor cursor = pFakeNoteEditor->textCursor();
                        cursor.insertImage(resourceImgFormat);
                    }
                    else
                    {
                        // TODO: somehow display the metadata of the resource inside the QuteNoteTextEdit object
                    }

                    // TODO: support optional note content's en-media align, alt,
                    // longdesc, height, width, border, hspace, vspace, usemap, style, lang, xml:lang, dir
                }
                else
                {
                    errorMessage = QT_TR_NOOP("Internal error: found some ENML tag specified as "
                                              "Evernote-specific one but not en-note, en-crypt, "
                                              "en-media or en-todo: ");
                    errorMessage.append(tagName);
                    return false;
                }
            }
            else if (isAllowedXhtmlTag(tagName)) {
                QString elementHtml = domElementToRawXML(element);
                cursor.insertHtml(elementHtml);
            }
            else {
                errorMessage = QT_TR_NOOP("Found XHTML tag not listed as either "
                                          "forbidden or allowed one: ");
                errorMessage.append(tagName);
                return false;
            }
        }
        else
        {
            errorMessage = QT_TR_NOOP("Found QDomNode not convertable to QDomElement");
            return false;
        }
    }

    return true;
}

bool ENMLConverter::noteContentToPlainText(const QString & noteContent, QString & plainText,
                                           QString & errorMessage) const
{
    plainText.clear();

    QDomDocument enXmlDomDoc;
    int errorLine = -1, errorColumn = -1;
    bool res = enXmlDomDoc.setContent(noteContent, &errorMessage, &errorLine, &errorColumn);
    if (!res) {
        // TRANSLATOR Explaining the error of XML parsing
        errorMessage += QT_TR_NOOP(". Error happened at line ") + QString::number(errorLine) +
                        QT_TR_NOOP(", at column ") + QString::number(errorColumn) +
                        QT_TR_NOOP(", bad note content: ") + noteContent;
        return false;
    }

    QDomElement docElem = enXmlDomDoc.documentElement();
    QString rootTag = docElem.tagName();
    if (rootTag != QString("en-note")) {
        // TRANSLATOR Explaining the error of XML parsing
        errorMessage = QT_TR_NOOP("Bad note content: wrong root tag, should be \"en-note\", instead: ");
        errorMessage += rootTag;
        return false;
    }

    QDomNode nextNode = docElem.firstChild();
    while(!nextNode.isNull())
    {
        QDomElement element = nextNode.toElement();
        if (!element.isNull())
        {
            QString tagName = element.tagName();
            if (isAllowedXhtmlTag(tagName)) {
                plainText += element.text();
            }
            else if (isForbiddenXhtmlTag(tagName)) {
                errorMessage = QT_TR_NOOP("Found forbidden XHTML tag in ENML: ");
                errorMessage += tagName;
                return false;
            }
            else if (!isEvernoteSpecificXhtmlTag(tagName)) {
                errorMessage = QT_TR_NOOP("Found XHTML tag not listed as either "
                                          "forbidden or allowed one: ");
                errorMessage += tagName;
                return false;
            }
        }
        else
        {
            errorMessage = QT_TR_NOOP("Found QDomNode not convertable to QDomElement");
            return false;
        }

        nextNode = nextNode.nextSibling();
    }

    return true;
}

bool ENMLConverter::noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                             QString & errorMessage, QString * plainText) const
{
    QString _plainText;
    bool res = noteContentToPlainText(noteContent, _plainText, errorMessage);
    if (!res) {
        listOfWords.clear();
        return false;
    }

    if (plainText) {
        *plainText = _plainText;
    }

    listOfWords = plainTextToListOfWords(_plainText);
    return true;
}

QStringList ENMLConverter::plainTextToListOfWords(const QString & plainText)
{
    // Simply remove all non-word characters from plain text
    return plainText.split(QRegExp("\\W+"), QString::SkipEmptyParts);
}

void ENMLConverter::fillTagsLists()
{
    m_forbiddenXhtmlTags.clear();
    m_forbiddenXhtmlTags.insert("applet");
    m_forbiddenXhtmlTags.insert("base");
    m_forbiddenXhtmlTags.insert("basefont");
    m_forbiddenXhtmlTags.insert("bgsound");
    m_forbiddenXhtmlTags.insert("blink");
    m_forbiddenXhtmlTags.insert("body");
    m_forbiddenXhtmlTags.insert("button");
    m_forbiddenXhtmlTags.insert("dir");
    m_forbiddenXhtmlTags.insert("embed");
    m_forbiddenXhtmlTags.insert("fieldset");
    m_forbiddenXhtmlTags.insert("form");
    m_forbiddenXhtmlTags.insert("frame");
    m_forbiddenXhtmlTags.insert("frameset");
    m_forbiddenXhtmlTags.insert("head");
    m_forbiddenXhtmlTags.insert("html");
    m_forbiddenXhtmlTags.insert("iframe");
    m_forbiddenXhtmlTags.insert("ilayer");
    m_forbiddenXhtmlTags.insert("input");
    m_forbiddenXhtmlTags.insert("isindex");
    m_forbiddenXhtmlTags.insert("label");
    m_forbiddenXhtmlTags.insert("layer");
    m_forbiddenXhtmlTags.insert("legend");
    m_forbiddenXhtmlTags.insert("link");
    m_forbiddenXhtmlTags.insert("marquee");
    m_forbiddenXhtmlTags.insert("menu");
    m_forbiddenXhtmlTags.insert("meta");
    m_forbiddenXhtmlTags.insert("noframes");
    m_forbiddenXhtmlTags.insert("noscript");
    m_forbiddenXhtmlTags.insert("object");
    m_forbiddenXhtmlTags.insert("optgroup");
    m_forbiddenXhtmlTags.insert("option");
    m_forbiddenXhtmlTags.insert("param");
    m_forbiddenXhtmlTags.insert("plaintext");
    m_forbiddenXhtmlTags.insert("script");
    m_forbiddenXhtmlTags.insert("select");
    m_forbiddenXhtmlTags.insert("style");
    m_forbiddenXhtmlTags.insert("textarea");
    m_forbiddenXhtmlTags.insert("xml");

    m_forbiddenXhtmlAttributes.clear();
    m_forbiddenXhtmlAttributes.insert("id");
    m_forbiddenXhtmlAttributes.insert("class");
    m_forbiddenXhtmlAttributes.insert("onclick");
    m_forbiddenXhtmlAttributes.insert("ondblclick");
    m_forbiddenXhtmlAttributes.insert("accesskey");
    m_forbiddenXhtmlAttributes.insert("data");
    m_forbiddenXhtmlAttributes.insert("dynsrc");
    m_forbiddenXhtmlAttributes.insert("tableindex");

    m_evernoteSpecificXhtmlTags.clear();
    m_evernoteSpecificXhtmlTags.insert("en-note");
    m_evernoteSpecificXhtmlTags.insert("en-media");
    m_evernoteSpecificXhtmlTags.insert("en-crypt");
    m_evernoteSpecificXhtmlTags.insert("en-todo");

    m_allowedXhtmlTags.clear();
    m_allowedXhtmlTags.insert("a");
    m_allowedXhtmlTags.insert("abbr");
    m_allowedXhtmlTags.insert("acronym");
    m_allowedXhtmlTags.insert("address");
    m_allowedXhtmlTags.insert("area");
    m_allowedXhtmlTags.insert("b");
    m_allowedXhtmlTags.insert("bdo");
    m_allowedXhtmlTags.insert("big");
    m_allowedXhtmlTags.insert("blockquote");
    m_allowedXhtmlTags.insert("br");
    m_allowedXhtmlTags.insert("caption");
    m_allowedXhtmlTags.insert("center");
    m_allowedXhtmlTags.insert("cite");
    m_allowedXhtmlTags.insert("code");
    m_allowedXhtmlTags.insert("col");
    m_allowedXhtmlTags.insert("colgroup");
    m_allowedXhtmlTags.insert("dd");
    m_allowedXhtmlTags.insert("del");
    m_allowedXhtmlTags.insert("dfn");
    m_allowedXhtmlTags.insert("div");
    m_allowedXhtmlTags.insert("dl");
    m_allowedXhtmlTags.insert("dt");
    m_allowedXhtmlTags.insert("em");
    m_allowedXhtmlTags.insert("font");
    m_allowedXhtmlTags.insert("h1");
    m_allowedXhtmlTags.insert("h2");
    m_allowedXhtmlTags.insert("h3");
    m_allowedXhtmlTags.insert("h4");
    m_allowedXhtmlTags.insert("h5");
    m_allowedXhtmlTags.insert("h6");
    m_allowedXhtmlTags.insert("hr");
    m_allowedXhtmlTags.insert("i");
    m_allowedXhtmlTags.insert("img");
    m_allowedXhtmlTags.insert("ins");
    m_allowedXhtmlTags.insert("kbd");
    m_allowedXhtmlTags.insert("li");
    m_allowedXhtmlTags.insert("map");
    m_allowedXhtmlTags.insert("ol");
    m_allowedXhtmlTags.insert("p");
    m_allowedXhtmlTags.insert("pre");
    m_allowedXhtmlTags.insert("q");
    m_allowedXhtmlTags.insert("s");
    m_allowedXhtmlTags.insert("samp");
    m_allowedXhtmlTags.insert("small");
    m_allowedXhtmlTags.insert("span");
    m_allowedXhtmlTags.insert("strike");
    m_allowedXhtmlTags.insert("strong");
    m_allowedXhtmlTags.insert("sub");
    m_allowedXhtmlTags.insert("sup");
    m_allowedXhtmlTags.insert("table");
    m_allowedXhtmlTags.insert("tbody");
    m_allowedXhtmlTags.insert("td");
    m_allowedXhtmlTags.insert("tfoot");
    m_allowedXhtmlTags.insert("th");
    m_allowedXhtmlTags.insert("thead");
    m_allowedXhtmlTags.insert("title");
    m_allowedXhtmlTags.insert("tr");
    m_allowedXhtmlTags.insert("tt");
    m_allowedXhtmlTags.insert("u");
    m_allowedXhtmlTags.insert("ul");
    m_allowedXhtmlTags.insert("var");
    m_allowedXhtmlTags.insert("xmp");
}

bool ENMLConverter::encodeFragment(const QTextFragment & fragment,
                                   QString & encodedFragment,
                                   QString & errorMessage) const
{
    if (!fragment.isValid()) {
        errorMessage = QT_TR_NOOP("Cannot encode ENML fragment: fragment is not valid: ");
        errorMessage.append(fragment.text());
        return false;
    }

    QTextCharFormat format = fragment.charFormat();
    QString text = fragment.text();

    QTextEdit fakeTextEdit;
    QTextCursor cursor = fakeTextEdit.textCursor();
    cursor.insertText(text, format);

    QString html = fakeTextEdit.toHtml();

    // Erasing redundant html tags added by Qt's internal method
    QDomDocument htmlXmlDoc;
    int errorLine = -1, errorColumn = -1;
    bool res = htmlXmlDoc.setContent(html, &errorMessage, &errorLine, &errorColumn);
    if (!res) {
        errorMessage.append(QT_TR_NOOP(". Error happened at line ") +
                            QString::number(errorLine) + QT_TR_NOOP(", at column ") +
                            QString::number(errorColumn));
        return false;
    }

    encodedFragment.clear();

    QDomElement element = htmlXmlDoc.documentElement();
    QDomNode nextNode = element.firstChild();
    while(!nextNode.isNull())
    {
        element = nextNode.toElement();
        if (!element.isNull())
        {
            QString tagName = element.tagName();
            if ( (tagName != "html") &&
                 (tagName != "head") &&
                 (tagName != "meta") )
            {
                QString elementRawXml = domElementToRawXML(element);
                encodedFragment.append(elementRawXml);
            }
        }

        nextNode = element.firstChild();
    }

    // TODO: verify that encodedFragment has only allowed tags and attributes using Evernote's DTD file
    return true;
}

const QString ENMLConverter::domElementToRawXML(const QDomElement & elem) const
{
    QString xml = "<";

    QString elemTagName = elem.tagName();
    xml += elemTagName;

    QDomNamedNodeMap attributes = elem.attributes();
    size_t numAttributes = attributes.size();
    for(size_t i = 0; i < numAttributes; ++i)
    {
        QDomAttr attribute = attributes.item(i).toAttr();
        xml += QString::fromLatin1(" %0=\"%1\"").arg(attribute.name()).arg(attribute.value());
    }
    xml += ">";
    xml += elem.text();
    xml += "</";
    xml += elemTagName;
    xml += ">";

    return xml;
}

bool ENMLConverter::isForbiddenXhtmlTag(const QString & tagName) const
{
    auto it = m_forbiddenXhtmlTags.find(tagName);
    if (it == m_forbiddenXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverter::isForbiddenXhtmlAttribute(const QString & attributeName) const
{
    auto it = m_forbiddenXhtmlAttributes.find(attributeName);
    if (it == m_forbiddenXhtmlAttributes.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverter::isEvernoteSpecificXhtmlTag(const QString & tagName) const
{
    auto it = m_evernoteSpecificXhtmlTags.find(tagName);
    if (it == m_evernoteSpecificXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverter::isAllowedXhtmlTag(const QString & tagName) const
{
    auto it = m_allowedXhtmlTags.find(tagName);
    if (it == m_allowedXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

int ENMLConverter::indexOfResourceByHash(const QList<qevercloud::Resource> & resources,
                                         const QByteArray & hash)
{
    if (resources.isEmpty()) {
        return -1;
    }

    int numResources = resources.size();
    for(int i = 0; i < numResources; ++i)
    {
        const qevercloud::Resource & resource = resources.at(i);

        if (resource.data.isSet() && resource.data->bodyHash.isSet()) {
            const QByteArray & resourceHash = resource.data->bodyHash;
            if (hash == resourceHash) {
                return i;
            }
        }

        if (resource.recognition.isSet() && resource.recognition->bodyHash.isSet()) {
            const QByteArray & resourceHash = resource.recognition->bodyHash;
            if (hash == resourceHash) {
                return i;
            }
        }

        if (resource.alternateData.isSet() && resource.alternateData->bodyHash.isSet()) {
            const QByteArray & resourceHash = resource.alternateData->bodyHash;
            if (hash == resourceHash) {
                return i;
            }
        }
    }

    return -1;
}

bool ENMLConverter::addEnMediaFromCharFormat(const QTextCharFormat & format, QString & ENML,
                                             QString & errorDescription) const
{
    // FIXME: set valid property id instead of these ad-hoc ones

    QVariant resourceHash = format.property(QTextFormat::UserProperty + 1);
    if (!resourceHash.isValid()) {
        errorDescription = QT_TR_NOOP("Internal error! Unable to determine "
                                       "resource hash from QTextCharFormat "
                                       "in QuteNoteTextEdit widget");
        return false;
    }

    QVariant resourceMimeType = format.property(QTextFormat::UserProperty + 2);
    if (!resourceMimeType.isValid()) {
        errorDescription = QT_TR_NOOP("Internal error! Unable to determime "
                                       "resource mime type from QTextCharFormat "
                                       "in QuteNoteTextEdit widget");
        return false;
    }

    ENML.append("<en-media hash=\"");
    ENML.append(QString(resourceHash.toByteArray()));
    ENML.append("\" type=\"");
    ENML.append(resourceMimeType.toString());
    ENML.append("\"");

#define CHECK_AND_APPEND_PROPERTY(prop, id) \
    QVariant resource##prop = format.property(id); \
    if (resource##prop.isValid()) { \
        ENML.append(" " #prop "=\""); \
        ENML.append(resource##prop.toString()); \
        ENML.append("\""); \
    }

    CHECK_AND_APPEND_PROPERTY(align, QTextFormat::UserProperty + 3);
    CHECK_AND_APPEND_PROPERTY(alt, QTextFormat::UserProperty + 4);
    CHECK_AND_APPEND_PROPERTY(longdesc, QTextFormat::UserProperty + 5);
    CHECK_AND_APPEND_PROPERTY(height, QTextFormat::UserProperty + 6);
    CHECK_AND_APPEND_PROPERTY(width, QTextFormat::UserProperty + 7);
    CHECK_AND_APPEND_PROPERTY(border, QTextFormat::UserProperty + 8);
    CHECK_AND_APPEND_PROPERTY(hspace, QTextFormat::UserProperty + 9);
    CHECK_AND_APPEND_PROPERTY(vspace, QTextFormat::UserProperty + 10);
    CHECK_AND_APPEND_PROPERTY(usemap, QTextFormat::UserProperty + 11);

    CHECK_AND_APPEND_PROPERTY(style, QTextFormat::UserProperty + 12);
    CHECK_AND_APPEND_PROPERTY(title, QTextFormat::UserProperty + 13);
    CHECK_AND_APPEND_PROPERTY(lang, QTextFormat::UserProperty + 14);
    CHECK_AND_APPEND_PROPERTY(dir, QTextFormat::UserProperty + 15);

#undef CHECK_AND_APPEND_PROPERTY

    QVariant resourceXmlLang = format.property(QTextFormat::UserProperty + 16);
    if (resourceXmlLang.isValid()) {
        ENML.append(" xml:lang=\"");
        ENML.append(resourceXmlLang.toString());
        ENML.append("\"");
    }

    ENML.append("/>");
    return true;
}


}
