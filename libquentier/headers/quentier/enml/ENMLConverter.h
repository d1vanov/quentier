#ifndef LIB_QUENTIER_ENML_ENML_CONVERTER_H
#define LIB_QUENTIER_ENML_ENML_CONVERTER_H

#include <quentier/utility/Printable.h>
#include <quentier/utility/Linkage.h>
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QSet>
#include <QString>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)
QT_FORWARD_DECLARE_CLASS(ENMLConverterPrivate)

/**
 * @brief The ENMLConverter class encapsulates a set of methods
 * and helper data structures for performing the conversions between ENML
 * and other note content formats, namely HTML
 */
class QUENTIER_EXPORT ENMLConverter
{
public:
    ENMLConverter();
    virtual ~ENMLConverter();

    /**
     * @brief The SkipHtmlElementRule class describes the set of rules
     * for HTML -> ENML conversion about the HTML elements that should not
     * be actually converted to ENML due to their nature of being "helper"
     * elements for the display or functioning of something within the note editor's page.
     * The HTML -> ENML conversion would ignore tags and attributes forbidden by ENML
     * even without these rules conditionally preserving or skipping the contents and nested elements
     * of skipped elements
     */
    class QUENTIER_EXPORT SkipHtmlElementRule: public Printable
    {
    public:
        enum ComparisonRule {
            Equals = 0,
            StartsWith,
            EndsWith,
            Contains
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
            m_attributeValueCaseSensitivity(Qt::CaseSensitive),
            m_includeElementContents(false)
        {}

        virtual QTextStream & print(QTextStream & strm) const Q_DECL_OVERRIDE;

        QString             m_elementNameToSkip;
        ComparisonRule      m_elementNameComparisonRule;
        Qt::CaseSensitivity m_elementNameCaseSensitivity;

        QString             m_attributeNameToSkip;
        ComparisonRule      m_attributeNameComparisonRule;
        Qt::CaseSensitivity m_attributeNameCaseSensitivity;

        QString             m_attributeValueToSkip;
        ComparisonRule      m_attributeValueComparisonRule;
        Qt::CaseSensitivity m_attributeValueCaseSensitivity;

        bool                m_includeElementContents;
    };

    bool htmlToNoteContent(const QString & html, QString & noteContent,
                           DecryptedTextManager & decryptedTextManager,
                           QNLocalizedString & errorDescription,
                           const QVector<SkipHtmlElementRule> & skipRules = QVector<SkipHtmlElementRule>()) const;

    struct NoteContentToHtmlExtraData
    {
        quint64     m_numEnToDoNodes;
        quint64     m_numHyperlinkNodes;
        quint64     m_numEnCryptNodes;
        quint64     m_numEnDecryptedNodes;
    };

    bool noteContentToHtml(const QString & noteContent, QString & html, QNLocalizedString & errorDescription,
                           DecryptedTextManager & decryptedTextManager, NoteContentToHtmlExtraData & extraData) const;

    bool validateEnml(const QString & enml, QNLocalizedString & errorDescription) const;

    static bool noteContentToPlainText(const QString & noteContent, QString & plainText,
                                       QNLocalizedString & errorMessage);

    static bool noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                         QNLocalizedString & errorMessage, QString * plainText = Q_NULLPTR);

    static QStringList plainTextToListOfWords(const QString & plainText);

    static QString toDoCheckboxHtml(const bool checked, const quint64 idNumber);

    static QString encryptedTextHtml(const QString & encryptedText, const QString & hint,
                                     const QString & cipher, const size_t keyLength,
                                     const quint64 enCryptIndex);

    static QString decryptedTextHtml(const QString & decryptedText, const QString & encryptedText,
                                     const QString & hint, const QString & cipher,
                                     const size_t keyLength, const quint64 enDecryptedIndex);

    static QString resourceHtml(const IResource & resource, QNLocalizedString & errorDescription);

    static void escapeString(QString & string, const bool simplify = true);

private:
    Q_DISABLE_COPY(ENMLConverter)

private:
    ENMLConverterPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ENMLConverter)
};

} // namespace quentier

#endif // LIB_QUENTIER_ENML_ENML_CONVERTER_H
