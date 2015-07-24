#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H

#include <note_editor/DecryptedTextCache.h>
#include <tools/Linkage.h>
#include <tools/qt4helper.h>
#include <QSet>
#include <QString>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPluginFactory)
QT_FORWARD_DECLARE_CLASS(ENMLConverterPrivate)

class QUTE_NOTE_EXPORT ENMLConverter
{
public:
    ENMLConverter();
    virtual ~ENMLConverter();

    bool htmlToNoteContent(const QString & html, QString & noteContent, QString & errorDescription) const;
    bool noteContentToHtml(const QString & noteContent, QString & html, QString & errorDescription,
                           DecryptedTextCachePtr decryptedTextCache = DecryptedTextCachePtr(),
                           const NoteEditorPluginFactory * pluginFactory = nullptr) const;

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

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
