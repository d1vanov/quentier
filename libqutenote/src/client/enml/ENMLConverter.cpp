#include <qute_note/enml/ENMLConverter.h>
#include "ENMLConverter_p.h"
#include <qute_note/note_editor/NoteEditorPluginFactory.h>
#include <qute_note/logging/QuteNoteLogger.h>

namespace qute_note {

ENMLConverter::ENMLConverter() :
    d_ptr(new ENMLConverterPrivate)
{}

ENMLConverter::~ENMLConverter()
{
    delete d_ptr;
}

bool ENMLConverter::htmlToNoteContent(const QString & html, QString & noteContent, QString & errorDescription) const
{
    QNDEBUG("ENMLConverter::htmlToNoteContent");

    Q_D(const ENMLConverter);
    return d->htmlToNoteContent(html, noteContent, errorDescription);
}

bool ENMLConverter::noteContentToHtml(const QString & noteContent, QString & html, QString & errorDescription,
                                      DecryptedTextCachePtr decryptedTextCache,
                                      const NoteEditorPluginFactory * pluginFactory) const
{
    QNDEBUG("ENMLConverter::noteContentToHtml");

    Q_D(const ENMLConverter);
    return d->noteContentToHtml(noteContent, html, errorDescription, decryptedTextCache, pluginFactory);
}

bool ENMLConverter::validateEnml(const QString & enml, QString & errorDescription) const
{
    Q_D(const ENMLConverter);
    return d->validateEnml(enml, errorDescription);
}

bool ENMLConverter::noteContentToPlainText(const QString & noteContent, QString & plainText,
                                           QString & errorMessage)
{
    return ENMLConverterPrivate::noteContentToPlainText(noteContent, plainText, errorMessage);
}

bool ENMLConverter::noteContentToListOfWords(const QString & noteContent, QStringList & listOfWords,
                                             QString & errorMessage, QString * plainText)
{
    return ENMLConverterPrivate::noteContentToListOfWords(noteContent, listOfWords, errorMessage, plainText);
}

QStringList ENMLConverter::plainTextToListOfWords(const QString & plainText)
{
    return ENMLConverterPrivate::plainTextToListOfWords(plainText);
}

QString ENMLConverter::getToDoCheckboxHtml(const bool checked)
{
    return ENMLConverterPrivate::getToDoCheckboxHtml(checked);
}

} // namespace qute_note

