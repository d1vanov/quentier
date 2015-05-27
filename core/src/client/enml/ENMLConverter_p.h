#ifndef __QUTE_NOTE__CORE__CLIENT__ENML__ENML_CONVERTER_P_H
#define __QUTE_NOTE__CORE__CLIENT__ENML__ENML_CONVERTER_P_H

#include <QtGlobal>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QXmlStreamReader)
QT_FORWARD_DECLARE_CLASS(QXmlStreamWriter)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(HTMLCleaner)

class ENMLConverterPrivate
{
public:
    ENMLConverterPrivate();
    ~ENMLConverterPrivate();

    bool htmlToNoteContent(const QString & html, Note & note, QString & errorDescription) const;
    bool noteContentToHtml(const Note & note, QString & html, qint32 & lastFreeImageId,
                           QString & errorDescription) const;

    bool validateEnml(const QString & enml, QString & errorDescription) const;

    static bool noteContentToPlainText(const QString & noteContent, QString & plainText,
                                       QString & errorMessage);

    static bool noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                         QString & errorMessage, QString * plainText = nullptr);

    static QStringList plainTextToListOfWords(const QString & plainText);

    static QString getToDoCheckboxHtml(const bool checked, const qint32 id);

private:
    static bool isForbiddenXhtmlTag(const QString & tagName);
    static bool isForbiddenXhtmlAttribute(const QString & attributeName);
    static bool isEvernoteSpecificXhtmlTag(const QString & tagName);
    static bool isAllowedXhtmlTag(const QString & tagName);

    bool writeObjectInfoToEnml(QXmlStreamReader & reader, const QString & namespaceUri,
                               QXmlStreamWriter & writer, QString & errorDescription) const;

    bool writeEnCryptTagToEnml(QXmlStreamReader & reader, const QString & namespaceUri,
                               QXmlStreamWriter & writer, QString & errorDescription) const;
    bool writeEnCryptTagToHtml(const QXmlStreamReader & reader, const QString & namespaceUri,
                               QXmlStreamWriter & writer, QString & errorDescription) const;

    bool convertEnToDoTagsToHtml(QString & html, qint32 & lastFreeImageId, QString & errorDescription) const;

private:
    Q_DISABLE_COPY(ENMLConverterPrivate)

private:
    mutable HTMLCleaner *   m_pHtmlCleaner;
    mutable QString         m_cachedNoteContent;    // Cached memory for the converted note ENML content
    mutable QString         m_cachedConvertedXml;   // Cached memory for the html converted to valid xml
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__ENML__ENML_CONVERTER_P_H
