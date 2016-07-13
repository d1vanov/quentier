#include <quentier/enml/ENMLConverter.h>
#include "ENMLConverter_p.h"
#include <quentier/types/IResource.h>
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
                                      QNLocalizedString & errorDescription,
                                      const QVector<SkipHtmlElementRule> & skipRules) const
{
    QNDEBUG("ENMLConverter::htmlToNoteContent");

    Q_D(const ENMLConverter);
    return d->htmlToNoteContent(html, skipRules, noteContent, decryptedTextManager, errorDescription);
}

bool ENMLConverter::noteContentToHtml(const QString & noteContent, QString & html,
                                      QNLocalizedString & errorDescription,
                                      DecryptedTextManager & decryptedTextManager,
                                      NoteContentToHtmlExtraData & extraData) const
{
    QNDEBUG("ENMLConverter::noteContentToHtml");

    Q_D(const ENMLConverter);
    return d->noteContentToHtml(noteContent, html, errorDescription, decryptedTextManager, extraData);
}

bool ENMLConverter::validateEnml(const QString & enml, QNLocalizedString & errorDescription) const
{
    Q_D(const ENMLConverter);
    return d->validateEnml(enml, errorDescription);
}

bool ENMLConverter::noteContentToPlainText(const QString & noteContent, QString & plainText,
                                           QNLocalizedString & errorMessage)
{
    return ENMLConverterPrivate::noteContentToPlainText(noteContent, plainText, errorMessage);
}

bool ENMLConverter::noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                             QNLocalizedString & errorMessage, QString * plainText)
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

QString ENMLConverter::resourceHtml(const IResource & resource, QNLocalizedString & errorDescription)
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
            strm << "Equals"; \
            break; \
        case StartsWith: \
            strm << "Starts with"; \
            break; \
        case EndsWith: \
            strm << "Ends with"; \
            break; \
        case Contains: \
            strm << "Contains"; \
            break; \
        default: \
            strm << "unknown"; \
            break; \
    }

    strm << "SkipHtmlElementRule: {\n";
    strm << "  element name to skip = " << m_elementNameToSkip
         << ", rule: ";
    PRINT_COMPARISON_RULE(m_elementNameComparisonRule)
    strm << ", case " << (m_elementNameCaseSensitivity == Qt::CaseSensitive
                          ? "sensitive"
                          : "insensitive");
    strm << "\n";

    strm << "  attribute name to skip = " << m_attributeNameToSkip
         << ", rule: ";
    PRINT_COMPARISON_RULE(m_attributeNameComparisonRule)
    strm << ", case " << (m_attributeNameCaseSensitivity == Qt::CaseSensitive
                          ? "sensitive"
                          : "insensitive");
    strm << "\n";

    strm << "  attribute value to skip = " << m_attributeValueToSkip
         << ", rule: ";
    PRINT_COMPARISON_RULE(m_attributeValueComparisonRule)
    strm << ", case " << (m_attributeValueCaseSensitivity == Qt::CaseSensitive
                          ? "sensitive"
                          : "insensitive");
    strm << "\n}\n";

    return strm;
}

} // namespace quentier

