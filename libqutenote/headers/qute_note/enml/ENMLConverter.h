#ifndef __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_H
#define __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_H

#include <qute_note/utility/Linkage.h>
#include <qute_note/utility/Qt4Helper.h>
#include <QSet>
#include <QString>

namespace qute_note {

#ifndef USE_QT_WEB_ENGINE
QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactory)
#endif

QT_FORWARD_DECLARE_CLASS(DecryptedTextManager)
QT_FORWARD_DECLARE_CLASS(ENMLConverterPrivate)

class QUTE_NOTE_EXPORT ENMLConverter
{
public:
    ENMLConverter();
    virtual ~ENMLConverter();

    bool htmlToNoteContent(const QString & html, QString & noteContent,
                           DecryptedTextManager & decryptedTextManager,
                           QString & errorDescription) const;
    bool noteContentToHtml(const QString & noteContent, QString & html, QString & errorDescription,
                           DecryptedTextManager & decryptedTextManager
#ifndef USE_QT_WEB_ENGINE
                           , const NoteEditorPluginFactory * pluginFactory = Q_NULLPTR
#endif
                           ) const;

    bool validateEnml(const QString & enml, QString & errorDescription) const;

    static bool noteContentToPlainText(const QString & noteContent, QString & plainText,
                                       QString & errorMessage);

    static bool noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                         QString & errorMessage, QString * plainText = Q_NULLPTR);

    static QStringList plainTextToListOfWords(const QString & plainText);

    static QString toDoCheckboxHtml(const bool checked);

    static QString encryptedTextHtml(const QString & encryptedText, const QString & hint,
                                     const QString & cipher, const size_t keyLength);

    static QString decryptedTextHtml(const QString & decryptedText, const QString & encryptedText,
                                     const QString & hint, const QString & cipher, const size_t keyLength);

    static void escapeString(QString & string);

private:
    Q_DISABLE_COPY(ENMLConverter)

private:
    ENMLConverterPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ENMLConverter)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_H
