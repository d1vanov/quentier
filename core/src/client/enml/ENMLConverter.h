#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H

#include <tools/Linkage.h>
#include <tools/qt4helper.h>
#include <QSet>
#include <QString>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(ENMLConverterPrivate)

class QUTE_NOTE_EXPORT ENMLConverter
{
public:
    ENMLConverter();
    virtual ~ENMLConverter();

    bool htmlToNoteContent(const QString & html, Note & note, QString & errorDescription) const;
    bool noteContentToHtml(const Note & note, QString & html, qint32 & lastFreeImageId, QString & errorDescription) const;

    bool validateEnml(const QString & enml, QString & errorDescription) const;

    static bool noteContentToPlainText(const QString & noteContent, QString & plainText,
                                       QString & errorMessage);

    static bool noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                         QString & errorMessage, QString * plainText = nullptr);

    static QStringList plainTextToListOfWords(const QString & plainText);

    static QString getToDoCheckboxHtml(const bool checked, const qint32 id);

private:
    Q_DISABLE_COPY(ENMLConverter)

private:
    ENMLConverterPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ENMLConverter)
};

} // namespace qute_note

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
