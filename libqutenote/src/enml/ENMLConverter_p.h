#ifndef __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_P_H
#define __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_P_H

#include <QtGlobal>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QXmlStreamReader)
QT_FORWARD_DECLARE_CLASS(QXmlStreamWriter)
QT_FORWARD_DECLARE_CLASS(QXmlStreamAttributes)

namespace qute_note {

#ifndef USE_QT_WEB_ENGINE
QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactory)
#endif

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)
QT_FORWARD_DECLARE_CLASS(HTMLCleaner)

class ENMLConverterPrivate
{
public:
    ENMLConverterPrivate();
    ~ENMLConverterPrivate();

    bool htmlToNoteContent(const QString & html, QString & noteContent, QString & errorDescription) const;
    bool noteContentToHtml(const QString & noteContent, QString & html, QString & errorDescription,
                           DecryptedTextManager & decryptedTextManager
#ifndef USE_QT_WEB_ENGINE
                           , const NoteEditorPluginFactory * pluginFactory
#endif
                           ) const;

    bool validateEnml(const QString & enml, QString & errorDescription) const;

    static bool noteContentToPlainText(const QString & noteContent, QString & plainText,
                                       QString & errorMessage);

    static bool noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                         QString & errorMessage, QString * plainText = nullptr);

    static QStringList plainTextToListOfWords(const QString & plainText);

    static QString getToDoCheckboxHtml(const bool checked);

private:
    static bool isForbiddenXhtmlTag(const QString & tagName);
    static bool isForbiddenXhtmlAttribute(const QString & attributeName);
    static bool isEvernoteSpecificXhtmlTag(const QString & tagName);
    static bool isAllowedXhtmlTag(const QString & tagName);

    // convert <div> element with decrypted text to ENML <en-crypt> tag
    bool decryptedTextToEnml(const QXmlStreamReader & reader, QXmlStreamWriter & writer,
                             QString & errorDescription) const;

    // convert ENML en-crypt tag to HTML <object> tag
    bool encryptedTextToHtml(const QXmlStreamAttributes & enCryptAttributes,
                             const QStringRef & encryptedTextCharacters,
                             QXmlStreamWriter & writer,
                             DecryptedTextManager & decryptedTextManager) const;

    // convert ENML <en-media> tag to HTML <object> tag
    bool resourceInfoToHtml(const QXmlStreamReader & reader, QXmlStreamWriter & writer,
                            QString & errorDescription
#ifndef USE_QT_WEB_ENGINE
                            , const NoteEditorPluginFactory * pluginFactory
#endif
                            ) const;

    void toDoTagsToHtml(const QXmlStreamReader & reader, QXmlStreamWriter & writer) const;

private:
    Q_DISABLE_COPY(ENMLConverterPrivate)

private:
    mutable HTMLCleaner *   m_pHtmlCleaner;
    mutable QString         m_cachedConvertedXml;   // Cached memory for the html converted to valid xml
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_P_H
