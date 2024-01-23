/*
 * Copyright 2016-2024 Dmitry Ivanov
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

#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>

namespace quentier {

class BasicXMLSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit BasicXMLSyntaxHighlighter(QTextDocument * pTextDoc);

protected:
    virtual void highlightBlock(const QString & text);

private:
    void highlightByRegex(
        const QTextCharFormat & format, const QRegularExpression & regex,
        const QString & text);

    void setRegexes();
    void setFormats();

private:
    QTextCharFormat m_xmlKeywordFormat;
    QTextCharFormat m_xmlElementFormat;
    QTextCharFormat m_xmlAttributeFormat;
    QTextCharFormat m_xmlValueFormat;
    QTextCharFormat m_xmlCommentFormat;

    QList<QRegularExpression> m_xmlKeywordRegexes;
    QRegularExpression m_xmlElementRegex;
    QRegularExpression m_xmlAttributeRegex;
    QRegularExpression m_xmlValueRegex;
    QRegularExpression m_xmlCommentRegex;
};

} // namespace quentier
