#ifndef __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_P_H
#define __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_P_H

#include <qute_note/enml/ENMLConverter.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QtGlobal>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QXmlStreamReader)
QT_FORWARD_DECLARE_CLASS(QXmlStreamWriter)
QT_FORWARD_DECLARE_CLASS(QXmlStreamAttributes)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)
QT_FORWARD_DECLARE_CLASS(HTMLCleaner)

class ENMLConverterPrivate
{
public:
    ENMLConverterPrivate();
    ~ENMLConverterPrivate();

    typedef ENMLConverter::NoteContentToHtmlExtraData NoteContentToHtmlExtraData;
    typedef ENMLConverter::SkipHtmlElementRule SkipHtmlElementRule;

    bool htmlToNoteContent(const QString & html,
                           const QVector<SkipHtmlElementRule> & skipRules,
                           QString & noteContent,
                           DecryptedTextManager & decryptedTextManager,
                           QString & errorDescription) const;
    bool noteContentToHtml(const QString & noteContent, QString & html,
                           QString & errorDescription,
                           DecryptedTextManager & decryptedTextManager,
                           NoteContentToHtmlExtraData & extraData) const;

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

    static QString resourceHtml(const IResource & resource, QString & errorDescription);

    static void escapeString(QString & string);

private:
    static bool isForbiddenXhtmlTag(const QString & tagName);
    static bool isForbiddenXhtmlAttribute(const QString & attributeName);
    static bool isEvernoteSpecificXhtmlTag(const QString & tagName);
    static bool isAllowedXhtmlTag(const QString & tagName);

    // convert <div> element with decrypted text to ENML <en-crypt> tag
    bool decryptedTextToEnml(QXmlStreamReader & reader,
                             DecryptedTextManager & decryptedTextManager,
                             QXmlStreamWriter & writer, QString & errorDescription) const;

    // convert ENML en-crypt tag to HTML <object> tag
    bool encryptedTextToHtml(const QXmlStreamAttributes & enCryptAttributes,
                             const QStringRef & encryptedTextCharacters, const quint64 enCryptIndex, const quint64 enDecryptedIndex,
                             QXmlStreamWriter & writer, DecryptedTextManager & decryptedTextManager, bool &convertedToEnCryptNode) const;

    // convert ENML <en-media> tag to HTML <object> tag
    static bool resourceInfoToHtml(const QXmlStreamAttributes & attributes, QXmlStreamWriter & writer,
                                   QString & errorDescription);

    void toDoTagsToHtml(const QXmlStreamReader & reader, const quint64 enToDoIndex,
                        QXmlStreamWriter & writer) const;

    static void decryptedTextHtml(const QString & decryptedText, const QString & encryptedText,
                                  const QString & hint, const QString & cipher, const size_t keyLength, const quint64 enDecryptedIndex,
                                  QXmlStreamWriter & writer);

    bool shouldSkipElement(const QString &elementName,
                           const QXmlStreamAttributes & attributes,
                           const QVector<SkipHtmlElementRule> & skipRules) const;

private:
    Q_DISABLE_COPY(ENMLConverterPrivate)

private:
    mutable HTMLCleaner *   m_pHtmlCleaner;
    mutable QString         m_cachedConvertedXml;   // Cached memory for the html converted to valid xml
};

} // namespace qute_note

__QUTE_NOTE_DECLARE_PRINTABLE(QXmlStreamAttributes)
__QUTE_NOTE_DECLARE_PRINTABLE(QVector<qute_note::ENMLConverter::SkipHtmlElementRule>)

#endif // __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_P_H
