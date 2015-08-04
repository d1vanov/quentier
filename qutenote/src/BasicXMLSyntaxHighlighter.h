#ifndef __QUTE_NOTE__BASIC_XML_SYNTAX_HIGHLIGHTER_H
#define __QUTE_NOTE__BASIC_XML_SYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>

class BasicXMLSyntaxHighlighter: public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit BasicXMLSyntaxHighlighter(QTextDocument * pTextDoc);

protected:
    virtual void highlightBlock(const QString & text);

private:
    void highlightByRegex(const QTextCharFormat & format,
                          const QRegExp & regex, const QString & text);

private:
    QTextCharFormat     m_xmlKeywordFormat;
    QTextCharFormat     m_xmlElementFormat;
    QTextCharFormat     m_xmlAttributeFormat;
    QTextCharFormat     m_xmlValueFormat;
    QTextCharFormat     m_xmlCommentFormat;

    QList<QRegExp>      m_xmlKeywordRegexes;
    QRegExp             m_xmlElementRegex;
    QRegExp             m_xmlAttributeRegex;
    QRegExp             m_xmlValueRegex;
    QRegExp             m_xmlCommentRegex;
};

#endif // __QUTE_NOTE__BASIC_XML_SYNTAX_HIGHLIGHTER_H
