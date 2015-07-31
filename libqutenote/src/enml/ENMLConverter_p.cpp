#include "ENMLConverter_p.h"
#include <qute_note/enml/HTMLCleaner.h>
#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#include <qute_note/logging/QuteNoteLogger.h>
#include <libxml/xmlreader.h>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QScopedPointer>
#include <QDomDocument>
#include <QRegExp>

namespace qute_note {

#define WRAP(x) \
    << x

static const QSet<QString> forbiddenXhtmlTags = QSet<QString>()
#include "forbiddenXhtmlTags.inl"
;

static const QSet<QString> forbiddenXhtmlAttributes = QSet<QString>()
#include "forbiddenXhtmlAttributes.inl"
;


static const QSet<QString> evernoteSpecificXhtmlTags = QSet<QString>()
#include "evernoteSpecificXhtmlTags.inl"
;


static const QSet<QString> allowedXhtmlTags = QSet<QString>()
#include "allowedXhtmlTags.inl"
;

static const QSet<QString> allowedEnMediaAttributes = QSet<QString>()
#include "allowedEnMediaAttributes.inl"
;

#undef WRAP

ENMLConverterPrivate::ENMLConverterPrivate() :
    m_pHtmlCleaner(nullptr),
    m_cachedConvertedXml()
{}

ENMLConverterPrivate::~ENMLConverterPrivate()
{
    delete m_pHtmlCleaner;
}

bool ENMLConverterPrivate::htmlToNoteContent(const QString & html, QString & noteContent, QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::htmlToNoteContent: " << html);

    if (!m_pHtmlCleaner) {
        m_pHtmlCleaner = new HTMLCleaner;
    }

    m_cachedConvertedXml.resize(0);
    bool res = m_pHtmlCleaner->htmlToXml(html, m_cachedConvertedXml, errorDescription);
    if (!res) {
        errorDescription.prepend(QT_TR_NOOP("Could not clean up note's html: "));
        return false;
    }

    QXmlStreamReader reader(m_cachedConvertedXml);

    noteContent.resize(0);
    QXmlStreamWriter writer(&noteContent);
    writer.setAutoFormatting(true);
    writer.setCodec("UTF-8");
    writer.writeStartDocument();
    writer.writeDTD("<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">");

    int writeElementCounter = 0;
    QString lastElementName;
    QXmlStreamAttributes lastElementAttributes;

    bool insideEnCryptElement = false;
    QXmlStreamAttributes enCryptAttributes;

    bool insideEnMediaElement = false;
    QXmlStreamAttributes enMediaAttributes;

    bool insideDecryptedEnCryptElement = false;

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartDocument()) {
            continue;
        }

        if (reader.isDTD()) {
            continue;
        }

        if (reader.isEndDocument()) {
            break;
        }

        if (reader.isStartElement())
        {
            lastElementName = reader.name().toString();
            if (lastElementName == "form") {
                QNTRACE("Skipping <form> tag");
                continue;
            }
            else if (lastElementName == "html") {
                QNTRACE("Skipping <html> tag");
                continue;
            }
            else if (lastElementName == "title") {
                QNTRACE("Skipping <title> tag");
                continue;
            }
            else if (lastElementName == "body") {
                lastElementName = "en-note";
                QNTRACE("Found \"body\" HTML tag, will replace it with \"en-note\" tag for written ENML");
            }

            auto tagIt = forbiddenXhtmlTags.find(lastElementName);
            if ((tagIt != forbiddenXhtmlTags.end()) && (lastElementName != "object") && (lastElementName != "param")) {
                QNTRACE("Skipping forbidden XHTML tag: " << lastElementName);
                continue;
            }

            tagIt = allowedXhtmlTags.find(lastElementName);
            if ((tagIt == allowedXhtmlTags.end()) && (lastElementName != "param"))
            {
                tagIt = evernoteSpecificXhtmlTags.find(lastElementName);
                if (tagIt == evernoteSpecificXhtmlTags.end()) {
                    QNTRACE("Haven't found tag " << lastElementName << " within the list of allowed XHTML tags "
                            "or within Evernote-specific tags, skipping it");
                    continue;
                }
            }

            lastElementAttributes = reader.attributes();

            if ((lastElementName == "div") && lastElementAttributes.hasAttribute("en-tag"))
            {
                const QString enTag = lastElementAttributes.value("en-tag").toString();
                if (enTag == "en-decrypted")
                {
                    QNTRACE("Found decrypted text area, need to convert it back to en-crypt form");
                    bool res = decryptedTextToEnml(reader, writer, errorDescription);
                    if (!res) {
                        return false;
                    }

                    insideDecryptedEnCryptElement = true;
                    ++writeElementCounter;
                    continue;
                }
            }
            else if (lastElementName == "img")
            {
                if (!lastElementAttributes.hasAttribute("en-tag")) {
                    QNTRACE("Skipping img element without en-tag attribute");
                    continue;
                }

                if (lastElementAttributes.hasAttribute("src"))
                {
                    QStringRef enTag = lastElementAttributes.value("en-tag");
                    if (enTag == "en-todo")
                    {
                        QStringRef srcValue = lastElementAttributes.value("src");
                        if (srcValue.contains("qrc:/checkbox_icons/checkbox_no.png")) {
                            writer.writeStartElement("en-todo");
                            ++writeElementCounter;
                            continue;
                        }
                        else if (srcValue.contains("qrc:/checkbox_icons/checkbox_yes.png")) {
                            writer.writeStartElement("en-todo");
                            writer.writeAttribute("checked", "true");
                            ++writeElementCounter;
                            continue;
                        }
                    }
                }
            }

            if (lastElementName == "object")
            {
                if (!lastElementAttributes.hasAttribute("en-tag")) {
                    QNTRACE("Skipping <object> tag without en-tag attribute");
                    continue;
                }

                QStringRef enTag = lastElementAttributes.value("en-tag");
                if (enTag == "en-crypt") {
                    lastElementName = "en-crypt";
                    insideEnCryptElement = true;
                    enCryptAttributes.clear();
                    writer.writeStartElement(lastElementName);
                    ++writeElementCounter;
                    QNTRACE("Started writing en-crypt element");
                    continue;
                }
            }
            else if (lastElementName == "param")
            {
                if (insideEnCryptElement || insideEnMediaElement)
                {
                    if (!lastElementAttributes.hasAttribute("name")) {
                        errorDescription = QT_TR_NOOP("Can't convert note to ENML: can't parse en-crypt or en-media tag: nested param tag doesn't have name attribute");
                        QNWARNING(errorDescription << ", html = " << html << ", cleaned up xml = " << m_cachedConvertedXml);
                        return false;
                    }

                    if (!lastElementAttributes.hasAttribute("value")) {
                        errorDescription = QT_TR_NOOP("Can't convert note to ENML: can't parse en-crypt or en-media tag: nested param tag doesn't have value attribute");
                        QNWARNING(errorDescription << ", html = " << html << ", cleaned up xml = " << m_cachedConvertedXml);
                        return false;
                    }

                    QStringRef name = lastElementAttributes.value("name");
                    QStringRef value = lastElementAttributes.value("value");

                    if (insideEnCryptElement)
                    {
                        if (name != "encrypted-text") {
                            enCryptAttributes.append(name.toString(), value.toString());
                        }
                        else {
                            if (!enCryptAttributes.isEmpty()) {
                                writer.writeAttributes(enCryptAttributes);
                            }
                            writer.writeCharacters(value.toString());
                        }
                    }
                    else if (insideEnMediaElement)
                    {
                        enMediaAttributes.append(name.toString(), value.toString());
                    }
                    else
                    {
                        QNINFO("Skipping <param> tag occasionally encountered when no parsing en-crypt or en-media tags");
                    }
                }

                continue;
            }

            if ((lastElementName == "object") || (lastElementName == "img"))
            {
                if (!lastElementAttributes.hasAttribute("en-tag"))
                {
                    QNTRACE("Skipping <object> or <img> element without en-tag attribute");
                    continue;
                }
                else
                {
                    bool isImage = (lastElementName == "img");
                    QStringRef enTag = lastElementAttributes.value("en-tag");
                    if (enTag == "en-media")
                    {
                        lastElementName = "en-media";
                        writer.writeStartElement(lastElementName);
                        ++writeElementCounter;
                        enMediaAttributes.clear();

                        if (isImage)
                        {
                            // Simple case - all necessary attributes are already in the tag
                            const int numAttributes = lastElementAttributes.size();
                            for(int i = 0; i < numAttributes; ++i)
                            {
                                const QXmlStreamAttribute & attribute = lastElementAttributes[i];
                                const QString attributeQualifiedName = attribute.qualifiedName().toString();
                                const QString attributeValue = attribute.value().toString();

                                if (allowedEnMediaAttributes.contains(attributeQualifiedName)) {
                                    enMediaAttributes.append(attributeQualifiedName, attributeValue);
                                }
                            }

                            writer.writeAttributes(enMediaAttributes);
                            enMediaAttributes.clear();
                            QNTRACE("Wrote en-media element from img element in HTML");
                        }
                        else
                        {
                            // Complicated case - the necessary attributes must be retrieved from nested tags
                            insideEnMediaElement = true;
                            QNTRACE("Started writing en-media element");
                        }

                        continue;
                    }
                }
            }

            // Erasing the forbidden attributes
            for(QXmlStreamAttributes::iterator it = lastElementAttributes.begin(); it != lastElementAttributes.end(); )
            {
                const QStringRef attributeName = it->name();
                if (isForbiddenXhtmlAttribute(attributeName.toString())) {
                    QNTRACE("Erasing the forbidden attribute " << attributeName);
                    it = lastElementAttributes.erase(it);
                    continue;
                }

                ++it;
            }

            writer.writeStartElement(lastElementName);
            writer.writeAttributes(lastElementAttributes);
            ++writeElementCounter;
            QNTRACE("Wrote element: name = " << lastElementName << " and its attributes");
        }

        if ((writeElementCounter > 0) && reader.isCharacters())
        {
            if (insideEnCryptElement || insideEnMediaElement || insideDecryptedEnCryptElement) {
                continue;
            }

            QString text = reader.text().toString();

            if (reader.isCDATA()) {
                writer.writeCDATA(text);
                QNTRACE("Wrote CDATA: " << text);
            }
            else {
                writer.writeCharacters(text);
                QNTRACE("Wrote characters: " << text);
            }
        }

        if ((writeElementCounter > 0) && reader.isEndElement())
        {
            if (insideEnCryptElement)
            {
                if (reader.name() == "object") {
                    insideEnCryptElement = false;
                    enCryptAttributes.clear();
                }
                else {
                    // Don't write end of element corresponding to ends of en-crypt <object> tag's child elements
                    continue;
                }
            }

            if (insideEnMediaElement)
            {
                if ((reader.name() == "object") || (reader.name() == "img")) {
                    insideEnMediaElement = false;
                    writer.writeAttributes(enMediaAttributes);
                    enMediaAttributes.clear();
                }
                else {
                    // Don't write end of element corresponding to ends of en-media <object> or <img> tag's child elements
                    continue;
                }
            }

            if (insideDecryptedEnCryptElement)
            {
                if (reader.name() == "div") {
                    insideDecryptedEnCryptElement = false;
                }
                else {
                    // Don't write end of element corresponding to ends of en-decrypted <div> tag's child elements
                    continue;
                }
            }

            writer.writeEndElement();
            --writeElementCounter;
        }
    }

    if (reader.hasError()) {
        QNWARNING("Error reading html: " << reader.errorString());
        return false;
    }

    QNTRACE("Converted ENML: " << noteContent);

    res = validateEnml(noteContent, errorDescription);
    if (!res)
    {
        if (!errorDescription.isEmpty()) {
            errorDescription = QT_TR_NOOP("Can't validate ENML with DTD: ") + errorDescription;
        }
        else {
            errorDescription = QT_TR_NOOP("Failed to convert, produced ENML is invalid according to dtd");
        }

        QNWARNING(errorDescription << ", ENML: " << noteContent << "\nHTML: " << html);
        return false;
    }

    return true;
}

bool ENMLConverterPrivate::noteContentToHtml(const QString & noteContent, QString & html,
                                             QString & errorDescription,
                                             DecryptedTextCachePtr decryptedTextCache,
                                             const NoteEditorPluginFactory * pluginFactory) const
{
    QNDEBUG("ENMLConverterPrivate::noteContentToHtml: " << noteContent);

    html.resize(0);
    errorDescription.resize(0);

    QXmlStreamReader reader(noteContent);

    QXmlStreamWriter writer(&html);
    writer.setAutoFormatting(true);
    int writeElementCounter = 0;

    QString lastElementName;
    QXmlStreamAttributes lastElementAttributes;

    bool insideEnCryptTag = false;

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartDocument()) {
            continue;
        }

        if (reader.isDTD()) {
            continue;
        }

        if (reader.isEndDocument()) {
            break;
        }

        if (reader.isStartElement())
        {
            ++writeElementCounter;
            lastElementName = reader.name().toString();
            lastElementAttributes = reader.attributes();

            if (lastElementName == "en-note")
            {
                QNTRACE("Replacing en-note with \"body\" tag");
                lastElementName = "body";
            }
            else if (lastElementName == "en-media")
            {
                bool res = resourceInfoToHtml(reader, writer, errorDescription, pluginFactory);
                if (!res) {
                    return false;
                }

                continue;
            }
            else if (lastElementName == "en-crypt")
            {
                insideEnCryptTag = true;
                --writeElementCounter;
                // NOTE: the attributes will be converted to HTML later, along with the characters data
                continue;
            }
            else if (lastElementName == "en-todo")
            {
                toDoTagsToHtml(reader, writer);
                continue;
            }

            // NOTE: do not attempt to process en-todo tags here, it would be done below

            writer.writeStartElement(lastElementName);
            writer.writeAttributes(lastElementAttributes);

            QNTRACE("Wrote start element: " << lastElementName << " and its attributes");
        }

        if ((writeElementCounter > 0) && reader.isCharacters())
        {
            if (reader.isCDATA()) {
                writer.writeCDATA(reader.text().toString());
                QNTRACE("Wrote CDATA: " << reader.text().toString());
            }
            else
            {
                QStringRef text = reader.text();

                if ((lastElementName == "en-crypt") && insideEnCryptTag) {
                    encryptedTextToHtml(lastElementAttributes, text, writer, decryptedTextCache);
                    ++writeElementCounter;
                    insideEnCryptTag = false;
                }
                else {
                    writer.writeCharacters(reader.text().toString());
                    QNTRACE("Wrote characters: " << reader.text().toString());
                }
            }
        }

        if ((writeElementCounter > 0) && reader.isEndElement()) {
            writer.writeEndElement();
            --writeElementCounter;
        }
    }

    if (reader.hasError()) {
        QNWARNING("Error reading ENML: " << reader.errorString());
        return false;
    }

    return true;
}

bool ENMLConverterPrivate::validateEnml(const QString & enml, QString & errorDescription) const
{
    errorDescription.resize(0);

    QByteArray inputBuffer = enml.toLocal8Bit();
    xmlDocPtr pDoc = xmlParseMemory(inputBuffer.constData(), inputBuffer.size());
    if (!pDoc) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't parse enml to xml doc");
        QNWARNING(errorDescription << ": enml = " << enml);
        return false;
    }

    QFile dtdFile(":/enml2.dtd");
    if (!dtdFile.open(QIODevice::ReadOnly)) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't open resource file with DTD");
        QNWARNING(errorDescription << ": enml = " << enml);
        xmlFreeDoc(pDoc);
        return false;
    }

    QByteArray dtdRawData = dtdFile.readAll();

    xmlParserInputBufferPtr pBuf = xmlParserInputBufferCreateMem(dtdRawData.constData(), dtdRawData.size(),
                                                                 XML_CHAR_ENCODING_NONE);
    if (!pBuf) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't allocate input buffer for dtd validation");
        QNWARNING(errorDescription);
        xmlFreeDoc(pDoc);
        return false;
    }

    xmlDtdPtr pDtd = xmlIOParseDTD(NULL, pBuf, XML_CHAR_ENCODING_NONE);
    if (!pDtd) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't parse dtd from buffer");
        QNWARNING(errorDescription);
        xmlFreeParserInputBuffer(pBuf);
        xmlFreeDoc(pDoc);
        return false;
    }

    xmlParserCtxtPtr pContext = xmlNewParserCtxt();
    if (!pContext) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't allocate parses context");
        QNWARNING(errorDescription);
        xmlFreeDtd(pDtd);
        xmlFreeDoc(pDoc);
        return false;
    }

    bool res = static_cast<bool>(xmlValidateDtd(&pContext->vctxt, pDoc, pDtd));

    xmlFreeParserCtxt(pContext);
    xmlFreeDtd(pDtd);
    // WARNING: xmlIOParseDTD should have "consumed" the input buffer so one should not attempt to free it manually
    xmlFreeDoc(pDoc);

    return res;
}

bool ENMLConverterPrivate::noteContentToPlainText(const QString & noteContent, QString & plainText,
                                                  QString & errorMessage)
{
    // FIXME: remake using QXmlStreamReader

    plainText.resize(0);

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

bool ENMLConverterPrivate::noteContentToListOfWords(const QString & noteContent,
                                                    QStringList & listOfWords,
                                                    QString & errorMessage, QString * plainText)
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

QStringList ENMLConverterPrivate::plainTextToListOfWords(const QString & plainText)
{
    // Simply remove all non-word characters from plain text
    return plainText.split(QRegExp("\\W+"), QString::SkipEmptyParts);
}

QString ENMLConverterPrivate::getToDoCheckboxHtml(const bool checked)
{
    QString html = "<img src=\"qrc:/checkbox_icons/checkbox_";
    if (checked) {
        html += "yes.png\" class=\"checkbox_checked\" ";
    }
    else {
        html += "no.png\" class=\"checkbox_unchecked\" ";
    }

    html += "style=\"margin:0px 4px\" "
            "onmouseover=\"style.cursor=\\'default\\'\" "
            "onclick=\"if (this.className == \\'checkbox_unchecked\\') { "
            "this.src=\\'qrc:/checkbox_icons/checkbox_yes.png\\'; this.className = \\'checkbox_checked\\'; } "
            "else { this.src=\\'qrc:/checkbox_icons/checkbox_no.png\\'; this.className = \\'checkbox_unchecked\\' }\" />";
    return html;
}

bool ENMLConverterPrivate::isForbiddenXhtmlTag(const QString & tagName)
{
    auto it = forbiddenXhtmlTags.find(tagName);
    if (it == forbiddenXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::isForbiddenXhtmlAttribute(const QString & attributeName)
{
    auto it = forbiddenXhtmlAttributes.find(attributeName);
    if (it == forbiddenXhtmlAttributes.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::isEvernoteSpecificXhtmlTag(const QString & tagName)
{
    auto it = evernoteSpecificXhtmlTags.find(tagName);
    if (it == evernoteSpecificXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::isAllowedXhtmlTag(const QString & tagName)
{
    auto it = allowedXhtmlTags.find(tagName);
    if (it == allowedXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

void ENMLConverterPrivate::toDoTagsToHtml(const QXmlStreamReader & reader, QXmlStreamWriter & writer) const
{
    QNDEBUG("ENMLConverterPrivate::toDoTagsToHtml");

    QXmlStreamAttributes originalAttributes = reader.attributes();
    bool checked = false;
    if (originalAttributes.hasAttribute("checked")) {
        QStringRef checkedStr = originalAttributes.value("checked");
        if (checkedStr == "true") {
            checked = true;
        }
    }

    QNTRACE("Converting " << (checked ? "completed" : "not yet completed") << " ToDo item");

    writer.writeStartElement("img");

    QXmlStreamAttributes attributes;
    attributes.append("src", QString("qrc:/checkbox_icons/checkbox_") + QString(checked ? "yes" : "no") + QString(".png"));
    attributes.append("class", QString("checkbox_") + QString(checked ? "checked" : "unchecked"));
    attributes.append("en-tag", "en-todo");
    attributes.append("style", "margin:0px 4px");
    attributes.append("onmouseover", "style.cursor=\\'default\\'");
    attributes.append("onclick", "if (this.className == \\'checkbox_unchecked\\') { "
                      "this.src=\\'qrc:/checkbox_icons/checkbox_yes.png\\'; this.className = \\'checkbox_checked\\'; } "
                      "else { this.src=\\'qrc:/checkbox_icons/checkbox_no.png\\'; this.className = \\'checkbox_unchecked\\' }");

    writer.writeAttributes(attributes);
}

bool ENMLConverterPrivate::encryptedTextToHtml(const QXmlStreamAttributes & enCryptAttributes,
                                               const QStringRef & encryptedTextCharacters,
                                               QXmlStreamWriter & writer,
                                               DecryptedTextCachePtr decryptedTextCache) const
{
    QString cipher;
    if (enCryptAttributes.hasAttribute("cipher")) {
        cipher = enCryptAttributes.value("cipher").toString();
    }

    QString length;
    if (enCryptAttributes.hasAttribute("length")) {
        length = enCryptAttributes.value("length").toString();
    }

    QString hint;
    if (enCryptAttributes.hasAttribute("hint")) {
        hint = enCryptAttributes.value("hint").toString();
    }

    if (decryptedTextCache)
    {
        auto it = decryptedTextCache->find(encryptedTextCharacters.toString());
        if ((it != decryptedTextCache->end()) && it.value().second)
        {
            QNTRACE("Found encrypted text which has already been decrypted, "
                    "cached and remembered for the whole session. Encrypted text = "
                    << encryptedTextCharacters);

            writer.writeStartElement("div");
            writer.writeAttribute("en-tag", "en-decrypted");
            writer.writeAttribute("encrypted-text", encryptedTextCharacters.toString());

            if (!cipher.isEmpty()) {
                writer.writeAttribute("cipher", cipher);
            }

            if (!length.isEmpty()) {
                writer.writeAttribute("length", length);
            }

            if (!hint.isEmpty()) {
                writer.writeAttribute("hint", hint);
            }

            writer.writeAttribute("style", "border: 2px solid; "
                                  "border-color: rgb(195, 195, 195); "
                                  "border-radius: 8px; "
                                  "margin: 2px; "
                                  "padding: 2px;");

            writer.writeStartElement("textarea");
            writer.writeAttribute("readonly", "readonly");
            writer.writeCharacters(it.value().first);
            writer.writeEndElement();

            return true;
        }
    }

    writer.writeStartElement("object");
    writer.writeAttribute("en-tag", "en-crypt");

    if (!hint.isEmpty()) {
        writer.writeStartElement("param");
        writer.writeAttribute("name", "hint");
        writer.writeAttribute("value", hint);
        writer.writeEndElement();
    }

    if (!cipher.isEmpty()) {
        writer.writeStartElement("param");
        writer.writeAttribute("name", "cipher");
        writer.writeAttribute("value", cipher);
        writer.writeEndElement();
    }

    if (!length.isEmpty()) {
        writer.writeStartElement("param");
        writer.writeAttribute("name", "length");
        writer.writeAttribute("value", length);
        writer.writeEndElement();
    }

    writer.writeStartElement("param");
    writer.writeAttribute("name", "encrypted-text");
    writer.writeAttribute("value", encryptedTextCharacters.toString());
    writer.writeEndElement();

    QNTRACE("Wrote custom \"object\" element corresponding to en-crypt ENML tag");
    return true;
}

bool ENMLConverterPrivate::resourceInfoToHtml(const QXmlStreamReader & reader,
                                              QXmlStreamWriter & writer,
                                              QString & errorDescription,
                                              const NoteEditorPluginFactory * pluginFactory) const
{
    QNDEBUG("ENMLConverterPrivate::resourceInfoToHtml");

    QXmlStreamAttributes attributes = reader.attributes();

    if (!attributes.hasAttribute("hash")) {
        errorDescription = QT_TR_NOOP("Detected incorrect en-media tag missing hash attribute");
        return false;
    }

    if (!attributes.hasAttribute("type")) {
        errorDescription = QT_TR_NOOP("Detected incorrect en-media tag missing hash attribute");
        return false;
    }

    QStringRef mimeType = attributes.value("type");
    bool convertToImage = false;
    if (mimeType.startsWith("image", Qt::CaseInsensitive))
    {
        if (pluginFactory)
        {
            QRegExp regex("^image\\/.+");
            bool res = pluginFactory->hasPluginForMimeType(regex);
            if (!res) {
                convertToImage = true;
            }
        }
        else
        {
            convertToImage = true;
        }
    }

    writer.writeStartElement(convertToImage ? "img" : "object");

    // NOTE: ENMLConverterPrivate can't set src attribute for img tag as it doesn't know whether the resource is stored
    // in any local file yet. The user of noteContentToHtml should take care of those img tags and their src attributes

    if (convertToImage)
    {
        attributes.append("en-tag", "en-media");
        writer.writeAttributes(attributes);
    }
    else
    {
        writer.writeAttribute("en-tag", "en-media");

        const int numAttributes = attributes.size();
        for(int i = 0; i < numAttributes; ++i)
        {
            const QXmlStreamAttribute & attribute = attributes[i];
            const QString qualifiedName = attribute.qualifiedName().toString();
            if (qualifiedName == "en-tag") {
                continue;
            }

            const QString value = attribute.value().toString();

            writer.writeStartElement("param");
            writer.writeAttribute("name", qualifiedName);
            writer.writeAttribute("value", value);
            writer.writeEndElement();
        }
    }

    return true;
}

bool ENMLConverterPrivate::decryptedTextToEnml(const QXmlStreamReader & reader,
                                               QXmlStreamWriter & writer,
                                               QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::decryptedTextToEnml");

    QXmlStreamAttributes attributes = reader.attributes();

    if (!attributes.hasAttribute("encrypted-text")) {
        errorDescription = QT_TR_NOOP("Missing encrypted text attribute within en-decrypted div tag");
        return false;
    }

    QString encryptedText = attributes.value("encrypted-text").toString();

    QString hint;
    if (attributes.hasAttribute("hint")) {
        hint = attributes.value("hint").toString();
    }

    writer.writeStartElement("en-crypt");

    if (attributes.hasAttribute("cipher")) {
        writer.writeAttribute("cipher", attributes.value("cipher").toString());
    }

    if (attributes.hasAttribute("length")) {
        writer.writeAttribute("length", attributes.value("length").toString());
    }

    if (!hint.isEmpty()) {
        writer.writeAttribute("hint", hint);
    }

    writer.writeCharacters(encryptedText);

    QNTRACE("Wrote en-crypt ENML tag from en-decrypted div tag");
    return true;
}

} // namespace qute_note

