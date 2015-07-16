#include "ENMLConverter_p.h"
#include "HTMLCleaner.h"
#include <note_editor/NoteEditorPluginFactory.h>
#include <client/types/Note.h>
#include <logging/QuteNoteLogger.h>
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
    m_cachedNoteContent(),
    m_cachedConvertedXml()
{}

ENMLConverterPrivate::~ENMLConverterPrivate()
{
    delete m_pHtmlCleaner;
}

bool ENMLConverterPrivate::htmlToNoteContent(const QString & html, Note & note, QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::htmlToNoteContent: note local guid = " << note.localGuid());

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

    m_cachedNoteContent.resize(0);
    QXmlStreamWriter writer(&m_cachedNoteContent);

    int writeElementCounter = 0;

    QString lastElementName;
    QXmlStreamAttributes lastElementAttributes;
    bool insideEnCryptElement = false;
    QXmlStreamAttributes enCryptAttributes;

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartDocument()) {
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
                }
            }
            else if (lastElementName == "img")
            {
                if (lastElementAttributes.hasAttribute("src"))
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
                else
                {
                    errorDescription = QT_TR_NOOP("Can't convert note to ENML: found img html tag without src attribute");
                    QNWARNING(errorDescription << ", html = " << html << ", cleaned up xml = " << m_cachedConvertedXml);
                    return false;
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
                if (insideEnCryptElement)
                {
                    if (!lastElementAttributes.hasAttribute("name")) {
                        errorDescription = QT_TR_NOOP("Can't convert note to ENML: can't parse en-crypt tag: nested param tag doesn't have name attribute");
                        QNWARNING(errorDescription << ", html = " << html << ", cleaned up xml = " << m_cachedConvertedXml);
                        return false;
                    }

                    if (!lastElementAttributes.hasAttribute("value")) {
                        errorDescription = QT_TR_NOOP("Can't convert note to ENML: can't parse en-crypt tag: nested param tag doesn't have value attribute");
                        QNWARNING(errorDescription << ", html = " << html << ", cleaned up xml = " << m_cachedConvertedXml);
                        return false;
                    }

                    QStringRef name = lastElementAttributes.value("name");
                    QStringRef value = lastElementAttributes.value("value");

                    if (name != "encryptedText") {
                        enCryptAttributes.append(name.toString(), value.toString());
                    }
                    else {
                        if (!enCryptAttributes.isEmpty()) {
                            writer.writeAttributes(enCryptAttributes);
                        }
                        writer.writeCharacters(value.toString());
                    }

                    continue;
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
            if (insideEnCryptElement) {
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

            writer.writeEndElement();
            --writeElementCounter;
        }
    }

    QNTRACE("Converted ENML: " << m_cachedNoteContent);

    res = validateEnml(m_cachedNoteContent, errorDescription);
    if (!res)
    {
        if (!errorDescription.isEmpty()) {
            errorDescription = QT_TR_NOOP("Can't validate ENML with DTD: ") + errorDescription;
        }
        else {
            errorDescription = QT_TR_NOOP("Failed to convert, produced ENML is invalid according to dtd");
        }

        return false;
    }

    note.setContent(m_cachedNoteContent);
    return true;
}

bool ENMLConverterPrivate::noteContentToHtml(const Note & note, QString & html,
                                             QString & errorDescription,
                                             DecryptedTextCachePtr decryptedTextCache,
                                             const NoteEditorPluginFactory * pluginFactory) const
{
    QNDEBUG("ENMLConverterPrivate::noteContentToHtml: note local guid = " << note.localGuid());

    html.resize(0);
    errorDescription.resize(0);

    if (!note.hasContent()) {
        return true;
    }

    QXmlStreamReader reader(note.content());

    QXmlStreamWriter writer(&html);
    int writeElementCounter = 0;

    QString lastElementName;
    QXmlStreamAttributes lastElementAttributes;

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartDocument()) {
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
                if (lastElementName == "en-crypt")
                {
                    encryptedTextToHtml(lastElementAttributes, text, writer, decryptedTextCache);
                }
                else
                {
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

bool ENMLConverterPrivate::objectToEnml(QXmlStreamReader & reader, QXmlStreamWriter & writer,
                                        QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::objectToEnml");

    QXmlStreamAttributes attributes = reader.attributes();
    if (!attributes.hasAttribute("en-tag")) {
        QNTRACE("This <object> tag doen't have en-tag attribute, ignoring it");
        return true;
    }

    QString enTagValue = attributes.value("en-tag").toString();
    if (enTagValue == "en-crypt") {
        return encryptedTextToEnml(reader, writer, errorDescription);
    }
    else if (enTagValue == "en-media") {
        return resourceInfoToEnml(reader, writer, errorDescription);
    }

    QNINFO("Detected unknown value of en-tag attribute within object tag: " << enTagValue);
    return false;
}

bool ENMLConverterPrivate::encryptedTextToEnml(QXmlStreamReader & reader,
                                               QXmlStreamWriter & writer,
                                               QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::encryptedTextToEnml");

    QString hint;
    QString cipher;
    QString length;
    QString encryptedText;

    while(reader.readNextStartElement())
    {
        QString elementName = reader.name().toString();
        if (elementName != "param")  {
            break;
        }

        QXmlStreamAttributes paramAttributes = reader.attributes();
        if (!paramAttributes.hasAttribute("name")) {
            errorDescription = QT_TR_NOOP("Detected invalid HTML: object tag with en-tag = en-crypt "
                                          "has child param tag without name attribute");
            return false;
        }

        if (!paramAttributes.hasAttribute("value")) {
            errorDescription = QT_TR_NOOP("Detected invalid HTML: object tag with en-tag = en-crypt "
                                          "has child param tag without value attribute");
            return false;
        }

        QString paramName = paramAttributes.value("name").toString();
        QString paramValue = paramAttributes.value("value").toString();

        if (paramName == "hint") {
            hint = paramValue;
        }
        else if (paramName == "cipher") {
            cipher = paramValue;
        }
        else if (paramName == "length") {
            length = paramValue;
        }
        else if (paramName == "encryptedText") {
            encryptedText = paramValue;
        }
        else {
            errorDescription = QT_TR_NOOP("Detected invalid HTML: object tag with en-tag = en-crypt "
                                          "has child param tag with unidentified name attribute: ") + paramName;
            return false;
        }
    }

    if (encryptedText.isEmpty()) {
        errorDescription = QT_TR_NOOP("Couldn't find encrypted text for en-crypt ENML tag");
        return false;
    }

    writer.writeStartElement("en-crypt");
    if (!cipher.isEmpty()) {
        writer.writeAttribute("cipher", cipher);
    }

    if (!length.isEmpty()) {
        writer.writeAttribute("length", length);
    }

    if (!hint.isEmpty()) {
        writer.writeAttribute("hint", hint);
    }

    writer.writeCDATA(encryptedText);
    QNTRACE("Wrote en-crypt ENML element: cipher = " << cipher << ", length = " << length
            << ", encryptedText = " << encryptedText << ", hint = " << hint);
    return true;
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
            writer.writeAttribute("encryptedText", encryptedTextCharacters.toString());

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
            writer.writeCDATA(it.value().first);
            writer.writeEndElement();

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
    writer.writeAttribute("name", "encryptedText");
    writer.writeAttribute("value", encryptedTextCharacters.toString());
    writer.writeEndElement();

    QNTRACE("Wrote custom \"object\" element corresponding to en-crypt ENML tag");
    return true;
}

bool ENMLConverterPrivate::resourceInfoToEnml(const QXmlStreamReader & reader,
                                              QXmlStreamWriter & writer,
                                              QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::resourceInfoToEnml");

    QXmlStreamAttributes attributes = reader.attributes();

    if (!attributes.hasAttribute("hash")) {
        errorDescription = QT_TR_NOOP("Detected resource tag without hash attribute");
        return false;
    }

    if (!attributes.hasAttribute("type")) {
        errorDescription = QT_TR_NOOP("Detected resource tag without type attribute");
        return false;
    }

    for(QXmlStreamAttributes::Iterator it = attributes.begin(); it != attributes.end(); )
    {
        QXmlStreamAttribute & attribute = *it;
        if ( !allowedEnMediaAttributes.contains(attribute.name().toString()) &&
             ((attribute.namespaceUri() != "xml") || (attribute.name() != "lang")) )
        {
            it = attributes.erase(it);
            continue;
        }

        ++it;
    }

    writer.writeStartElement("en-media");
    writer.writeAttributes(attributes);
    writer.writeEndElement();

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

    attributes.append("en-tag", "en-media");
    writer.writeAttributes(attributes);

    return true;
}

bool ENMLConverterPrivate::decryptedTextToEnml(const QXmlStreamReader & reader,
                                               QXmlStreamWriter & writer,
                                               QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::decryptedTextToEnml");

    QXmlStreamAttributes attributes = reader.attributes();

    if (!attributes.hasAttribute("encryptedText")) {
        errorDescription = QT_TR_NOOP("Missing encrypted text attribute within en-decrypted div tag");
        return false;
    }

    if (!attributes.hasAttribute("cipher")) {
        errorDescription = QT_TR_NOOP("Missing cipher attribute within en-decrypted div tag");
        return false;
    }

    if (!attributes.hasAttribute("length")) {
        errorDescription = QT_TR_NOOP("Missing length attribute within en-dectyped dig tag");
        return false;
    }

    QString encryptedText = attributes.value("encryptedText").toString();
    QString cipher = attributes.value("cipher").toString();
    QString length = attributes.value("length").toString();

    QString hint;
    if (attributes.hasAttribute("hint")) {
        hint = attributes.value("hint").toString();
    }

    writer.writeStartElement("en-crypt");
    writer.writeAttribute("cipher", cipher);
    writer.writeAttribute("length", length);
    if (!hint.isEmpty()) {
        writer.writeAttribute("hint", hint);
    }

    writer.writeCDATA(encryptedText);
    writer.writeEndElement();

    QNTRACE("Wrote en-crypt ENML tag from en-decrypted div tag");
    return true;
}

} // namespace qute_note

