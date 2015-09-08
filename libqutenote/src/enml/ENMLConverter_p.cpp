#include "ENMLConverter_p.h"
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/enml/HTMLCleaner.h>

#ifndef USE_QT_WEB_ENGINE
#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#endif

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

bool ENMLConverterPrivate::htmlToNoteContent(const QString & html, QString & noteContent, DecryptedTextManager &decryptedTextManager, QString & errorDescription) const
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

    QNTRACE("HTML converted to XML by tidy: " << m_cachedConvertedXml);

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

            if ( ((lastElementName == "div") || (lastElementName == "img")) &&
                 lastElementAttributes.hasAttribute("en-tag") )
            {
                const QString enTag = lastElementAttributes.value("en-tag").toString();
                if (enTag == "en-decrypted")
                {
                    QNTRACE("Found decrypted text area, need to convert it back to en-crypt form");
                    bool res = decryptedTextToEnml(reader, decryptedTextManager, writer, errorDescription);
                    if (!res) {
                        return false;
                    }

                    continue;
                }
#ifdef USE_QT_WEB_ENGINE
                else if (enTag == "en-media")
                {
                    QXmlStreamAttributes filteredAttributes;
                    const int numAttributes = lastElementAttributes.size();
                    filteredAttributes.reserve(std::max(numAttributes - 2, 0)); // All except en-tag and class should fit
                    for(int i = 0; i < numAttributes; ++i)
                    {
                        const QStringRef attributeName = lastElementAttributes[i].name();
                        if ((attributeName == "en-tag") || (attributeName == "class") || (attributeName == "src")) {
                            continue;
                        }

                        filteredAttributes.push_back(lastElementAttributes[i]);
                    }

                    writer.writeStartElement("en-media");
                    writer.writeAttributes(filteredAttributes);
                    writer.writeEndElement();

                    Q_UNUSED(reader.readElementText(QXmlStreamReader::SkipChildElements));
                    continue;
                }
#endif
            }

            if (lastElementName == "img")
            {
                if (!lastElementAttributes.hasAttribute("en-tag")) {
                    QNTRACE("Skipping img element without en-tag attribute");
                    continue;
                }

                QStringRef enTag = lastElementAttributes.value("en-tag");
                if ((enTag == "en-todo") && lastElementAttributes.hasAttribute("src"))
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
#ifdef USE_QT_WEB_ENGINE
                else if (enTag == "en-crypt")
                {
                    const QXmlStreamAttributes attributes = reader.attributes();
                    QXmlStreamAttributes enCryptAttributes;

                    if (attributes.hasAttribute("cipher")) {
                        enCryptAttributes.append("cipher", attributes.value("cipher").toString());
                    }

                    if (attributes.hasAttribute("length")) {
                        enCryptAttributes.append("length", attributes.value("length").toString());
                    }

                    if (!attributes.hasAttribute("encrypted_text")) {
                        errorDescription = QT_TR_NOOP("Found en-crypt tag without encrypted_text attribute");
                        return false;
                    }

                    if (attributes.hasAttribute("hint")) {
                        enCryptAttributes.append("hint", attributes.value("hint").toString());
                    }

                    writer.writeStartElement("en-crypt");
                    writer.writeAttributes(enCryptAttributes);
                    writer.writeCharacters(attributes.value("encrypted_text").toString());
                    ++writeElementCounter;
                    QNTRACE("Started writing en-crypt tag");
                    continue;
                }
#endif
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
            if (insideEnCryptElement || insideEnMediaElement) {
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

            writer.writeEndElement();
            --writeElementCounter;
        }
    }

    if (reader.hasError()) {
        errorDescription = reader.errorString();
        QNWARNING("Error reading html: " << errorDescription
                  << ", HTML: " << html << "\nXML: " << m_cachedConvertedXml);
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
                                             DecryptedTextManager & decryptedTextManager
#ifndef USE_QT_WEB_ENGINE
                                             , const NoteEditorPluginFactory * pluginFactory
#endif
                                             ) const
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
                bool res = resourceInfoToHtml(reader, writer, errorDescription
#ifndef USE_QT_WEB_ENGINE
                                              , pluginFactory
#endif
                                              );
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
                    encryptedTextToHtml(lastElementAttributes, text, writer, decryptedTextManager);
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

    html += "en-tag=\"en-todo\" />";
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
    writer.writeAttributes(attributes);
}

bool ENMLConverterPrivate::encryptedTextToHtml(const QXmlStreamAttributes & enCryptAttributes,
                                               const QStringRef & encryptedTextCharacters,
                                               QXmlStreamWriter & writer,
                                               DecryptedTextManager & decryptedTextManager) const
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

    QString decryptedText;
    bool rememberForSession = false;
    bool foundDecryptedText = decryptedTextManager.findDecryptedTextByEncryptedText(encryptedTextCharacters.toString(),
                                                                                    decryptedText, rememberForSession);
    if (foundDecryptedText)
    {
        QNTRACE("Found encrypted text which has already been decrypted and cached; "
                "encrypted text = " << encryptedTextCharacters);

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

        QString formattedDecryptedText = decryptedText;
        formattedDecryptedText.prepend("<?xml version=\"1.0\"?>"
                                       "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
                                       "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
                                       "<div class=\"en-decrypted hvr-border-color\">");
        formattedDecryptedText.append("</div>");

        QXmlStreamReader decryptedTextReader(formattedDecryptedText);
        bool foundFormattedText = false;

        while(!decryptedTextReader.atEnd())
        {
            Q_UNUSED(decryptedTextReader.readNext());

            if (decryptedTextReader.isStartElement()) {
                writer.writeStartElement(decryptedTextReader.name().toString());
                writer.writeAttributes(decryptedTextReader.attributes());
                foundFormattedText = true;
                QNTRACE("Wrote start element from decrypted text: " << decryptedTextReader.name());
            }

            if (decryptedTextReader.isCharacters()) {
                writer.writeCharacters(decryptedTextReader.text().toString());
                foundFormattedText = true;
                QNTRACE("Wrote characters from decrypted text: " << decryptedTextReader.text());
            }

            if (decryptedTextReader.isEndElement()) {
                writer.writeEndElement();
                QNTRACE("Wrote end element from decrypted text: " << decryptedTextReader.name());
            }
        }

        if (decryptedTextReader.hasError()) {
            QNWARNING("Decrypted text reader has error: " << decryptedTextReader.errorString());
        }

        if (!foundFormattedText) {
            writer.writeCharacters(decryptedText);
            QNTRACE("Wrote unformatted decrypted text: " << decryptedText);
        }

        return true;
    }

#ifndef USE_QT_WEB_ENGINE
    writer.writeStartElement("object");
    writer.writeAttribute("type", "application/vnd.qutenote.encrypt");
#else
    writer.writeStartElement("img");
    writer.writeAttribute("src", QString());
#endif

    writer.writeAttribute("en-tag", "en-crypt");
    writer.writeAttribute("class", "en-crypt hvr-border-color");

    if (!hint.isEmpty())
    {
#ifndef USE_QT_WEB_ENGINE
        writer.writeStartElement("param");
        writer.writeAttribute("name", "hint");
        writer.writeAttribute("value", hint);
        writer.writeEndElement();
#else
        writer.writeAttribute("hint", hint);
#endif
    }

    if (!cipher.isEmpty())
    {
#ifndef USE_QT_WEB_ENGINE
        writer.writeStartElement("param");
        writer.writeAttribute("name", "cipher");
        writer.writeAttribute("value", cipher);
        writer.writeEndElement();
#else
        writer.writeAttribute("cipher", cipher);
#endif
    }

    if (!length.isEmpty())
    {
#ifndef USE_QT_WEB_ENGINE
        writer.writeStartElement("param");
        writer.writeAttribute("name", "length");
        writer.writeAttribute("value", length);
        writer.writeEndElement();
#else
        writer.writeAttribute("length", length);
#endif
    }

#ifndef USE_QT_WEB_ENGINE
    writer.writeStartElement("param");
    writer.writeAttribute("name", "encrypted-text");
    writer.writeAttribute("value", encryptedTextCharacters.toString());
    writer.writeEndElement();
#else
    writer.writeAttribute("encrypted_text", encryptedTextCharacters.toString());
#endif

    QNTRACE("Wrote custom "
#ifndef USE_QT_WEB_ENGINE
            "\"object\" "
#else
            "\"img\" "
#endif
            "element corresponding to en-crypt ENML tag");
    return true;
}

bool ENMLConverterPrivate::resourceInfoToHtml(const QXmlStreamReader & reader,
                                              QXmlStreamWriter & writer,
                                              QString & errorDescription
#ifndef USE_QT_WEB_ENGINE
                                              , const NoteEditorPluginFactory * pluginFactory
#endif
                                              ) const
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
#ifndef USE_QT_WEB_ENGINE
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
#else
        convertToImage = true;
#endif
    }

    writer.writeStartElement(convertToImage
                             ? "img"
#ifndef USE_QT_WEB_ENGINE
                             : "object");
#else
                             : "div");
#endif

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
        writer.writeAttribute("class", "en-media-generic hvr-border-color");

#ifndef USE_QT_WEB_ENGINE
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
#else
        writer.writeAttributes(attributes);
        writer.writeAttribute("class", "en-media-generic hvr-border-color");

        // Resource icon
        writer.writeStartElement("img");
        writer.writeAttribute("class", "resource-icon");

        writer.writeAttribute("src", "qrc:/generic_resource_icons/png/attachment.png");

        QStringRef mimeType = attributes.value("type");
        QNTRACE("Resource mime type: " << mimeType);
        writer.writeAttribute("type", (mimeType.isNull() ? QString() : mimeType.toString()));

        writer.writeEndElement();

        // Resource name div - the actual name is not specified here, let JavaScript communicating with NoteEditor's C++ code handle it
        writer.writeStartElement("div");
        writer.writeAttribute("class", "resource-name");
        writer.writeAttribute("readonly", "true");
        writer.writeCharacters("Sample resource name");  // TODO: remove later, will be replaced with proper resource name
        writer.writeEndElement();

        // Resource size div - the actual size is not specified here as well as the name
        writer.writeStartElement("div");
        writer.writeAttribute("class", "resource-size");
        writer.writeCharacters("Sample resource size");  // TODO: remove later, will be replaced with proper resource size
        writer.writeEndElement();

        // Open resource button
        writer.writeStartElement("img");
        writer.writeAttribute("class", "open-resource-button");

        // TODO: remove later
        writer.writeAttribute("src", "qrc:/generic_resource_icons/png/open_with.png");

        writer.writeEndElement();

        // Save resource to file button
        writer.writeStartElement("img");
        writer.writeAttribute("class", "save-resource-button");

        // TODO: remove later
        writer.writeAttribute("src", "qrc:/generic_resource_icons/png/save.png");

        writer.writeEndElement();
#endif
    }

    return true;
}

bool ENMLConverterPrivate::decryptedTextToEnml(QXmlStreamReader & reader,
                                               DecryptedTextManager & decryptedTextManager,
                                               QXmlStreamWriter & writer, QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::decryptedTextToEnml");

    const QXmlStreamAttributes attributes = reader.attributes();
    if (!attributes.hasAttribute("encrypted-text")) {
        errorDescription = QT_TR_NOOP("Missing encrypted text attribute within en-decrypted div tag");
        return false;
    }

    QString encryptedText = attributes.value("encrypted-text").toString();

    QString storedDecryptedText;
    bool rememberForSession = false;
    bool res = decryptedTextManager.findDecryptedTextByEncryptedText(encryptedText, storedDecryptedText, rememberForSession);
    if (!res) {
        errorDescription = QT_TR_NOOP("Can't find decrypted text by its encrypted text");
        QNWARNING(errorDescription);
        return false;
    }

    QString actualDecryptedText;
    QXmlStreamWriter decryptedTextWriter(&actualDecryptedText);

    int nestedElementsCounter = 0;
    while(!reader.atEnd())
    {
        reader.readNext();

        if (reader.isStartElement()) {
            decryptedTextWriter.writeStartElement(reader.name().toString());
            decryptedTextWriter.writeAttributes(reader.attributes());
            ++nestedElementsCounter;
        }

        if (reader.isCharacters()) {
            decryptedTextWriter.writeCharacters(reader.text().toString());
        }

        if (reader.isEndElement())
        {
            if (nestedElementsCounter > 0) {
                decryptedTextWriter.writeEndElement();
                --nestedElementsCounter;
            }
            else {
                break;
            }
        }
    }

    if (reader.hasError()) {
        errorDescription = reader.errorString();
        QNWARNING("Couldn't read the nested contents of en-decrypted div, reader has error: "
                  << errorDescription);
        return false;
    }

    if (storedDecryptedText != actualDecryptedText)
    {
        QNTRACE("Found modified decrypted text, need to re-encrypt");

        QString actualEncryptedText;
        res = decryptedTextManager.modifyDecryptedText(encryptedText, actualDecryptedText, actualEncryptedText);
        if (res) {
            QNTRACE("Re-evaluated the modified decrypted text's encrypted text; was: "
                    << encryptedText << "; new: " << actualEncryptedText);
            encryptedText = actualEncryptedText;
        }
    }

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
    writer.writeEndElement();

    QNTRACE("Wrote en-crypt ENML tag from en-decrypted div tag");
    return true;
}

} // namespace qute_note

