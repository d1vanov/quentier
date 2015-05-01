#ifndef __QUTE_NOTE__CORE__CLIENT__ENML__ENML_CONVERTER_P_H
#define __QUTE_NOTE__CORE__CLIENT__ENML__ENML_CONVERTER_P_H

#include <QtGlobal>
#include <QStringList>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(HTMLCleaner)

class ENMLConverterPrivate
{
public:
    ENMLConverterPrivate();
    ~ENMLConverterPrivate();

    bool htmlToNoteContent(const QString & html, Note & note, QString & errorDescription) const;
    bool noteContentToHtml(const Note & note, QString & html, QString & errorDescription) const;

    bool validateEnml(const QString & enml, QString & errorDescription) const;

    static bool noteContentToPlainText(const QString & noteContent, QString & plainText,
                                       QString & errorMessage);

    static bool noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                         QString & errorMessage, QString * plainText = nullptr);

    static QStringList plainTextToListOfWords(const QString & plainText);

private:
    static bool isForbiddenXhtmlTag(const QString & tagName);
    static bool isForbiddenXhtmlAttribute(const QString & attributeName);
    static bool isEvernoteSpecificXhtmlTag(const QString & tagName);
    static bool isAllowedXhtmlTag(const QString & tagName);

private:
    Q_DISABLE_COPY(ENMLConverterPrivate)

private:
    mutable HTMLCleaner * m_pHtmlCleaner;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__CLIENT__ENML__ENML_CONVERTER_P_H
