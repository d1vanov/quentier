#include "ENMLConverter_p.h"
#include "HTMLCleaner.h"
#include <client/types/Note.h>
#include <logging/QuteNoteLogger.h>
#include <libxml/xmlreader.h>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QScopedPointer>
#include <QDomDocument>

namespace qute_note {

static const QSet<QString> forbiddenXhtmlTags = QSet<QString>() << "applet"
                                                                << "base"
                                                                << "basefont"
                                                                << "bgsound"
                                                                << "body"
                                                                << "button"
                                                                << "dir"
                                                                << "embed"
                                                                << "fieldset"
                                                                << "form"
                                                                << "frame"
                                                                << "frameset"
                                                                << "head"
                                                                << "html"
                                                                << "iframe"
                                                                << "ilayer"
                                                                << "input"
                                                                << "isindex"
                                                                << "label"
                                                                << "layer"
                                                                << "legend"
                                                                << "link"
                                                                << "marquee"
                                                                << "menu"
                                                                << "meta"
                                                                << "noframes"
                                                                << "noscript"
                                                                << "object"
                                                                << "optgroup"
                                                                << "option"
                                                                << "param"
                                                                << "plaintext"
                                                                << "script"
                                                                << "select"
                                                                << "style"
                                                                << "textarea"
                                                                << "xml";

static const QSet<QString> forbiddenXhtmlAttributes = QSet<QString>() << "id"
                                                                      << "class"
                                                                      << "onclick"
                                                                      << "ondblclick"
                                                                      << "accesskey"
                                                                      << "data"
                                                                      << "dynsrc"
                                                                      << "tableindex";

static const QSet<QString> evernoteSpecificXhtmlTags = QSet<QString>() << "en-note"
                                                                       << "en-media"
                                                                       << "en-crypt"
                                                                       << "en-todo";

static const QSet<QString> allowedXhtmlTags = QSet<QString>() << "a"
                                                              << "abbr"
                                                              << "acronym"
                                                              << "address"
                                                              << "area"
                                                              << "b"
                                                              << "bdo"
                                                              << "big"
                                                              << "blockquote"
                                                              << "br"
                                                              << "caption"
                                                              << "center"
                                                              << "cite"
                                                              << "code"
                                                              << "col"
                                                              << "colgroup"
                                                              << "dd"
                                                              << "del"
                                                              << "dfn"
                                                              << "div"
                                                              << "dl"
                                                              << "dt"
                                                              << "em"
                                                              << "font"
                                                              << "h1"
                                                              << "h2"
                                                              << "h3"
                                                              << "h4"
                                                              << "h5"
                                                              << "h6"
                                                              << "hr"
                                                              << "i"
                                                              << "img"
                                                              << "ins"
                                                              << "kbd"
                                                              << "li"
                                                              << "map"
                                                              << "ol"
                                                              << "p"
                                                              << "pre"
                                                              << "q"
                                                              << "s"
                                                              << "samp"
                                                              << "small"
                                                              << "span"
                                                              << "strike"
                                                              << "strong"
                                                              << "sub"
                                                              << "sup"
                                                              << "table"
                                                              << "tbody"
                                                              << "td"
                                                              << "tfoot"
                                                              << "th"
                                                              << "thead"
                                                              << "title"
                                                              << "tr"
                                                              << "tt"
                                                              << "u"
                                                              << "ul"
                                                              << "var"
                                                              << "xmp";

ENMLConverterPrivate::ENMLConverterPrivate() :
    m_pHtmlCleaner(nullptr)
{}

ENMLConverterPrivate::~ENMLConverterPrivate()
{
    delete m_pHtmlCleaner;
}

bool ENMLConverterPrivate::htmlToNoteContent(const QString & html, Note & note, QString & errorDescription) const
{
    if (!m_pHtmlCleaner) {
        m_pHtmlCleaner = new HTMLCleaner;
    }

    QString convertedXml;
    bool res = m_pHtmlCleaner->htmlToXml(html, convertedXml, errorDescription);
    if (!res) {
        errorDescription.prepend(QT_TR_NOOP("Could not clean up note's html: "));
        return false;
    }

    QXmlStreamReader reader(convertedXml);

    QString noteContent;
    QXmlStreamWriter writer(&noteContent);
    writer.writeStartDocument();
    writer.writeDTD("<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">");

    while(!reader.atEnd())
    {
        Q_UNUSED(reader.readNext());

        if (reader.isStartDocument()) {
            continue;
        }

        if (reader.isEndDocument()) {
            writer.writeEndDocument();
            break;
        }

        if (reader.isStartElement())
        {
            QString name = reader.name().toString();
            if (name == "form") {
                QNTRACE("Skipping <form> tag");
                continue;
            }
            else if (name == "html") {
                QNTRACE("Skipping <html> tag");
                continue;
            }
            else if (name == "body") {
                name = "en-note";
                QNTRACE("Found \"body\" HTML tag, will replace it with \"en-note\" tag for written ENML");
            }

            const QString namespaceUri = reader.namespaceUri().toString();

            auto tagIt = forbiddenXhtmlTags.find(name);
            if (tagIt != forbiddenXhtmlTags.end()) {
                QNTRACE("Skipping forbidden XHTML tag: " << name);
                continue;
            }

            tagIt = allowedXhtmlTags.find(name);
            if (tagIt == allowedXhtmlTags.end()) {
                QNTRACE("Haven't found tag " << name << " within the list of allowed XHTML tags, skipping it");
                continue;
            }

            QXmlStreamAttributes attributes = reader.attributes();

            if (name == "img")
            {
                if (attributes.hasAttribute(namespaceUri, "src"))
                {
                    const QString srcValue = attributes.value(namespaceUri, "src").toString();
                    if (srcValue == "qrc:/checkbox_icons/checkbox_no.png") {
                        // TODO: write non-checked checkbox
                        continue;
                    }
                    else if (srcValue == "qrc:/checkbox_icons/checkbox_yes.png") {
                        // TODO: write checked checkbox
                        continue;
                    }
                    else {
                        // TODO: write image resource info
                    }
                }
                else
                {
                    errorDescription = QT_TR_NOOP("Can't convert note to ENML: found img html tag without src attribute");
                    QNWARNING(errorDescription << ", html = " << html << ", cleaned up xml = " << convertedXml);
                    return false;
                }
            }
            // TODO: check for other "reserved" tag names for resources, links, etc

            // Erasing the forbidden attributes
            for(QXmlStreamAttributes::iterator it = attributes.begin(); it != attributes.end(); )
            {
                const QStringRef attributeName = it->name();
                if (isForbiddenXhtmlAttribute(attributeName.toString())) {
                    QNTRACE("Erasing the forbidden attribute " << attributeName);
                    it = attributes.erase(it);
                    continue;
                }

                ++it;
            }

            writer.writeStartElement(namespaceUri, name);
            writer.writeAttributes(attributes);
            QNTRACE("Wrote element: namespaceUri = " << namespaceUri << ", name = " << name << " and its attributes");
        }

        if (reader.isEndElement()) {
            writer.writeEndElement();
        }
    }

    res = validateEnml(noteContent, errorDescription);
    if (!res)
    {
        if (!errorDescription.isEmpty()) {
            errorDescription = QT_TR_NOOP("Can't validate ENML with DTD: ") + errorDescription;
        }
        else {
            errorDescription = QT_TR_NOOP("Failed to convert, produced ENML is invalid according to dtd");
        }

        QNWARNING(errorDescription << ": " << noteContent << "\n\nSource html: " << html
                  << "\n\nCleaned up & converted xml: " << convertedXml);
        return false;
    }

    note.setContent(noteContent);
    return true;
}

bool ENMLConverterPrivate::noteContentToHtml(const Note & note, QString & html, QString & errorDescription) const
{
    // TODO: implement
    Q_UNUSED(note)
    Q_UNUSED(html)
    Q_UNUSED(errorDescription)
    return true;
}

bool ENMLConverterPrivate::validateEnml(const QString & enml, QString & errorDescription) const
{
    errorDescription.clear();

#if QT_VERSION >= 0x050101
    QScopedPointer<unsigned char, QScopedPointerArrayDeleter<unsigned char> > str(reinterpret_cast<unsigned char*>(qstrdup(enml.toUtf8().constData())));
#else
    QScopedPointer<unsigned char, QScopedPointerArrayDeleter<unsigned char> > str(reinterpret_cast<unsigned char*>(qstrdup(enml.toAscii().constData())));
#endif

    xmlDocPtr pDoc = xmlParseDoc(static_cast<unsigned char*>(str.data()));
    if (!pDoc) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't parse enml to xml doc");
        QNWARNING(errorDescription);
        return false;
    }

    QFile dtdFile(":/enml2.dtd");
    if (!dtdFile.open(QIODevice::ReadOnly)) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't open resource file with DTD");
        QNWARNING(errorDescription);
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
        xmlFreeDoc(pDoc);
        xmlFreeParserInputBuffer(pBuf);
        return false;
    }

    xmlParserCtxtPtr pContext = xmlNewParserCtxt();
    if (!pContext) {
        errorDescription = QT_TR_NOOP("Can't validate ENML: can't allocate parses context");
        QNWARNING(errorDescription);
        xmlFreeDoc(pDoc);
        xmlFreeParserInputBuffer(pBuf);
        xmlFreeDtd(pDtd);
    }

    bool res = static_cast<bool>(xmlValidateDtd(&pContext->vctxt, pDoc, pDtd));

    xmlFreeDoc(pDoc);
    xmlFreeParserInputBuffer(pBuf);
    xmlFreeDtd(pDtd);
    xmlFreeParserCtxt(pContext);

    return res;
}

bool ENMLConverterPrivate::noteContentToPlainText(const QString & noteContent, QString & plainText,
                                                  QString & errorMessage)
{
    // FIXME: remake using QXmlStreamReader

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

} // namespace qute_note

