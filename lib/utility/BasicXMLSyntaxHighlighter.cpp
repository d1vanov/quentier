/*
 * Copyright 2016-20244 Dmitry Ivanov
 *
 * This file is part of Quentier.
 *
 * Quentier is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Quentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Quentier. If not, see <http://www.gnu.org/licenses/>.
 */

#include "BasicXMLSyntaxHighlighter.h"

namespace quentier {

BasicXMLSyntaxHighlighter::BasicXMLSyntaxHighlighter(QTextDocument * parent) :
    QSyntaxHighlighter{parent}
{
    setRegexes();
    setFormats();
}

void BasicXMLSyntaxHighlighter::highlightBlock(const QString & text)
{
    // Special treatment for xml element regex as we use captured text to
    // emulate lookbehind

    int offset = 0;
    auto match = m_xmlElementRegex.match(text, offset);
    while (match.hasMatch()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        auto captured = match.capturedRef();
#else
        auto captured = match.capturedView();
#endif
        auto matchedPos = text.indexOf(captured, offset);
        auto matchedLength = captured.length();
        setFormat(matchedPos, matchedLength, m_xmlElementFormat);

        offset = matchedPos + matchedLength;
        match = m_xmlElementRegex.match(text, offset);
    }

    // Highlight xml keywords *after* xml elements to fix any occasional /
    // captured into the enclosing element
    for (auto it = m_xmlKeywordRegexes.begin(), end = m_xmlKeywordRegexes.end();
         it != end; ++it)
    {
        const auto & regex = *it;
        highlightByRegex(m_xmlKeywordFormat, regex, text);
    }

    highlightByRegex(m_xmlAttributeFormat, m_xmlAttributeRegex, text);
    highlightByRegex(m_xmlCommentFormat, m_xmlCommentRegex, text);
    highlightByRegex(m_xmlValueFormat, m_xmlValueRegex, text);
}

void BasicXMLSyntaxHighlighter::highlightByRegex(
    const QTextCharFormat & format, const QRegularExpression & regex,
    const QString & text)
{
    Q_UNUSED(format)

    int offset = 0;
    auto match = regex.match(text, offset);
    while (match.hasMatch()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        auto captured = match.capturedRef();
#else
        auto captured = match.capturedView();
#endif
        auto matchedPos = text.indexOf(captured, offset);
        auto matchedLength = captured.length();
        setFormat(matchedPos, matchedLength, m_xmlElementFormat);

        offset = matchedPos + matchedLength;
        match = regex.match(text, offset);
    }
}

void BasicXMLSyntaxHighlighter::setRegexes()
{
    m_xmlElementRegex.setPattern(
        QStringLiteral("<[\\s]*[/]?[\\s]*([^\\n]\\w*)(?=[\\s/>])"));

    m_xmlAttributeRegex.setPattern(QStringLiteral("\\w+(?=\\=)"));
    m_xmlValueRegex.setPattern(QStringLiteral("\"[^\\n\"]+\"(?=[\\s/>])"));
    m_xmlCommentRegex.setPattern(QStringLiteral("<!--[^\\n]*-->"));

    m_xmlKeywordRegexes = QList<QRegularExpression>()
        << QRegularExpression(QStringLiteral("<\\?"))
        << QRegularExpression(QStringLiteral("/>"))
        << QRegularExpression(QStringLiteral(">"))
        << QRegularExpression(QStringLiteral("<"))
        << QRegularExpression(QStringLiteral("</"))
        << QRegularExpression(QStringLiteral("\\?>"));
}

void BasicXMLSyntaxHighlighter::setFormats()
{
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

} // namespace quentier
