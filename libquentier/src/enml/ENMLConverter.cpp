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

#include <quentier/enml/ENMLConverter.h>
#include "ENMLConverter_p.h"
#include <quentier/types/Resource.h>
#include <quentier/logging/QuentierLogger.h>

namespace quentier {

ENMLConverter::ENMLConverter() :
    d_ptr(new ENMLConverterPrivate)
{}

ENMLConverter::~ENMLConverter()
{
    delete d_ptr;
}

bool ENMLConverter::htmlToNoteContent(const QString & html, QString & noteContent,
                                      DecryptedTextManager & decryptedTextManager,
                                      ErrorString & errorDescription,
                                      const QVector<SkipHtmlElementRule> & skipRules) const
{
    QNDEBUG(QStringLiteral("ENMLConverter::htmlToNoteContent"));

    Q_D(const ENMLConverter);
    return d->htmlToNoteContent(html, skipRules, noteContent, decryptedTextManager, errorDescription);
}

bool ENMLConverter::noteContentToHtml(const QString & noteContent, QString & html,
                                      ErrorString & errorDescription,
                                      DecryptedTextManager & decryptedTextManager,
                                      NoteContentToHtmlExtraData & extraData) const
{
    QNDEBUG(QStringLiteral("ENMLConverter::noteContentToHtml"));

    Q_D(const ENMLConverter);
    return d->noteContentToHtml(noteContent, html, errorDescription, decryptedTextManager, extraData);
}

bool ENMLConverter::validateEnml(const QString & enml, ErrorString & errorDescription) const
{
    Q_D(const ENMLConverter);
    return d->validateEnml(enml, errorDescription);
}

bool ENMLConverter::noteContentToPlainText(const QString & noteContent, QString & plainText,
                                           ErrorString & errorMessage)
{
    return ENMLConverterPrivate::noteContentToPlainText(noteContent, plainText, errorMessage);
}

bool ENMLConverter::noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                             ErrorString & errorMessage, QString * plainText)
{
    return ENMLConverterPrivate::noteContentToListOfWords(noteContent, listOfWords, errorMessage, plainText);
}

QStringList ENMLConverter::plainTextToListOfWords(const QString & plainText)
{
    return ENMLConverterPrivate::plainTextToListOfWords(plainText);
}

QString ENMLConverter::toDoCheckboxHtml(const bool checked, const quint64 idNumber)
{
    return ENMLConverterPrivate::toDoCheckboxHtml(checked, idNumber);
}

QString ENMLConverter::encryptedTextHtml(const QString & encryptedText, const QString & hint,
                                         const QString & cipher, const size_t keyLength,
                                         const quint64 enCryptIndex)
{
    return ENMLConverterPrivate::encryptedTextHtml(encryptedText, hint, cipher, keyLength, enCryptIndex);
}

QString ENMLConverter::decryptedTextHtml(const QString & decryptedText, const QString & encryptedText,
                                         const QString & hint, const QString & cipher,
                                         const size_t keyLength, const quint64 enDecryptedIndex)
{
    return ENMLConverterPrivate::decryptedTextHtml(decryptedText, encryptedText,
                                                   hint, cipher, keyLength, enDecryptedIndex);
}

QString ENMLConverter::resourceHtml(const Resource & resource, ErrorString & errorDescription)
{
    return ENMLConverterPrivate::resourceHtml(resource, errorDescription);
}

void ENMLConverter::escapeString(QString & string, const bool simplify)
{
    ENMLConverterPrivate::escapeString(string, simplify);
}

QTextStream & ENMLConverter::SkipHtmlElementRule::print(QTextStream & strm) const
{
#define PRINT_COMPARISON_RULE(rule) \
    switch(rule) \
    { \
        case Equals: \
            strm << QStringLiteral("Equals"); \
            break; \
        case StartsWith: \
            strm << QStringLiteral("Starts with"); \
            break; \
        case EndsWith: \
            strm << QStringLiteral("Ends with"); \
            break; \
        case Contains: \
            strm << QStringLiteral("Contains"); \
            break; \
        default: \
            strm << QStringLiteral("unknown"); \
            break; \
    }

    strm << QStringLiteral("SkipHtmlElementRule: {\n");
    strm << QStringLiteral("  element name to skip = ") << m_elementNameToSkip
         << QStringLiteral(", rule: ");
    PRINT_COMPARISON_RULE(m_elementNameComparisonRule)
    strm << QStringLiteral(", case ") << ((m_elementNameCaseSensitivity == Qt::CaseSensitive)
                                          ? QStringLiteral("sensitive")
                                          : QStringLiteral("insensitive"));
    strm << QStringLiteral("\n");

    strm << QStringLiteral("  attribute name to skip = ") << m_attributeNameToSkip
         << QStringLiteral(", rule: ");
    PRINT_COMPARISON_RULE(m_attributeNameComparisonRule)
    strm << QStringLiteral(", case ") << ((m_attributeNameCaseSensitivity == Qt::CaseSensitive)
                                          ? QStringLiteral("sensitive")
                                          : QStringLiteral("insensitive"));
    strm << QStringLiteral("\n");

    strm << QStringLiteral("  attribute value to skip = ") << m_attributeValueToSkip
         << QStringLiteral(", rule: ");
    PRINT_COMPARISON_RULE(m_attributeValueComparisonRule)
    strm << QStringLiteral(", case ") << ((m_attributeValueCaseSensitivity == Qt::CaseSensitive)
                                          ? QStringLiteral("sensitive")
                                          : QStringLiteral("insensitive"));
    strm << "\n}\n";

    return strm;
}

} // namespace quentier

