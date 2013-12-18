#include "ENMLConverter.h"
#include "../note_editor/QuteNoteTextEdit.h"
#include "../Note.h"
#include "../ResourceMetadata.h"
#include "../tools/QuteNoteCheckPtr.h"
#include <QTextEdit>
#include <QString>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QDomDocument>
#include <QDebug>

namespace qute_note {

ENMLConverter::ENMLConverter()
{
    fillTagsLists();
}

ENMLConverter::ENMLConverter(const ENMLConverter & other) :
    m_forbiddenXhtmlTags(other.m_forbiddenXhtmlTags.cbegin(),
                         other.m_forbiddenXhtmlTags.cend()),
    m_forbiddenXhtmlAttributes(other.m_forbiddenXhtmlAttributes.cbegin(),
                               other.m_forbiddenXhtmlAttributes.cend()),
    m_evernoteSpecificXhtmlTags(other.m_evernoteSpecificXhtmlTags.cbegin(),
                                other.m_evernoteSpecificXhtmlTags.cend()),
    m_allowedXhtmlTags(other.m_allowedXhtmlTags.cbegin(),
                       other.m_allowedXhtmlTags.cend())
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

// TODO: adopt some simple xml node-constructing instead of plain text insertions
bool ENMLConverter::richTextToENML(const QuteNoteTextEdit & noteEditor, QString & ENML,
                                   QString & errorDescription) const
{
    ENML.clear();

    const QTextDocument * pNoteDoc = noteEditor.document();
    QUTE_NOTE_CHECK_PTR(pNoteDoc, QObject::tr("Null QTextDocument pointer received from QuteNoteTextEdit"));

    const Note * pNote = noteEditor.getNotePtr();
    QUTE_NOTE_CHECK_PTR(pNote, QObject::tr("Null pointer to Note received from QuteNoteTextEdit"));

    std::vector<ResourceMetadata> resourcesMetadata;
    pNote->getResourcesMetadata(resourcesMetadata);
    std::size_t numAttachedResources = resourcesMetadata.size();
    std::size_t resourceIndex = 0;

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
                if (resourceIndex >= numAttachedResources)
                {
                    errorDescription = QObject::tr("Internal error! Found char format corresponding to resource "
                                                   "but haven't found the corresponding resource object for index ");
                    errorDescription.append(QString::number(resourceIndex));
                    return false;
                }
                else
                {
                    const ResourceMetadata & resourceMetadata = resourcesMetadata.at(resourceIndex);

                    QString hash = resourceMetadata.dataHash();
                    size_t width = resourceMetadata.width();
                    size_t height = resourceMetadata.height();
                    QString type = resourceMetadata.mimeType();
                    ENML.append("<en-media width=\"");
                    ENML.append(QString::number(width));
                    ENML.append("\" height=\"");
                    ENML.append(QString::number(height));
                    ENML.append("\" type=\"");
                    ENML.append(type);
                    ENML.append("\" hash=\"");
                    ENML.append(hash);
                    ENML.append("/>");
                }

                ++resourceIndex;
            }
            else if (currentFragment.isValid())
            {
                QString encodedCurrentFragment;
                bool res = encodeFragment(currentFragment, encodedCurrentFragment,
                                          errorDescription);
                if (!res) {
                    errorDescription = QObject::tr("ENML converter: can't encode fragment: ") +
                                       errorDescription;
                    return false;
                }
                else {
                    ENML.append(encodedCurrentFragment);
                }
            }
            else {
                errorDescription = QObject::tr("Found invalid QTextFragment during "
                                               "encoding note content to ENML: ");
                errorDescription.append(currentFragment.text());
                return false;
            }
            ENML.append(("</div>"));
        }
    }

    ENML.append("</en_note>");
    return true;
}

bool ENMLConverter::ENMLToRichText(const QString & ENML, const DatabaseManager & /* databaseManager */,
                                   QuteNoteTextEdit & noteEditor, QString & errorMessage) const
{
    const Note * pNote = noteEditor.getNotePtr();
    QUTE_NOTE_CHECK_PTR(pNote, QObject::tr("Null pointer to Note received from QuteNoteTextEdit"));

    const QScopedPointer<QuteNoteTextEdit> pFakeNoteEditor(new QuteNoteTextEdit());
    QTextDocument * pNoteEditorDoc = noteEditor.document();
    pFakeNoteEditor->setDocument(pNoteEditorDoc);
    QTextCursor cursor(pNoteEditorDoc);

    // Now we have fake note editor object which we can use to iterate through
    // document using QTextCursor

    QDomDocument enXmlDomDoc;
    int errorLine = -1, errorColumn = -1;
    bool res = enXmlDomDoc.setContent(ENML, &errorMessage, &errorLine, &errorColumn);
    if (!res) {
        errorMessage.append(QObject::tr(". Error happened at line ") +
                            QString::number(errorLine) + QObject::tr(", at column ") +
                            QString::number(errorColumn));
        return false;
    }

    QDomElement docElem = enXmlDomDoc.documentElement();
    QString rootTag = docElem.tagName();
    if (rootTag != QString("en-note")) {
        errorMessage = QObject::tr("Wrong root tag, should be \"en-note\", instead: ");
        errorMessage.append(rootTag);
        return false;
    }

    bool noteHasAttachedResources = pNote->hasAttachedResources();
    std::size_t numAttachedResources = pNote->numAttachedResources();
    std::vector<ResourceMetadata> resourcesMetadata;
    pNote->getResourcesMetadata(resourcesMetadata);
    std::size_t resourceIndex = 0;

    QDomNode nextNode = docElem.firstChild();
    while(!nextNode.isNull())
    {
        QDomElement element = nextNode.toElement();
        if (!element.isNull())
        {
            QString tagName = element.tagName();
            if (isForbiddenXhtmlTag(tagName)) {
                errorMessage = QObject::tr("Found forbidden XHTML tag in ENML: ");
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
                    errorMessage = QObject::tr("Encrypted note content is not supported in QuteNote yet");
                    return false;
                }
                else if (tagName == "en-note")
                {
                    errorMessage = QObject::tr("Internal error: en-note should be the root node of note\'s ENML");
                    return false;
                }
                else if (tagName == "en-media")
                {
                    if (!noteHasAttachedResources) {
                        errorMessage = QObject::tr("Internal error: note reported no attached resources "
                                                   "but \"en-media\" tag was found in its ENML. ");
                        // TODO: print note here
                        return false;
                    }

                    if (resourceIndex >= numAttachedResources) {
                        errorMessage = QObject::tr("Internal error: the index of the next resource ");
                        errorMessage.append(static_cast<int>(resourceIndex));
                        errorMessage.append(QObject::tr(" must be smaller than the number of resources "
                                                        "attached to the note."));
                        return false;
                    }

                    QString hashFromENML = element.attribute("hash");
                    if (hashFromENML.isEmpty()) {
                        errorMessage = QObject::tr("\"en-media\" tag has empty \"hash\" attribute");
                        return false;
                    }

                    const ResourceMetadata & currentResourceMetadata = resourcesMetadata.at(resourceIndex);
                    QString hashFromResource = currentResourceMetadata.dataHash();
                    if (hashFromResource.isEmpty())
                    {
                        errorMessage = QObject::tr("Binary data hash of the resource object is empty");
                        errorMessage.append(", ");
                        errorMessage.append(currentResourceMetadata.ToQString());
                        return false;
                    }

                    if (hashFromENML != hashFromResource)
                    {
                        errorMessage = QObject::tr("Hashes of binary data of the resource differ for ENML "
                                                   "and the corresponding Resource object. The ENML's hash: ");
                        errorMessage.append(hashFromENML);
                        errorMessage.append(QObject::tr(" , resource's hash: "));
                        errorMessage.append(hashFromResource);
                        errorMessage.append(", ");
                        errorMessage.append(currentResourceMetadata.ToQString());
                        return false;
                    }

                    ++resourceIndex;

                    /*
                    std::size_t width  = currentResourceMetadata.width();
                    std::size_t height = currentResourceMetadata.height();
                    */
                    QString mimeType = currentResourceMetadata.mimeType();
                    // This should be enough, we don't need the actual binary data just to display the resource
                    // The only exceptions are resources of image type

                    if (mimeType.contains("image/"))
                    {
                        // FIXME: reimplement: obtain resource's binary data from local storage database

                        /*
                        const QMimeData & resourceMimeData = currentResourceMetadata->mimeData();
                        if (!resourceMimeData.hasImage()) {
                            errorMessage = QObject::tr("Internal error: mime type of the resource "
                                                       "was marked as the image one but resource's "
                                                       "mime data does not have an image.");
                            // TODO: print resource here
                            return false;
                        }

                        QImage resourceImg = qvariant_cast<QImage>(resourceMimeData.imageData());
                        QTextImageFormat resourceImgFormat;
                        resourceImgFormat.setWidth(resourceImg.width());
                        resourceImgFormat.setHeight(resourceImg.height());

                        QTextCursor cursor = pFakeNoteEditor->textCursor();
                        cursor.insertImage(resourceImgFormat);
                        */
                    }
                    else
                    {
                        // TODO: somehow display the metadata of the resource inside the QuteNoteTextEdit object
                    }
                }
                else
                {
                    errorMessage = QObject::tr("Internal error: found some ENML tag specified as "
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
                errorMessage = QObject::tr("Found XHTML tag not listed as either "
                                           "forbidden or allowed one: ");
                errorMessage.append(tagName);
                return false;
            }
        }
        else
        {
            errorMessage = QObject::tr("Found QDomNode not convertable to QDomElement");
            return false;
        }
    }

    return true;
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
        errorMessage = QObject::tr("Cannot encode ENML fragment: fragment is not valid: ");
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
        errorMessage.append(QObject::tr(". Error happened at line ") +
                            QString::number(errorLine) + QObject::tr(", at column ") +
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
    QString head = "<" + elem.tagName();
    QDomNamedNodeMap attributes = elem.attributes();
    size_t numAttributes = attributes.size();
    for(size_t i = 0; i < numAttributes; ++i)
    {
        QDomAttr attribute = attributes.item(i).toAttr();
        head += QString::fromLatin1(" %0=\"%1\"").arg(attribute.name()).arg(attribute.value());
    }
    head += ">";
    return head + elem.text() + "</" + elem.tagName() + ">";
}

bool ENMLConverter::isForbiddenXhtmlTag(const QString & tagName) const
{
    auto it = m_forbiddenXhtmlTags.find(tagName);
    if (it == m_forbiddenXhtmlTags.cend()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverter::isForbiddenXhtmlAttribute(const QString & attributeName) const
{
    auto it = m_forbiddenXhtmlAttributes.find(attributeName);
    if (it == m_forbiddenXhtmlAttributes.cend()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverter::isEvernoteSpecificXhtmlTag(const QString & tagName) const
{
    auto it = m_evernoteSpecificXhtmlTags.find(tagName);
    if (it == m_evernoteSpecificXhtmlTags.cend()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverter::isAllowedXhtmlTag(const QString & tagName) const
{
    auto it = m_allowedXhtmlTags.find(tagName);
    if (it == m_allowedXhtmlTags.cend()) {
        return false;
    }
    else {
        return true;
    }
}

}
