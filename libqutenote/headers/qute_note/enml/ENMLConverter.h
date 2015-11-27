#ifndef __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_H
#define __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_H

#include <qute_note/utility/Printable.h>
#include <qute_note/utility/Linkage.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QSet>
#include <QString>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)
QT_FORWARD_DECLARE_CLASS(ENMLConverterPrivate)

/**
 * @brief The ENMLConverter class encapsulates a set of methods
 * and helper data structures for performing the conversions between ENML
 * and other note content formats, namely HTML
 */
class QUTE_NOTE_EXPORT ENMLConverter
{
public:
    ENMLConverter();
    virtual ~ENMLConverter();

    /**
     * @brief The SkipHtmlElementRule class describes the set of rules
     * for HTML -> ENML conversion about the HTML elements that should not
     * be actually converted to ENML due to their nature of being "helper"
     * elements for the display or functioning of something within the web page.
     * The HTML -> ENML conversion would ignore tags and attributes forbidden by ENML
     * even without these rules.
     *
     * The elements nested into those skipped would also be skipped whether or not
     * the child elements fulfill the specified rules
     */
    class SkipHtmlElementRule: public Printable
    {
    public:
        enum ComparisonRule {
            Equals = 0,
            StartsWith,
            EndsWith
        };

        SkipHtmlElementRule() :
            m_elementNameToSkip(),
            m_elementNameComparisonRule(Equals),
            m_elementNameCaseSensitivity(Qt::CaseSensitive),
            m_attributeNameToSkip(),
            m_attributeNameComparisonRule(Equals),
            m_attributeNameCaseSensitivity(Qt::CaseSensitive),
            m_attributeValueToSkip(),
            m_attributeValueComparisonRule(Equals),
            m_attributeValueCaseSensitivity(Qt::CaseSensitive)
        {}

        virtual QTextStream & Print(QTextStream & strm) const Q_DECL_OVERRIDE;

        QString             m_elementNameToSkip;
        ComparisonRule      m_elementNameComparisonRule;
        Qt::CaseSensitivity m_elementNameCaseSensitivity;

        QString             m_attributeNameToSkip;
        ComparisonRule      m_attributeNameComparisonRule;
        Qt::CaseSensitivity m_attributeNameCaseSensitivity;

        QString             m_attributeValueToSkip;
        ComparisonRule      m_attributeValueComparisonRule;
        Qt::CaseSensitivity m_attributeValueCaseSensitivity;
    };

    bool htmlToNoteContent(const QString & html, QString & noteContent,
                           DecryptedTextManager & decryptedTextManager,
                           QString & errorDescription,
                           const QVector<SkipHtmlElementRule> & skipRules = QVector<SkipHtmlElementRule>()) const;

    struct NoteContentToHtmlExtraData
    {
        quint64     m_numEnToDoNodes;
        quint64     m_numHyperlinkNodes;
        quint64     m_numEnCryptNodes;
        quint64     m_numEnDecryptedNodes;
    };

    bool noteContentToHtml(const QString & noteContent, QString & html, QString & errorDescription,
                           DecryptedTextManager & decryptedTextManager, NoteContentToHtmlExtraData & extraData) const;

    bool validateEnml(const QString & enml, QString & errorDescription) const;

    static bool noteContentToPlainText(const QString & noteContent, QString & plainText,
                                       QString & errorMessage);

    static bool noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                         QString & errorMessage, QString * plainText = Q_NULLPTR);

    static QStringList plainTextToListOfWords(const QString & plainText);

    static QString toDoCheckboxHtml(const bool checked, const quint64 idNumber);

    static QString encryptedTextHtml(const QString & encryptedText, const QString & hint,
                                     const QString & cipher, const size_t keyLength,
                                     const quint64 enCryptIndex);

    static QString decryptedTextHtml(const QString & decryptedText, const QString & encryptedText,
                                     const QString & hint, const QString & cipher,
                                     const size_t keyLength, const quint64 enDecryptedIndex);

    static void escapeString(QString & string);

private:
    Q_DISABLE_COPY(ENMLConverter)

private:
    ENMLConverterPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ENMLConverter)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_H
