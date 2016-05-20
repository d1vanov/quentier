#include "ENMLConverter_p.h"
#include <qute_note/note_editor/DecryptedTextManager.h>
#include <qute_note/enml/HTMLCleaner.h>
#include <qute_note/types/IResource.h>
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
    m_pHtmlCleaner(Q_NULLPTR),
    m_cachedConvertedXml()
{}

ENMLConverterPrivate::~ENMLConverterPrivate()
{
    delete m_pHtmlCleaner;
}

bool ENMLConverterPrivate::htmlToNoteContent(const QString & html, const QVector<SkipHtmlElementRule> & skipRules,
                                             QString & noteContent, DecryptedTextManager & decryptedTextManager,
                                             QString & errorDescription) const
{
    QNDEBUG("ENMLConverterPrivate::htmlToNoteContent: " << html
            << "\nskip element rules: " << skipRules);

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

    bool insideEnMediaElement = false;
    QXmlStreamAttributes enMediaAttributes;

    size_t  skippedElementNestingCounter = 0;
    size_t  skippedElementWithPreservedContentsNestingCounter = 0;

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
            if (skippedElementNestingCounter) {
                QNTRACE("Skipping everyting inside element skipped together with its contents by the rules");
                ++skippedElementNestingCounter;
                continue;
            }

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
            if ((tagIt != forbiddenXhtmlTags.end()) && (lastElementName != "object")) {
                QNTRACE("Skipping forbidden XHTML tag: " << lastElementName);
                continue;
            }

            tagIt = allowedXhtmlTags.find(lastElementName);
            if (tagIt == allowedXhtmlTags.end())
            {
                tagIt = evernoteSpecificXhtmlTags.find(lastElementName);
                if (tagIt == evernoteSpecificXhtmlTags.end()) {
                    QNTRACE("Haven't found tag " << lastElementName << " within the list of allowed XHTML tags "
                            "or within Evernote-specific tags, skipping it");
                    continue;
                }
            }

            lastElementAttributes = reader.attributes();

            ShouldSkipElementResult::type shouldSkip = shouldSkipElement(lastElementName, lastElementAttributes, skipRules);
            if (shouldSkip != ShouldSkipElementResult::ShouldNotSkip)
            {
                QNTRACE("Skipping element " << lastElementName << " per skip rules; the contents would be "
                        << (shouldSkip == ShouldSkipElementResult::SkipWithContents ? "skipped" : "preserved"));

                if (shouldSkip == ShouldSkipElementResult::SkipWithContents) {
                    ++skippedElementNestingCounter;
                }
                else if (shouldSkip == ShouldSkipElementResult::SkipButPreserveContents) {
                    ++skippedElementWithPreservedContentsNestingCounter;
                }

                continue;
            }

            if ( ((lastElementName == "img") || (lastElementName == "object") || (lastElementName == "div")) &&
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
                else if (enTag == "en-todo")
                {
                    if (!lastElementAttributes.hasAttribute("src")) {
                        QNWARNING("Found en-todo tag without src attribute");
                        continue;
                    }

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
                    insideEnCryptElement = true;
                    continue;
                }
                else if (enTag == "en-media")
                {
                    bool isImage = (lastElementName == "img");
                    lastElementName = "en-media";
                    writer.writeStartElement(lastElementName);
                    ++writeElementCounter;
                    enMediaAttributes.clear();
                    insideEnMediaElement = true;

                    const int numAttributes = lastElementAttributes.size();
                    for(int i = 0; i < numAttributes; ++i)
                    {
                        const QXmlStreamAttribute & attribute = lastElementAttributes[i];
                        const QString attributeQualifiedName = attribute.qualifiedName().toString();
                        const QString attributeValue = attribute.value().toString();

                        if (!isImage)
                        {
                            if (attributeQualifiedName == "resource-mime-type") {
                                enMediaAttributes.append("type", attributeValue);
                            }
                            else if (allowedEnMediaAttributes.contains(attributeQualifiedName) && (attributeQualifiedName != "type")) {
                                enMediaAttributes.append(attributeQualifiedName, attributeValue);
                            }
                        }
                        else if (allowedEnMediaAttributes.contains(attributeQualifiedName)) { // img
                            enMediaAttributes.append(attributeQualifiedName, attributeValue);
                        }
                    }

                    writer.writeAttributes(enMediaAttributes);
                    enMediaAttributes.clear();
                    QNTRACE("Wrote en-media element from img element in HTML");

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

                if ((lastElementName == "a") && (attributeName == "en-hyperlink-id")) {
                    QNTRACE("Erasing custom attribute en-hyperlink-id");
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
            if (skippedElementNestingCounter) {
                continue;
            }

            if (insideEnMediaElement) {
                continue;
            }

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

        if (reader.isEndElement())
        {
            if (skippedElementNestingCounter) {
                --skippedElementNestingCounter;
                continue;
            }

            if (skippedElementWithPreservedContentsNestingCounter) {
                --skippedElementWithPreservedContentsNestingCounter;
                continue;
            }

            if (writeElementCounter <= 0) {
                continue;
            }

            if (insideEnMediaElement) {
                insideEnMediaElement = false;
            }

            if (insideEnCryptElement) {
                insideEnCryptElement = false;
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
                                             DecryptedTextManager & decryptedTextManager,
                                             NoteContentToHtmlExtraData & extraData) const
{
    QNDEBUG("ENMLConverterPrivate::noteContentToHtml: " << noteContent);

    extraData.m_numEnToDoNodes = 0;
    extraData.m_numHyperlinkNodes = 0;
    extraData.m_numEnCryptNodes = 0;
    extraData.m_numEnDecryptedNodes = 0;

    html.resize(0);
    errorDescription.resize(0);

    QXmlStreamReader reader(noteContent);

    QXmlStreamWriter writer(&html);
    writer.setAutoFormatting(true);
    int writeElementCounter = 0;

    bool insideEnCryptTag = false;

    QString lastElementName;
    QXmlStreamAttributes lastElementAttributes;

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
                bool res = resourceInfoToHtml(lastElementAttributes, writer, errorDescription);
                if (!res) {
                    return false;
                }

                continue;
            }
            else if (lastElementName == "en-crypt")
            {
                insideEnCryptTag = true;
                continue;
            }
            else if (lastElementName == "en-todo")
            {
                quint64 enToDoIndex = extraData.m_numEnToDoNodes + 1;
                toDoTagsToHtml(reader, enToDoIndex, writer);
                ++extraData.m_numEnToDoNodes;
                continue;
            }
            else if (lastElementName == "a")
            {
                quint64 hyperlinkIndex = extraData.m_numHyperlinkNodes + 1;
                lastElementAttributes.append("en-hyperlink-id", QString::number(hyperlinkIndex));
                ++extraData.m_numHyperlinkNodes;
            }

            // NOTE: do not attempt to process en-todo tags here, it would be done below

            writer.writeStartElement(lastElementName);
            writer.writeAttributes(lastElementAttributes);

            QNTRACE("Wrote start element: " << lastElementName << " and its attributes");
        }

        if ((writeElementCounter > 0) && reader.isCharacters())
        {
            if (insideEnCryptTag)
            {
                quint64 enCryptIndex = extraData.m_numEnCryptNodes + 1;
                quint64 enDecryptedIndex = extraData.m_numEnDecryptedNodes + 1;
                bool convertedToEnCryptNode = false;

                encryptedTextToHtml(lastElementAttributes, reader.text(), enCryptIndex,
                                    enDecryptedIndex, writer, decryptedTextManager,
                                    convertedToEnCryptNode);

                if (convertedToEnCryptNode) {
                    ++extraData.m_numEnCryptNodes;
                }
                else {
                    ++extraData.m_numEnDecryptedNodes;
                }

                insideEnCryptTag = false;
                continue;
            }

            if (reader.isCDATA()) {
                writer.writeCDATA(reader.text().toString());
                QNTRACE("Wrote CDATA: " << reader.text().toString());
            }
            else {
                writer.writeCharacters(reader.text().toString());
                QNTRACE("Wrote characters: " << reader.text().toString());
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
    QNDEBUG("ENMLConverterPrivate::noteContentToPlainText: " << noteContent);

    plainText.resize(0);

    QXmlStreamReader reader(noteContent);
    QString lastElementName;

    bool skipIteration = false;
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
            const QStringRef element = reader.name();
            if ((element == "en-media") || (element == "en-crypt")) {
                skipIteration = true;
            }

            continue;
        }

        if (reader.isEndElement())
        {
            const QStringRef element = reader.name();
            if ((element == "en-media") || (element == "en-crypt")) {
                skipIteration = false;
            }

            continue;
        }

        if (reader.isCharacters() && !skipIteration) {
            plainText += reader.text();
        }
    }

    if (Q_UNLIKELY(reader.hasError())) {
        errorMessage = QT_TR_NOOP("Encountered error when trying to convert the note content to plain text: ");
        errorMessage += reader.errorString();
        return false;
    }

    return true;
}

bool ENMLConverterPrivate::noteContentToListOfWords(const QString & noteContent,
                                                    QStringList & listOfWords,
                                                    QString & errorMessage, QString * plainText)
{
    QString localPlainText;
    bool res = noteContentToPlainText(noteContent, localPlainText, errorMessage);
    if (!res) {
        listOfWords.clear();
        return false;
    }

    if (plainText) {
        *plainText = localPlainText;
    }

    listOfWords = plainTextToListOfWords(localPlainText);
    return true;
}

QStringList ENMLConverterPrivate::plainTextToListOfWords(const QString & plainText)
{
    // Simply remove all non-word characters from plain text
    return plainText.split(QRegExp("\\W+"), QString::SkipEmptyParts);
}

QString ENMLConverterPrivate::toDoCheckboxHtml(const bool checked, const quint64 idNumber)
{
    QString html = "<img src=\"qrc:/checkbox_icons/checkbox_";
    if (checked) {
        html += "yes.png\" class=\"checkbox_checked\" ";
    }
    else {
        html += "no.png\" class=\"checkbox_unchecked\" ";
    }

    html += "en-tag=\"en-todo\" en-todo-id=\"";
    html += QString::number(idNumber);
    html += "\" />";
    return html;
}

QString ENMLConverterPrivate::encryptedTextHtml(const QString & encryptedText, const QString & hint,
                                                const QString & cipher, const size_t keyLength,
                                                const quint64 enCryptIndex)
{
    QString encryptedTextHtmlObject;

#ifdef USE_QT_WEB_ENGINE
    encryptedTextHtmlObject = "<img ";
#else
    encryptedTextHtmlObject = "<object type=\"application/vnd.qutenote.encrypt\" ";
#endif
    encryptedTextHtmlObject +=  "en-tag=\"en-crypt\" cipher=\"";
    encryptedTextHtmlObject += cipher;
    encryptedTextHtmlObject += "\" length=\"";
    encryptedTextHtmlObject += QString::number(keyLength);
    encryptedTextHtmlObject += "\" class=\"en-crypt hvr-border-color\" encrypted_text=\"";
    encryptedTextHtmlObject += encryptedText;
    encryptedTextHtmlObject += "\" en-crypt-id=\"";
    encryptedTextHtmlObject += QString::number(enCryptIndex);
    encryptedTextHtmlObject += "\" ";

    if (!hint.isEmpty())
    {
        encryptedTextHtmlObject += "hint=\"";

        QString hintWithEscapedDoubleQuotes = hint;
        escapeString(hintWithEscapedDoubleQuotes, /* simplify = */ true);

        encryptedTextHtmlObject += hintWithEscapedDoubleQuotes;
        encryptedTextHtmlObject += "\" ";
    }

#ifdef USE_QT_WEB_ENGINE
    encryptedTextHtmlObject += " />";
#else
    encryptedTextHtmlObject += ">some fake characters to prevent self-enclosing html tag confusing webkit</object>";
#endif

    return encryptedTextHtmlObject;
}

QString ENMLConverterPrivate::decryptedTextHtml(const QString & decryptedText, const QString & encryptedText,
                                                const QString & hint, const QString & cipher,
                                                const size_t keyLength, const quint64 enDecryptedIndex)
{
    QString result;
    QXmlStreamWriter writer(&result);
    decryptedTextHtml(decryptedText, encryptedText, hint, cipher, keyLength, enDecryptedIndex, writer);
    writer.writeEndElement();
    return result;
}

QString ENMLConverterPrivate::resourceHtml(const IResource & resource, QString & errorDescription)
{
    QNDEBUG("ENMLConverterPrivate::resourceHtml");

    if (Q_UNLIKELY(!resource.hasDataHash())) {
        errorDescription = QT_TR_NOOP("Can't compose the html representation of the resource: no data hash is set");
        QNWARNING(errorDescription << ", resource: " << resource);
        return QString();
    }

    if (Q_UNLIKELY(!resource.hasMime())) {
        errorDescription = QT_TR_NOOP("Can't compose the html representation of the resource: no mime type is set");
        QNWARNING(errorDescription << ", resource: " << resource);
        return QString();
    }

    QXmlStreamAttributes attributes;
    attributes.append("hash", resource.dataHash());
    attributes.append("type", resource.mime());

    QString html;
    QXmlStreamWriter writer(&html);

    bool res = resourceInfoToHtml(attributes, writer, errorDescription);
    if (Q_UNLIKELY(!res)) {
        errorDescription = QT_TR_NOOP("Can't compose the html representation of the resource: ") + errorDescription;
        QNWARNING(errorDescription << ", resource: " << resource);
        return QString();
    }

    writer.writeEndElement();
    return html;
}

void ENMLConverterPrivate::escapeString(QString & string, const bool simplify)
{
    QNTRACE("String before escaping: " << string);
    string.replace("\'", "\\x27", Qt::CaseInsensitive);
    string.replace('"', "\\x22", Qt::CaseInsensitive);
    if (simplify) {
        string = string.simplified();
    }
    QNTRACE("String after escaping: " << string);
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

void ENMLConverterPrivate::toDoTagsToHtml(const QXmlStreamReader & reader,
                                          const quint64 enToDoIndex,
                                          QXmlStreamWriter & writer) const
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
    attributes.append("en-todo-id", QString::number(enToDoIndex));
    writer.writeAttributes(attributes);
}

bool ENMLConverterPrivate::encryptedTextToHtml(const QXmlStreamAttributes & enCryptAttributes,
                                               const QStringRef & encryptedTextCharacters,
                                               const quint64 enCryptIndex, const quint64 enDecryptedIndex,
                                               QXmlStreamWriter & writer,
                                               DecryptedTextManager & decryptedTextManager,
                                               bool & convertedToEnCryptNode) const
{
    QNDEBUG("ENMLConverterPrivate::encryptedTextToHtml: encrypted text = "
            << encryptedTextCharacters << ", en-crypt index = " << enCryptIndex
            << ", en-decrypted index = " << enDecryptedIndex);

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

        size_t keyLength = 0;
        if (!length.isEmpty())
        {
            bool conversionResult = false;
            keyLength = static_cast<size_t>(length.toUInt(&conversionResult));
            if (!conversionResult) {
                QNWARNING("Can't convert encryption key length from string to unsigned integer: " << length);
                keyLength = 0;
            }
        }

        decryptedTextHtml(decryptedText, encryptedTextCharacters.toString(), hint,
                          cipher, keyLength, enDecryptedIndex, writer);
        convertedToEnCryptNode = false;
        return true;
    }

    convertedToEnCryptNode = true;

#ifndef USE_QT_WEB_ENGINE
    writer.writeStartElement("object");
    writer.writeAttribute("type", "application/vnd.qutenote.encrypt");
#else
    writer.writeStartElement("img");
    writer.writeAttribute("src", QString());
#endif

    writer.writeAttribute("en-tag", "en-crypt");
    writer.writeAttribute("class", "en-crypt hvr-border-color");

    if (!hint.isEmpty()) {
        writer.writeAttribute("hint", hint);
    }

    if (!cipher.isEmpty()) {
        writer.writeAttribute("cipher", cipher);
    }

    if (!length.isEmpty()) {
        writer.writeAttribute("length", length);
    }

    writer.writeAttribute("encrypted_text", encryptedTextCharacters.toString());
    QNTRACE("Wrote element corresponding to en-crypt ENML tag");

    writer.writeAttribute("en-crypt-id", QString::number(enCryptIndex));

#ifndef USE_QT_WEB_ENGINE
    // Required for webkit, otherwise it can't seem to handle self-enclosing object tag properly
    writer.writeCharacters("some fake characters to prevent self-enclosing html tag confusing webkit");
#endif
    return true;
}

bool ENMLConverterPrivate::resourceInfoToHtml(const QXmlStreamAttributes & attributes,
                                              QXmlStreamWriter & writer, QString & errorDescription)
{
    QNDEBUG("ENMLConverterPrivate::resourceInfoToHtml");

    if (!attributes.hasAttribute("hash")) {
        errorDescription = QT_TR_NOOP("Detected incorrect en-media tag missing hash attribute");
        return false;
    }

    if (!attributes.hasAttribute("type")) {
        errorDescription = QT_TR_NOOP("Detected incorrect en-media tag missing type attribute");
        return false;
    }

    QStringRef mimeType = attributes.value("type");
    bool inlineImage = false;
    if (mimeType.startsWith("image", Qt::CaseInsensitive))
    {
        // TODO: consider some proper high-level interface for making it possible to customize ENML <--> HTML conversion
        /*
        if (pluginFactory)
        {
            QRegExp regex("^image\\/.+");
            bool res = pluginFactory->hasResourcePluginForMimeType(regex);
            if (!res) {
                inlineImage = true;
            }
        }
        else
        {
        */
        inlineImage = true;
    }

#ifndef USE_QT_WEB_ENGINE
    writer.writeStartElement(inlineImage ? "img" : "object");
#else
    writer.writeStartElement("img");
#endif

    // NOTE: ENMLConverterPrivate can't set src attribute for img tag as it doesn't know whether the resource is stored
    // in any local file yet. The user of noteContentToHtml should take care of those img tags and their src attributes

    writer.writeAttribute("en-tag", "en-media");

    if (inlineImage)
    {
        writer.writeAttributes(attributes);
        writer.writeAttribute("class", "en-media-image");
    }
    else
    {
        writer.writeAttribute("class", "en-media-generic hvr-border-color");

#ifndef USE_QT_WEB_ENGINE
        writer.writeAttribute("type", "application/vnd.qutenote.resource");

        const int numAttributes = attributes.size();
        for(int i = 0; i < numAttributes; ++i)
        {
            const QXmlStreamAttribute & attribute = attributes[i];
            const QString qualifiedName = attribute.qualifiedName().toString();
            if (qualifiedName == "en-tag") {
                continue;
            }

            const QString value = attribute.value().toString();

            if (qualifiedName == "type") {
                writer.writeAttribute("resource-mime-type", value);
            }
            else {
                writer.writeAttribute(qualifiedName, value);
            }
        }

        // Required for webkit, otherwise it can't seem to handle self-enclosing object tag properly
        writer.writeCharacters("some fake characters to prevent self-enclosing html tag confusing webkit");
#else
        writer.writeAttributes(attributes);
        writer.writeAttribute("src", "qrc:/generic_resource_icons/png/attachment.png");
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
    if (!attributes.hasAttribute("encrypted_text")) {
        errorDescription = QT_TR_NOOP("Missing encrypted text attribute within en-decrypted div tag");
        return false;
    }

    QString encryptedText = attributes.value("encrypted_text").toString();

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

    QNTRACE("Wrote en-crypt ENML tag from en-decrypted p tag");
    return true;
}

void ENMLConverterPrivate::decryptedTextHtml(const QString & decryptedText, const QString & encryptedText,
                                             const QString & hint, const QString & cipher,
                                             const size_t keyLength, const quint64 enDecryptedIndex,
                                             QXmlStreamWriter & writer)
{
    writer.writeStartElement("div");
    writer.writeAttribute("en-tag", "en-decrypted");
    writer.writeAttribute("encrypted_text", encryptedText);
    writer.writeAttribute("en-decrypted-id", QString::number(enDecryptedIndex));
    writer.writeAttribute("class", "en-decrypted hvr-border-color");

    if (!cipher.isEmpty()) {
        writer.writeAttribute("cipher", cipher);
    }

    if (keyLength != 0) {
        writer.writeAttribute("length", QString::number(keyLength));
    }

    if (!hint.isEmpty()) {
        writer.writeAttribute("hint", hint);
    }

    QString formattedDecryptedText = decryptedText;
    formattedDecryptedText.prepend("<?xml version=\"1.0\"?>"
                                   "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
                                   "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
                                   "<div id=\"decrypted_text_html_to_enml_temporary\">");
    formattedDecryptedText.append("</div>");

    QXmlStreamReader decryptedTextReader(formattedDecryptedText);
    bool foundFormattedText = false;

    while(!decryptedTextReader.atEnd())
    {
        Q_UNUSED(decryptedTextReader.readNext());

        if (decryptedTextReader.isStartElement())
        {
            const QXmlStreamAttributes attributes = decryptedTextReader.attributes();
            if (attributes.hasAttribute("id") && (attributes.value("id") == "decrypted_text_html_to_enml_temporary")) {
                QNTRACE("Skipping the start of temporarily added div");
                continue;
            }

            writer.writeStartElement(decryptedTextReader.name().toString());
            writer.writeAttributes(attributes);
            foundFormattedText = true;
            QNTRACE("Wrote start element from decrypted text: " << decryptedTextReader.name());
        }

        if (decryptedTextReader.isCharacters()) {
            writer.writeCharacters(decryptedTextReader.text().toString());
            foundFormattedText = true;
            QNTRACE("Wrote characters from decrypted text: " << decryptedTextReader.text());
        }

        if (decryptedTextReader.isEndElement())
        {
            const QXmlStreamAttributes attributes = decryptedTextReader.attributes();
            if (attributes.hasAttribute("id") && (attributes.value("id") == "decrypted_text_html_to_enml_temporary")) {
                QNTRACE("Skipping the end of temporarily added div");
                continue;
            }

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
}

ENMLConverterPrivate::ShouldSkipElementResult::type ENMLConverterPrivate::shouldSkipElement(const QString & elementName,
                                                                                            const QXmlStreamAttributes & attributes,
                                                                                            const QVector<SkipHtmlElementRule> & skipRules) const
{
    QNDEBUG("ENMLConverterPrivate::shouldSkipElement: element name = " << elementName
            << ", attributes = " << attributes);

    if (skipRules.isEmpty()) {
        return ShouldSkipElementResult::ShouldNotSkip;
    }

    ShouldSkipElementResult::Types flags;
    flags |= ShouldSkipElementResult::ShouldNotSkip;

#define CHECK_IF_SHOULD_SKIP() \
    if (shouldSkip) \
    { \
        if (rule.m_includeElementContents) { \
            flags |= ShouldSkipElementResult::SkipButPreserveContents; \
        } \
        else { \
            return ShouldSkipElementResult::SkipWithContents; \
        } \
    }

    const int numAttributes = attributes.size();

    const int numSkipRules = skipRules.size();
    for(int i = 0; i < numSkipRules; ++i)
    {
        const SkipHtmlElementRule & rule = skipRules[i];

        if (!rule.m_elementNameToSkip.isEmpty())
        {
            bool shouldSkip = false;

            switch(rule.m_elementNameComparisonRule)
            {
            case SkipHtmlElementRule::Equals:
            {
                if (rule.m_elementNameCaseSensitivity == Qt::CaseSensitive) {
                    shouldSkip = (elementName == rule.m_elementNameToSkip);
                }
                else {
                    shouldSkip = (elementName.toUpper() == rule.m_elementNameToSkip.toUpper());
                }
                break;
            }
            case SkipHtmlElementRule::StartsWith:
                shouldSkip = elementName.startsWith(rule.m_elementNameToSkip, rule.m_elementNameCaseSensitivity);
                break;
            case SkipHtmlElementRule::EndsWith:
                shouldSkip = elementName.endsWith(rule.m_elementNameToSkip, rule.m_elementNameCaseSensitivity);
                break;
            case SkipHtmlElementRule::Contains:
                shouldSkip = elementName.contains(rule.m_elementNameToSkip, rule.m_elementNameCaseSensitivity);
                break;
            default:
                QNWARNING("Detected unhandled SkipHtmlElementRule::ComparisonRule");
                break;
            }

            CHECK_IF_SHOULD_SKIP()
        }

        if (!rule.m_attributeNameToSkip.isEmpty())
        {
            for(int j = 0; j < numAttributes; ++j)
            {
                bool shouldSkip = false;

                const QXmlStreamAttribute & attribute = attributes[j];

                switch(rule.m_attributeNameComparisonRule)
                {
                case SkipHtmlElementRule::Equals:
                {
                    if (rule.m_attributeNameCaseSensitivity == Qt::CaseSensitive) {
                        shouldSkip = (attribute.name() == rule.m_attributeNameToSkip);
                    }
                    else {
                        shouldSkip = (attribute.name().toString().toUpper() == rule.m_attributeNameToSkip.toUpper());
                    }
                    break;
                }
                case SkipHtmlElementRule::StartsWith:
                    shouldSkip = attribute.name().startsWith(rule.m_attributeNameToSkip, rule.m_attributeNameCaseSensitivity);
                    break;
                case SkipHtmlElementRule::EndsWith:
                    shouldSkip = attribute.name().endsWith(rule.m_attributeNameToSkip, rule.m_attributeNameCaseSensitivity);
                    break;
                case SkipHtmlElementRule::Contains:
                    shouldSkip = attribute.name().contains(rule.m_attributeNameToSkip, rule.m_attributeNameCaseSensitivity);
                    break;
                default:
                    QNWARNING("Detected unhandled SkipHtmlElementRule::ComparisonRule");
                    break;
                }

                CHECK_IF_SHOULD_SKIP()
            }
        }

        if (!rule.m_attributeValueToSkip.isEmpty())
        {
            for(int j = 0; j < numAttributes; ++j)
            {
                bool shouldSkip = false;

                const QXmlStreamAttribute & attribute = attributes[j];

                switch(rule.m_attributeValueComparisonRule)
                {
                case SkipHtmlElementRule::Equals:
                {
                    if (rule.m_attributeValueCaseSensitivity == Qt::CaseSensitive) {
                        shouldSkip = (attribute.value() == rule.m_attributeValueToSkip);
                    }
                    else {
                        shouldSkip = (attribute.value().toString().toUpper() == rule.m_attributeValueToSkip.toUpper());
                    }
                    break;
                }
                case SkipHtmlElementRule::StartsWith:
                    shouldSkip = attribute.value().startsWith(rule.m_attributeValueToSkip, rule.m_attributeValueCaseSensitivity);
                    break;
                case SkipHtmlElementRule::EndsWith:
                    shouldSkip = attribute.value().endsWith(rule.m_attributeValueToSkip, rule.m_attributeValueCaseSensitivity);
                    break;
                case SkipHtmlElementRule::Contains:
                    shouldSkip = attribute.value().contains(rule.m_attributeValueToSkip, rule.m_attributeValueCaseSensitivity);
                    break;
                default:
                    QNWARNING("Detected unhandled SkipHtmlElementRule::ComparisonRule");
                    break;
                }

                CHECK_IF_SHOULD_SKIP()
            }
        }
    }

    if (flags & ShouldSkipElementResult::SkipButPreserveContents) {
        return ShouldSkipElementResult::SkipButPreserveContents;
    }

    return ShouldSkipElementResult::ShouldNotSkip;
}

} // namespace qute_note

QTextStream & operator<<(QTextStream & strm, const QXmlStreamAttributes & attributes)
{
    const int numAttributes = attributes.size();

    strm << "QXmlStreamAttributes(" << numAttributes << "): {\n";

    for(int i = 0; i < numAttributes; ++i) {
        const QXmlStreamAttribute & attribute = attributes[i];
        strm << "  [" << i << "]: name = " << attribute.name().toString()
             << ", value = " << attribute.value().toString() << "\n";
    }

    strm << "}\n";

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const QVector<qute_note::ENMLConverter::SkipHtmlElementRule> & rules)
{
    strm << "SkipHtmlElementRules";

    if (rules.isEmpty()) {
        strm << ": <empty>";
        return strm;
    }

    const int numRules = rules.size();

    strm << "(" << numRules << "): {\n";

    typedef qute_note::ENMLConverter::SkipHtmlElementRule SkipHtmlElementRule;

    for(int i = 0; i < numRules; ++i) {
        const SkipHtmlElementRule & rule = rules[i];
        strm << " [" << i << "]: " << rule << "\n";
    }

    strm << "}\n";

    return strm;
}
