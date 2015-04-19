#include "BasicXMLSyntaxHighlighter.h"

BasicXMLSyntaxHighlighter::BasicXMLSyntaxHighlighter(QTextDocument * pTextDoc) :
    QSyntaxHighlighter(pTextDoc),
    m_xmlKeywordFormat(),
    m_xmlElementFormat(),
    m_xmlAttributeFormat(),
    m_xmlValueFormat(),
    m_xmlCommentFormat(),
    m_xmlKeywordRegexes(),
    m_xmlElementRegex("\\w+(?=[\\s/>])"),
    m_xmlAttributeRegex("\\w+(?=\\=)"),
    m_xmlValueRegex("\"[^\\n\"]+\"(?=[\\s/>])"),
    m_xmlCommentRegex("<!--[\\N]*-->")
{
    m_xmlKeywordRegexes << QRegExp("\\b?xml\\b")
                        << QRegExp("/>")
                        << QRegExp(">")
                        << QRegExp("<")
                        << QRegExp("</");

    m_xmlKeywordFormat.setForeground(Qt::blue);
    m_xmlKeywordFormat.setFontWeight(QFont::Bold);

    m_xmlElementFormat.setForeground(Qt::darkMagenta);
    m_xmlElementFormat.setFontWeight(QFont::Bold);

    m_xmlAttributeFormat.setForeground(Qt::darkGreen);
    m_xmlAttributeFormat.setFontWeight(QFont::Bold);
    m_xmlAttributeFormat.setFontItalic(true);

    m_xmlValueFormat.setForeground(Qt::darkRed);

    m_xmlCommentFormat.setForeground(Qt::gray);
}

void BasicXMLSyntaxHighlighter::highlightBlock(const QString & text)
{
    typedef QList<QRegExp>::const_iterator Iter;
    Iter xmlKeywordRegexesEnd = m_xmlKeywordRegexes.end();
    for(Iter it = m_xmlKeywordRegexes.begin(); it != xmlKeywordRegexesEnd; ++it) {
        const QRegExp & regex = *it;
        highlightByRegex(m_xmlKeywordFormat, regex, text);
    }

    highlightByRegex(m_xmlElementFormat, m_xmlElementRegex, text);
    highlightByRegex(m_xmlAttributeFormat, m_xmlAttributeRegex, text);
    highlightByRegex(m_xmlCommentFormat, m_xmlCommentRegex, text);
    highlightByRegex(m_xmlValueFormat, m_xmlValueRegex, text);
}

void BasicXMLSyntaxHighlighter::highlightByRegex(const QTextCharFormat & format,
                                                 const QRegExp & regex, const QString & text)
{
    int index = regex.indexIn(text);

    while(index >= 0)
    {
        int matchedLength = regex.matchedLength();
        setFormat(index, matchedLength, format);

        index = regex.indexIn(text, index + matchedLength);
    }
}
