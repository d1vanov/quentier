/*
 * Copyright 2016-2021 Dmitry Ivanov
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

#ifndef QUENTIER_LIB_UTILITY_BASIC_XML_SYNTAX_HIGHLIGHTER_H
#define QUENTIER_LIB_UTILITY_BASIC_XML_SYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>

namespace quentier {

class BasicXMLSyntaxHighlighter final : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit BasicXMLSyntaxHighlighter(QTextDocument * pTextDoc);
    ~BasicXMLSyntaxHighlighter() override;

protected:
    void highlightBlock(const QString & text) override;

private:
    void highlightByRegex(
        const QTextCharFormat & format, const QRegExp & regex,
        const QString & text);

    void setRegexes();
    void setFormats();

private:
    QTextCharFormat m_xmlKeywordFormat;
    QTextCharFormat m_xmlElementFormat;
    QTextCharFormat m_xmlAttributeFormat;
    QTextCharFormat m_xmlValueFormat;
    QTextCharFormat m_xmlCommentFormat;

    QList<QRegExp> m_xmlKeywordRegexes;
    QRegExp m_xmlElementRegex;
    QRegExp m_xmlAttributeRegex;
    QRegExp m_xmlValueRegex;
    QRegExp m_xmlCommentRegex;
};

} // namespace quentier

#endif // QUENTIER_LIB_UTILITY_BASIC_XML_SYNTAX_HIGHLIGHTER_H
