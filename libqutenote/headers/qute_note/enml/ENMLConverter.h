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

    bool htmlToNoteContent(const QString & html, QString & noteContent, QString & errorDescription) const;
    bool noteContentToHtml(const QString & noteContent, QString & html, QString & errorDescription,
                           DecryptedTextManager & decryptedTextManager
#ifndef USE_QT_WEB_ENGINE
                           , const NoteEditorPluginFactory * pluginFactory = nullptr
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
    Q_DISABLE_COPY(ENMLConverter)

private:
    ENMLConverterPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ENMLConverter)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__ENML__ENML_CONVERTER_H
