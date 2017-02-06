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

#include "ENMLConverter_p.h"
#include <quentier/note_editor/DecryptedTextManager.h>
#include <quentier/enml/HTMLCleaner.h>
#include <quentier/types/Resource.h>
#include <quentier/logging/QuentierLogger.h>
#include <libxml/xmlreader.h>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QScopedPointer>
#include <QDomDocument>
#include <QRegExp>
#include <QFile>

namespace quentier {

#define WRAP(x) \
    << QStringLiteral(x)

ENMLConverterPrivate::ENMLConverterPrivate() :
    m_forbiddenXhtmlTags(QSet<QString>()
#include "forbiddenXhtmlTags.inl"
    ),
    m_forbiddenXhtmlAttributes(QSet<QString>()
#include "forbiddenXhtmlAttributes.inl"
    ),
    m_evernoteSpecificXhtmlTags(QSet<QString>()
#include "evernoteSpecificXhtmlTags.inl"
    ),
    m_allowedXhtmlTags(QSet<QString>()
#include "allowedXhtmlTags.inl"
    ),
    m_allowedEnMediaAttributes(QSet<QString>()
#include "allowedEnMediaAttributes.inl"
    ),
    m_pHtmlCleaner(Q_NULLPTR),
    m_cachedConvertedXml()
{}

#undef WRAP

ENMLConverterPrivate::~ENMLConverterPrivate()
{
    delete m_pHtmlCleaner;
}

bool ENMLConverterPrivate::htmlToNoteContent(const QString & html, const QVector<SkipHtmlElementRule> & skipRules,
                                             QString & noteContent, DecryptedTextManager & decryptedTextManager,
                                             ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("ENMLConverterPrivate::htmlToNoteContent: ") << html << QStringLiteral("\nskip element rules: ") << skipRules);

    if (!m_pHtmlCleaner) {
        m_pHtmlCleaner = new HTMLCleaner;
    }

    QString error;
    m_cachedConvertedXml.resize(0);
    bool res = m_pHtmlCleaner->htmlToXml(html, m_cachedConvertedXml, error);
    if (!res) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Failed to clean up the note's html");
        errorDescription.details() = error;
        return false;
    }

    QNTRACE(QStringLiteral("HTML converted to XML by tidy: ") << m_cachedConvertedXml);

    QXmlStreamReader reader(m_cachedConvertedXml);

    noteContent.resize(0);
    QXmlStreamWriter writer(&noteContent);
    writer.setAutoFormatting(true);
    writer.setCodec("UTF-8");
    writer.writeStartDocument();
    writer.writeDTD(QStringLiteral("<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">"));

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
                QNTRACE(QStringLiteral("Skipping everyting inside element skipped together with its contents by the rules"));
                ++skippedElementNestingCounter;
                continue;
            }

            lastElementName = reader.name().toString();
            if (lastElementName == QStringLiteral("form")) {
                QNTRACE(QStringLiteral("Skipping <form> tag"));
                continue;
            }
            else if (lastElementName == QStringLiteral("html")) {
                QNTRACE("Skipping <html> tag");
                continue;
            }
            else if (lastElementName == QStringLiteral("title")) {
                QNTRACE(QStringLiteral("Skipping <title> tag"));
                continue;
            }
            else if (lastElementName == QStringLiteral("body")) {
                lastElementName = QStringLiteral("en-note");
                QNTRACE(QStringLiteral("Found \"body\" HTML tag, will replace it with \"en-note\" tag for written ENML"));
            }

            auto tagIt = m_forbiddenXhtmlTags.find(lastElementName);
            if ((tagIt != m_forbiddenXhtmlTags.end()) && (lastElementName != QStringLiteral("object"))) {
                QNTRACE(QStringLiteral("Skipping forbidden XHTML tag: ") << lastElementName);
                continue;
            }

            tagIt = m_allowedXhtmlTags.find(lastElementName);
            if (tagIt == m_allowedXhtmlTags.end())
            {
                tagIt = m_evernoteSpecificXhtmlTags.find(lastElementName);
                if (tagIt == m_evernoteSpecificXhtmlTags.end()) {
                    QNTRACE(QStringLiteral("Haven't found tag ") << lastElementName
                            << QStringLiteral(" within the list of allowed XHTML tags or within Evernote-specific tags, skipping it"));
                    continue;
                }
            }

            lastElementAttributes = reader.attributes();

            ShouldSkipElementResult::type shouldSkip = shouldSkipElement(lastElementName, lastElementAttributes, skipRules);
            if (shouldSkip != ShouldSkipElementResult::ShouldNotSkip)
            {
                QNTRACE(QStringLiteral("Skipping element ") << lastElementName << QStringLiteral(" per skip rules; the contents would be ")
                        << (shouldSkip == ShouldSkipElementResult::SkipWithContents ? QStringLiteral("skipped") : QStringLiteral("preserved")));

                if (shouldSkip == ShouldSkipElementResult::SkipWithContents) {
                    ++skippedElementNestingCounter;
                }
                else if (shouldSkip == ShouldSkipElementResult::SkipButPreserveContents) {
                    ++skippedElementWithPreservedContentsNestingCounter;
                }

                continue;
            }

            if ( ((lastElementName == QStringLiteral("img")) || (lastElementName == QStringLiteral("object")) || (lastElementName == QStringLiteral("div"))) &&
                 lastElementAttributes.hasAttribute(QStringLiteral("en-tag")) )
            {
                const QString enTag = lastElementAttributes.value(QStringLiteral("en-tag")).toString();
                if (enTag == QStringLiteral("en-decrypted"))
                {
                    QNTRACE(QStringLiteral("Found decrypted text area, need to convert it back to en-crypt form"));
                    bool res = decryptedTextToEnml(reader, decryptedTextManager, writer, errorDescription);
                    if (!res) {
                        return false;
                    }

                    continue;
                }
                else if (enTag == QStringLiteral("en-todo"))
                {
                    if (!lastElementAttributes.hasAttribute(QStringLiteral("src"))) {
                        QNWARNING(QStringLiteral("Found en-todo tag without src attribute"));
                        continue;
                    }

                    QStringRef srcValue = lastElementAttributes.value(QStringLiteral("src"));
                    if (srcValue.contains(QStringLiteral("qrc:/checkbox_icons/checkbox_no.png"))) {
                        writer.writeStartElement(QStringLiteral("en-todo"));
                        ++writeElementCounter;
                        continue;
                    }
                    else if (srcValue.contains(QStringLiteral("qrc:/checkbox_icons/checkbox_yes.png"))) {
                        writer.writeStartElement(QStringLiteral("en-todo"));
                        writer.writeAttribute(QStringLiteral("checked"), QStringLiteral("true"));
                        ++writeElementCounter;
                        continue;
                    }
                }
                else if (enTag == QStringLiteral("en-crypt"))
                {
                    const QXmlStreamAttributes attributes = reader.attributes();
                    QXmlStreamAttributes enCryptAttributes;

                    if (attributes.hasAttribute(QStringLiteral("cipher"))) {
                        enCryptAttributes.append(QStringLiteral("cipher"), attributes.value(QStringLiteral("cipher")).toString());
                    }

                    if (attributes.hasAttribute(QStringLiteral("length"))) {
                        enCryptAttributes.append(QStringLiteral("length"), attributes.value(QStringLiteral("length")).toString());
                    }

                    if (!attributes.hasAttribute(QStringLiteral("encrypted_text"))) {
                        errorDescription.base() = QT_TRANSLATE_NOOP("", "Found en-crypt tag without encrypted_text attribute");
                        QNDEBUG(errorDescription);
                        return false;
                    }

                    if (attributes.hasAttribute(QStringLiteral("hint"))) {
                        enCryptAttributes.append(QStringLiteral("hint"), attributes.value(QStringLiteral("hint")).toString());
                    }

                    writer.writeStartElement(QStringLiteral("en-crypt"));
                    writer.writeAttributes(enCryptAttributes);
                    writer.writeCharacters(attributes.value(QStringLiteral("encrypted_text")).toString());
                    ++writeElementCounter;
                    QNTRACE(QStringLiteral("Started writing en-crypt tag"));
                    insideEnCryptElement = true;
                    continue;
                }
                else if (enTag == QStringLiteral("en-media"))
                {
                    bool isImage = (lastElementName == QStringLiteral("img"));
                    lastElementName = QStringLiteral("en-media");
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
                            if (attributeQualifiedName == QStringLiteral("resource-mime-type")) {
                                enMediaAttributes.append(QStringLiteral("type"), attributeValue);
                            }
                            else if (m_allowedEnMediaAttributes.contains(attributeQualifiedName) && (attributeQualifiedName != QStringLiteral("type"))) {
                                enMediaAttributes.append(attributeQualifiedName, attributeValue);
                            }
                        }
                        else if (m_allowedEnMediaAttributes.contains(attributeQualifiedName)) { // img
                            enMediaAttributes.append(attributeQualifiedName, attributeValue);
                        }
                    }

                    writer.writeAttributes(enMediaAttributes);
                    enMediaAttributes.clear();
                    QNTRACE(QStringLiteral("Wrote en-media element from img element in HTML"));

                    continue;
                }
            }

            // Erasing the forbidden attributes
            for(QXmlStreamAttributes::iterator it = lastElementAttributes.begin(); it != lastElementAttributes.end(); )
            {
                const QStringRef attributeName = it->name();
                if (isForbiddenXhtmlAttribute(attributeName.toString())) {
                    QNTRACE(QStringLiteral("Erasing the forbidden attribute ") << attributeName);
                    it = lastElementAttributes.erase(it);
                    continue;
                }

                if ((lastElementName == QStringLiteral("a")) && (attributeName == QStringLiteral("en-hyperlink-id"))) {
                    QNTRACE(QStringLiteral("Erasing custom attribute en-hyperlink-id"));
                    it = lastElementAttributes.erase(it);
                    continue;
                }

                ++it;
            }

            writer.writeStartElement(lastElementName);
            writer.writeAttributes(lastElementAttributes);
            ++writeElementCounter;
            QNTRACE(QStringLiteral("Wrote element: name = ") << lastElementName << QStringLiteral(" and its attributes"));
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
                QNTRACE(QStringLiteral("Wrote CDATA: ") << text);
            }
            else {
                writer.writeCharacters(text);
                QNTRACE(QStringLiteral("Wrote characters: ") << text);
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
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't convert the note's html to ENML");
        errorDescription.details() = reader.errorString();
        QNWARNING(QStringLiteral("Error reading html: ") << errorDescription
                  << QStringLiteral(", HTML: ") << html << QStringLiteral("\nXML: ") << m_cachedConvertedXml);
        return false;
    }

    QNTRACE(QStringLiteral("Converted ENML: ") << noteContent);

    ErrorString validationError;
    res = validateEnml(noteContent, validationError);
    if (!res) {
        errorDescription = validationError;
        QNWARNING(errorDescription << QStringLiteral(", ENML: ") << noteContent << QStringLiteral("\nHTML: ") << html);
        return false;
    }

    return true;
}

bool ENMLConverterPrivate::noteContentToHtml(const QString & noteContent, QString & html,
                                             ErrorString & errorDescription,
                                             DecryptedTextManager & decryptedTextManager,
                                             NoteContentToHtmlExtraData & extraData) const
{
    QNDEBUG(QStringLiteral("ENMLConverterPrivate::noteContentToHtml: ") << noteContent);

    extraData.m_numEnToDoNodes = 0;
    extraData.m_numHyperlinkNodes = 0;
    extraData.m_numEnCryptNodes = 0;
    extraData.m_numEnDecryptedNodes = 0;

    html.resize(0);
    errorDescription.clear();

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

            if (lastElementName == QStringLiteral("en-note"))
            {
                QNTRACE(QStringLiteral("Replacing en-note with \"body\" tag"));
                lastElementName = QStringLiteral("body");
            }
            else if (lastElementName == QStringLiteral("en-media"))
            {
                bool res = resourceInfoToHtml(lastElementAttributes, writer, errorDescription);
                if (!res) {
                    return false;
                }

                continue;
            }
            else if (lastElementName == QStringLiteral("en-crypt"))
            {
                insideEnCryptTag = true;
                continue;
            }
            else if (lastElementName == QStringLiteral("en-todo"))
            {
                quint64 enToDoIndex = extraData.m_numEnToDoNodes + 1;
                toDoTagsToHtml(reader, enToDoIndex, writer);
                ++extraData.m_numEnToDoNodes;
                continue;
            }
            else if (lastElementName == QStringLiteral("a"))
            {
                quint64 hyperlinkIndex = extraData.m_numHyperlinkNodes + 1;
                lastElementAttributes.append(QStringLiteral("en-hyperlink-id"), QString::number(hyperlinkIndex));
                ++extraData.m_numHyperlinkNodes;
            }

            // NOTE: do not attempt to process en-todo tags here, it would be done below

            writer.writeStartElement(lastElementName);
            writer.writeAttributes(lastElementAttributes);

            QNTRACE(QStringLiteral("Wrote start element: ") << lastElementName << QStringLiteral(" and its attributes"));
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
                QNTRACE(QStringLiteral("Wrote CDATA: ") << reader.text().toString());
            }
            else {
                writer.writeCharacters(reader.text().toString());
                QNTRACE(QStringLiteral("Wrote characters: ") << reader.text().toString());
            }
        }

        if ((writeElementCounter > 0) && reader.isEndElement()) {
            writer.writeEndElement();
            --writeElementCounter;
        }
    }

    if (reader.hasError()) {
        QNWARNING(QStringLiteral("Error reading ENML: ") << reader.errorString());
        return false;
    }

    return true;
}

bool ENMLConverterPrivate::validateEnml(const QString & enml, ErrorString & errorDescription) const
{
    errorDescription.clear();

    QByteArray inputBuffer = enml.toLocal8Bit();
    xmlDocPtr pDoc = xmlParseMemory(inputBuffer.constData(), inputBuffer.size());
    if (!pDoc) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't validate ENML: can't parse enml into xml doc");
        QNWARNING(errorDescription << QStringLiteral(": enml = ") << enml);
        return false;
    }

    QFile dtdFile(QStringLiteral(":/enml2.dtd"));
    if (!dtdFile.open(QIODevice::ReadOnly)) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't validate ENML: can't open the resource file with DTD");
        QNWARNING(errorDescription << QStringLiteral(": enml = ") << enml);
        xmlFreeDoc(pDoc);
        return false;
    }

    QByteArray dtdRawData = dtdFile.readAll();

    xmlParserInputBufferPtr pBuf = xmlParserInputBufferCreateMem(dtdRawData.constData(), dtdRawData.size(),
                                                                 XML_CHAR_ENCODING_NONE);
    if (!pBuf) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't validate ENML: can't allocate the input buffer for dtd validation");
        QNWARNING(errorDescription);
        xmlFreeDoc(pDoc);
        return false;
    }

    xmlDtdPtr pDtd = xmlIOParseDTD(NULL, pBuf, XML_CHAR_ENCODING_NONE);
    if (!pDtd) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't validate ENML: failed to parse DTD");
        QNWARNING(errorDescription);
        xmlFreeParserInputBuffer(pBuf);
        xmlFreeDoc(pDoc);
        return false;
    }

    xmlParserCtxtPtr pContext = xmlNewParserCtxt();
    if (!pContext) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't validate ENML: can't allocate parser context");
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
                                                  ErrorString & errorMessage)
{
    QNDEBUG(QStringLiteral("ENMLConverterPrivate::noteContentToPlainText: ") << noteContent);

    plainText.resize(0);

    QXmlStreamReader reader(noteContent);

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
            if ((element == QStringLiteral("en-media")) || (element == QStringLiteral("en-crypt"))) {
                skipIteration = true;
            }

            continue;
        }

        if (reader.isEndElement())
        {
            const QStringRef element = reader.name();
            if ((element == QStringLiteral("en-media")) || (element == QStringLiteral("en-crypt"))) {
                skipIteration = false;
            }

            continue;
        }

        if (reader.isCharacters() && !skipIteration) {
            plainText += reader.text();
        }
    }

    if (Q_UNLIKELY(reader.hasError())) {
        errorMessage.base() = QT_TRANSLATE_NOOP("", "Failed to convert the note content to plain text");
        errorMessage.details() = reader.errorString();
        errorMessage.details() += QStringLiteral(", error code ");
        errorMessage.details() += QString::number(reader.error());
        QNWARNING(errorMessage);
        return false;
    }

    return true;
}

bool ENMLConverterPrivate::noteContentToListOfWords(const QString & noteContent,
                                                    QStringList & listOfWords,
                                                    ErrorString & errorMessage, QString * plainText)
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
    return plainText.split(QRegExp(QStringLiteral("\\W+")), QString::SkipEmptyParts);
}

QString ENMLConverterPrivate::toDoCheckboxHtml(const bool checked, const quint64 idNumber)
{
    QString html = QStringLiteral("<img src=\"qrc:/checkbox_icons/checkbox_");
    if (checked) {
        html += QStringLiteral("yes.png\" class=\"checkbox_checked\" ");
    }
    else {
        html += QStringLiteral("no.png\" class=\"checkbox_unchecked\" ");
    }

    html += QStringLiteral("en-tag=\"en-todo\" en-todo-id=\"");
    html += QString::number(idNumber);
    html += QStringLiteral("\" />");
    return html;
}

QString ENMLConverterPrivate::encryptedTextHtml(const QString & encryptedText, const QString & hint,
                                                const QString & cipher, const size_t keyLength,
                                                const quint64 enCryptIndex)
{
    QString encryptedTextHtmlObject;

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    encryptedTextHtmlObject = QStringLiteral("<img ");
#else
    encryptedTextHtmlObject = QStringLiteral("<object type=\"application/vnd.quentier.encrypt\" ");
#endif
    encryptedTextHtmlObject += QStringLiteral("en-tag=\"en-crypt\" cipher=\"");
    encryptedTextHtmlObject += cipher;
    encryptedTextHtmlObject += QStringLiteral("\" length=\"");
    encryptedTextHtmlObject += QString::number(keyLength);
    encryptedTextHtmlObject += QStringLiteral("\" class=\"en-crypt hvr-border-color\" encrypted_text=\"");
    encryptedTextHtmlObject += encryptedText;
    encryptedTextHtmlObject += QStringLiteral("\" en-crypt-id=\"");
    encryptedTextHtmlObject += QString::number(enCryptIndex);
    encryptedTextHtmlObject += QStringLiteral("\" ");

    if (!hint.isEmpty())
    {
        encryptedTextHtmlObject += QStringLiteral("hint=\"");

        QString hintWithEscapedDoubleQuotes = hint;
        escapeString(hintWithEscapedDoubleQuotes, /* simplify = */ true);

        encryptedTextHtmlObject += hintWithEscapedDoubleQuotes;
        encryptedTextHtmlObject += QStringLiteral("\" ");
    }

#ifdef QUENTIER_USE_QT_WEB_ENGINE
    encryptedTextHtmlObject += QStringLiteral(" />");
#else
    encryptedTextHtmlObject += QStringLiteral(">some fake characters to prevent self-enclosing html tag confusing webkit</object>");
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

QString ENMLConverterPrivate::resourceHtml(const Resource & resource, ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("ENMLConverterPrivate::resourceHtml"));

    if (Q_UNLIKELY(!resource.hasDataHash())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't compose the resource's html representation: no data hash is set");
        QNWARNING(errorDescription << QStringLiteral(", resource: ") << resource);
        return QString();
    }

    if (Q_UNLIKELY(!resource.hasMime())) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't compose the resource's html representation: no mime type is set");
        QNWARNING(errorDescription << QStringLiteral(", resource: ") << resource);
        return QString();
    }

    QXmlStreamAttributes attributes;
    attributes.append(QStringLiteral("hash"), resource.dataHash().toHex());
    attributes.append(QStringLiteral("type"), resource.mime());

    QString html;
    QXmlStreamWriter writer(&html);

    bool res = resourceInfoToHtml(attributes, writer, errorDescription);
    if (Q_UNLIKELY(!res)) {
        QNWARNING(errorDescription << QStringLiteral(", resource: ") << resource);
        return QString();
    }

    writer.writeEndElement();
    return html;
}

void ENMLConverterPrivate::escapeString(QString & string, const bool simplify)
{
    QNTRACE(QStringLiteral("String before escaping: ") << string);
    string.replace(QStringLiteral("\'"), QStringLiteral("\\x27"), Qt::CaseInsensitive);
    string.replace(QStringLiteral("\""), QStringLiteral("\\x22"), Qt::CaseInsensitive);
    if (simplify) {
        string = string.simplified();
    }
    QNTRACE(QStringLiteral("String after escaping: ") << string);
}

bool ENMLConverterPrivate::isForbiddenXhtmlTag(const QString & tagName) const
{
    auto it = m_forbiddenXhtmlTags.find(tagName);
    if (it == m_forbiddenXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::isForbiddenXhtmlAttribute(const QString & attributeName) const
{
    auto it = m_forbiddenXhtmlAttributes.find(attributeName);
    if (it == m_forbiddenXhtmlAttributes.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::isEvernoteSpecificXhtmlTag(const QString & tagName) const
{
    auto it = m_evernoteSpecificXhtmlTags.find(tagName);
    if (it == m_evernoteSpecificXhtmlTags.constEnd()) {
        return false;
    }
    else {
        return true;
    }
}

bool ENMLConverterPrivate::isAllowedXhtmlTag(const QString & tagName) const
{
    auto it = m_allowedXhtmlTags.find(tagName);
    if (it == m_allowedXhtmlTags.constEnd()) {
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
    QNDEBUG(QStringLiteral("ENMLConverterPrivate::toDoTagsToHtml"));

    QXmlStreamAttributes originalAttributes = reader.attributes();
    bool checked = false;
    if (originalAttributes.hasAttribute(QStringLiteral("checked"))) {
        QStringRef checkedStr = originalAttributes.value(QStringLiteral("checked"));
        if (checkedStr == QStringLiteral("true")) {
            checked = true;
        }
    }

    QNTRACE(QStringLiteral("Converting ") << (checked ? QStringLiteral("completed") : QStringLiteral("not yet completed"))
            << QStringLiteral(" ToDo item"));

    writer.writeStartElement(QStringLiteral("img"));

    QXmlStreamAttributes attributes;
    attributes.append(QStringLiteral("src"), QStringLiteral("qrc:/checkbox_icons/checkbox_") + (checked ? QStringLiteral("yes") : QStringLiteral("no")) + QStringLiteral(".png"));
    attributes.append(QStringLiteral("class"), QStringLiteral("checkbox_") + (checked ? QStringLiteral("checked") : QStringLiteral("unchecked")));
    attributes.append(QStringLiteral("en-tag"), QStringLiteral("en-todo"));
    attributes.append(QStringLiteral("en-todo-id"), QString::number(enToDoIndex));
    writer.writeAttributes(attributes);
}

bool ENMLConverterPrivate::encryptedTextToHtml(const QXmlStreamAttributes & enCryptAttributes,
                                               const QStringRef & encryptedTextCharacters,
                                               const quint64 enCryptIndex, const quint64 enDecryptedIndex,
                                               QXmlStreamWriter & writer,
                                               DecryptedTextManager & decryptedTextManager,
                                               bool & convertedToEnCryptNode) const
{
    QNDEBUG(QStringLiteral("ENMLConverterPrivate::encryptedTextToHtml: encrypted text = ")
            << encryptedTextCharacters << QStringLiteral(", en-crypt index = ") << enCryptIndex
            << QStringLiteral(", en-decrypted index = ") << enDecryptedIndex);

    QString cipher;
    if (enCryptAttributes.hasAttribute(QStringLiteral("cipher"))) {
        cipher = enCryptAttributes.value(QStringLiteral("cipher")).toString();
    }

    QString length;
    if (enCryptAttributes.hasAttribute(QStringLiteral("length"))) {
        length = enCryptAttributes.value(QStringLiteral("length")).toString();
    }

    QString hint;
    if (enCryptAttributes.hasAttribute(QStringLiteral("hint"))) {
        hint = enCryptAttributes.value(QStringLiteral("hint")).toString();
    }

    QString decryptedText;
    bool rememberForSession = false;
    bool foundDecryptedText = decryptedTextManager.findDecryptedTextByEncryptedText(encryptedTextCharacters.toString(),
                                                                                    decryptedText, rememberForSession);
    if (foundDecryptedText)
    {
        QNTRACE(QStringLiteral("Found encrypted text which has already been decrypted and cached; encrypted text = ")
                << encryptedTextCharacters);

        size_t keyLength = 0;
        if (!length.isEmpty())
        {
            bool conversionResult = false;
            keyLength = static_cast<size_t>(length.toUInt(&conversionResult));
            if (!conversionResult) {
                QNWARNING(QStringLiteral("Can't convert encryption key length from string to unsigned integer: ") << length);
                keyLength = 0;
            }
        }

        decryptedTextHtml(decryptedText, encryptedTextCharacters.toString(), hint,
                          cipher, keyLength, enDecryptedIndex, writer);
        convertedToEnCryptNode = false;
        return true;
    }

    convertedToEnCryptNode = true;

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    writer.writeStartElement(QStringLiteral("object"));
    writer.writeAttribute(QStringLiteral("type"), QStringLiteral("application/vnd.quentier.encrypt"));
#else
    writer.writeStartElement(QStringLiteral("img"));
    writer.writeAttribute(QStringLiteral("src"), QString());
#endif

    writer.writeAttribute(QStringLiteral("en-tag"), QStringLiteral("en-crypt"));
    writer.writeAttribute(QStringLiteral("class"), QStringLiteral("en-crypt hvr-border-color"));

    if (!hint.isEmpty()) {
        writer.writeAttribute(QStringLiteral("hint"), hint);
    }

    if (!cipher.isEmpty()) {
        writer.writeAttribute(QStringLiteral("cipher"), cipher);
    }

    if (!length.isEmpty()) {
        writer.writeAttribute(QStringLiteral("length"), length);
    }

    writer.writeAttribute(QStringLiteral("encrypted_text"), encryptedTextCharacters.toString());
    QNTRACE(QStringLiteral("Wrote element corresponding to en-crypt ENML tag"));

    writer.writeAttribute(QStringLiteral("en-crypt-id"), QString::number(enCryptIndex));

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    // Required for webkit, otherwise it can't seem to handle self-enclosing object tag properly
    writer.writeCharacters(QStringLiteral("some fake characters to prevent self-enclosing html tag confusing webkit"));
#endif
    return true;
}

bool ENMLConverterPrivate::resourceInfoToHtml(const QXmlStreamAttributes & attributes,
                                              QXmlStreamWriter & writer, ErrorString & errorDescription)
{
    QNDEBUG(QStringLiteral("ENMLConverterPrivate::resourceInfoToHtml"));

    if (!attributes.hasAttribute(QStringLiteral("hash"))) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Detected incorrect en-media tag missing hash attribute");
        QNDEBUG(errorDescription);
        return false;
    }

    if (!attributes.hasAttribute(QStringLiteral("type"))) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Detected incorrect en-media tag missing type attribute");
        QNDEBUG(errorDescription);
        return false;
    }

    QStringRef mimeType = attributes.value(QStringLiteral("type"));
    bool inlineImage = false;
    if (mimeType.startsWith(QStringLiteral("image"), Qt::CaseInsensitive))
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

#ifndef QUENTIER_USE_QT_WEB_ENGINE
    writer.writeStartElement(inlineImage ? QStringLiteral("img") : QStringLiteral("object"));
#else
    writer.writeStartElement(QStringLiteral("img"));
#endif

    // NOTE: ENMLConverterPrivate can't set src attribute for img tag as it doesn't know whether the resource is stored
    // in any local file yet. The user of noteContentToHtml should take care of those img tags and their src attributes

    writer.writeAttribute(QStringLiteral("en-tag"), QStringLiteral("en-media"));

    if (inlineImage)
    {
        writer.writeAttributes(attributes);
        writer.writeAttribute(QStringLiteral("class"), QStringLiteral("en-media-image"));
    }
    else
    {
        writer.writeAttribute(QStringLiteral("class"), QStringLiteral("en-media-generic hvr-border-color"));

#ifndef QUENTIER_USE_QT_WEB_ENGINE
        writer.writeAttribute(QStringLiteral("type"), QStringLiteral("application/vnd.quentier.resource"));

        const int numAttributes = attributes.size();
        for(int i = 0; i < numAttributes; ++i)
        {
            const QXmlStreamAttribute & attribute = attributes[i];
            const QString qualifiedName = attribute.qualifiedName().toString();
            if (qualifiedName == QStringLiteral("en-tag")) {
                continue;
            }

            const QString value = attribute.value().toString();

            if (qualifiedName == QStringLiteral("type")) {
                writer.writeAttribute(QStringLiteral("resource-mime-type"), value);
            }
            else {
                writer.writeAttribute(qualifiedName, value);
            }
        }

        // Required for webkit, otherwise it can't seem to handle self-enclosing object tag properly
        writer.writeCharacters(QStringLiteral("some fake characters to prevent self-enclosing html tag confusing webkit"));
#else
        writer.writeAttributes(attributes);
        writer.writeAttribute(QStringLiteral("src"), QStringLiteral("qrc:/generic_resource_icons/png/attachment.png"));
#endif
    }

    return true;
}

bool ENMLConverterPrivate::decryptedTextToEnml(QXmlStreamReader & reader,
                                               DecryptedTextManager & decryptedTextManager,
                                               QXmlStreamWriter & writer, ErrorString & errorDescription) const
{
    QNDEBUG(QStringLiteral("ENMLConverterPrivate::decryptedTextToEnml"));

    const QXmlStreamAttributes attributes = reader.attributes();
    if (!attributes.hasAttribute(QStringLiteral("encrypted_text"))) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Missing encrypted text attribute within en-decrypted div tag");
        QNDEBUG(errorDescription);
        return false;
    }

    QString encryptedText = attributes.value(QStringLiteral("encrypted_text")).toString();

    QString storedDecryptedText;
    bool rememberForSession = false;
    bool res = decryptedTextManager.findDecryptedTextByEncryptedText(encryptedText, storedDecryptedText, rememberForSession);
    if (!res) {
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Can't find the decrypted text by its encrypted text");
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
        errorDescription.base() = QT_TRANSLATE_NOOP("", "Text decryption failed");
        errorDescription.details() = reader.errorString();
        QNWARNING(QStringLiteral("Couldn't read the nested contents of en-decrypted div, reader has error: ")
                  << errorDescription);
        return false;
    }

    if (storedDecryptedText != actualDecryptedText)
    {
        QNTRACE(QStringLiteral("Found modified decrypted text, need to re-encrypt"));

        QString actualEncryptedText;
        res = decryptedTextManager.modifyDecryptedText(encryptedText, actualDecryptedText, actualEncryptedText);
        if (res) {
            QNTRACE(QStringLiteral("Re-evaluated the modified decrypted text's encrypted text; was: ")
                    << encryptedText << QStringLiteral("; new: ") << actualEncryptedText);
            encryptedText = actualEncryptedText;
        }
    }

    QString hint;
    if (attributes.hasAttribute(QStringLiteral("hint"))) {
        hint = attributes.value(QStringLiteral("hint")).toString();
    }

    writer.writeStartElement(QStringLiteral("en-crypt"));

    if (attributes.hasAttribute(QStringLiteral("cipher"))) {
        writer.writeAttribute(QStringLiteral("cipher"), attributes.value(QStringLiteral("cipher")).toString());
    }

    if (attributes.hasAttribute(QStringLiteral("length"))) {
        writer.writeAttribute(QStringLiteral("length"), attributes.value(QStringLiteral("length")).toString());
    }

    if (!hint.isEmpty()) {
        writer.writeAttribute(QStringLiteral("hint"), hint);
    }

    writer.writeCharacters(encryptedText);
    writer.writeEndElement();

    QNTRACE(QStringLiteral("Wrote en-crypt ENML tag from en-decrypted p tag"));
    return true;
}

void ENMLConverterPrivate::decryptedTextHtml(const QString & decryptedText, const QString & encryptedText,
                                             const QString & hint, const QString & cipher,
                                             const size_t keyLength, const quint64 enDecryptedIndex,
                                             QXmlStreamWriter & writer)
{
    writer.writeStartElement(QStringLiteral("div"));
    writer.writeAttribute(QStringLiteral("en-tag"), QStringLiteral("en-decrypted"));
    writer.writeAttribute(QStringLiteral("encrypted_text"), encryptedText);
    writer.writeAttribute(QStringLiteral("en-decrypted-id"), QString::number(enDecryptedIndex));
    writer.writeAttribute(QStringLiteral("class"), QStringLiteral("en-decrypted hvr-border-color"));

    if (!cipher.isEmpty()) {
        writer.writeAttribute(QStringLiteral("cipher"), cipher);
    }

    if (keyLength != 0) {
        writer.writeAttribute(QStringLiteral("length"), QString::number(keyLength));
    }

    if (!hint.isEmpty()) {
        writer.writeAttribute(QStringLiteral("hint"), hint);
    }

    QString formattedDecryptedText = decryptedText;
    formattedDecryptedText.prepend(QStringLiteral("<?xml version=\"1.0\"?>"
                                                  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
                                                  "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
                                                  "<div id=\"decrypted_text_html_to_enml_temporary\">"));
    formattedDecryptedText.append(QStringLiteral("</div>"));

    QXmlStreamReader decryptedTextReader(formattedDecryptedText);
    bool foundFormattedText = false;

    while(!decryptedTextReader.atEnd())
    {
        Q_UNUSED(decryptedTextReader.readNext());

        if (decryptedTextReader.isStartElement())
        {
            const QXmlStreamAttributes attributes = decryptedTextReader.attributes();
            if (attributes.hasAttribute(QStringLiteral("id")) && (attributes.value(QStringLiteral("id")) == QStringLiteral("decrypted_text_html_to_enml_temporary"))) {
                QNTRACE(QStringLiteral("Skipping the start of temporarily added div"));
                continue;
            }

            writer.writeStartElement(decryptedTextReader.name().toString());
            writer.writeAttributes(attributes);
            foundFormattedText = true;
            QNTRACE(QStringLiteral("Wrote start element from decrypted text: ") << decryptedTextReader.name());
        }

        if (decryptedTextReader.isCharacters()) {
            writer.writeCharacters(decryptedTextReader.text().toString());
            foundFormattedText = true;
            QNTRACE(QStringLiteral("Wrote characters from decrypted text: ") << decryptedTextReader.text());
        }

        if (decryptedTextReader.isEndElement())
        {
            const QXmlStreamAttributes attributes = decryptedTextReader.attributes();
            if (attributes.hasAttribute(QStringLiteral("id")) && (attributes.value(QStringLiteral("id")) == QStringLiteral("decrypted_text_html_to_enml_temporary"))) {
                QNTRACE(QStringLiteral("Skipping the end of temporarily added div"));
                continue;
            }

            writer.writeEndElement();
            QNTRACE(QStringLiteral("Wrote end element from decrypted text: ") << decryptedTextReader.name());
        }
    }

    if (decryptedTextReader.hasError()) {
        QNWARNING(QStringLiteral("Decrypted text reader has error: ") << decryptedTextReader.errorString());
    }

    if (!foundFormattedText) {
        writer.writeCharacters(decryptedText);
        QNTRACE(QStringLiteral("Wrote unformatted decrypted text: ") << decryptedText);
    }
}

ENMLConverterPrivate::ShouldSkipElementResult::type ENMLConverterPrivate::shouldSkipElement(const QString & elementName,
                                                                                            const QXmlStreamAttributes & attributes,
                                                                                            const QVector<SkipHtmlElementRule> & skipRules) const
{
    QNDEBUG(QStringLiteral("ENMLConverterPrivate::shouldSkipElement: element name = ") << elementName
            << QStringLiteral(", attributes = ") << attributes);

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
                QNWARNING(QStringLiteral("Detected unhandled SkipHtmlElementRule::ComparisonRule"));
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
                    QNWARNING(QStringLiteral("Detected unhandled SkipHtmlElementRule::ComparisonRule"));
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
                    QNWARNING(QStringLiteral("Detected unhandled SkipHtmlElementRule::ComparisonRule"));
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

} // namespace quentier

QTextStream & operator<<(QTextStream & strm, const QXmlStreamAttributes & attributes)
{
    const int numAttributes = attributes.size();

    strm << QStringLiteral("QXmlStreamAttributes(") << numAttributes << QStringLiteral("): {\n");

    for(int i = 0; i < numAttributes; ++i) {
        const QXmlStreamAttribute & attribute = attributes[i];
        strm << QStringLiteral("  [") << i << QStringLiteral("]: name = ") << attribute.name().toString()
             << QStringLiteral(", value = ") << attribute.value().toString() << QStringLiteral("\n");
    }

    strm << QStringLiteral("}\n");

    return strm;
}

QTextStream & operator<<(QTextStream & strm, const QVector<quentier::ENMLConverter::SkipHtmlElementRule> & rules)
{
    strm << QStringLiteral("SkipHtmlElementRules");

    if (rules.isEmpty()) {
        strm << QStringLiteral(": <empty>");
        return strm;
    }

    const int numRules = rules.size();

    strm << QStringLiteral("(") << numRules << QStringLiteral("): {\n");

    typedef quentier::ENMLConverter::SkipHtmlElementRule SkipHtmlElementRule;

    for(int i = 0; i < numRules; ++i) {
        const SkipHtmlElementRule & rule = rules[i];
        strm << QStringLiteral(" [") << i << QStringLiteral("]: ") << rule << QStringLiteral("\n");
    }

    strm << QStringLiteral("}\n");

    return strm;
}
