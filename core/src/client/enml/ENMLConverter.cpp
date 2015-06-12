#include "ENMLConverter.h"
#include "ENMLConverter_p.h"
#include <note_editor/NoteEditorPluginFactory.h>
#include <logging/QuteNoteLogger.h>
#include <client/types/Note.h>

namespace qute_note {

ENMLConverter::ENMLConverter() :
    d_ptr(new ENMLConverterPrivate)
{}

ENMLConverter::~ENMLConverter()
{
    delete d_ptr;
}

bool ENMLConverter::htmlToNoteContent(const QString & html, Note & note, QString & errorDescription) const
{
    QNDEBUG("ENMLConverter::htmlToNoteContent: note local guid = " << note.localGuid());

    Q_D(const ENMLConverter);
    return d->htmlToNoteContent(html, note, errorDescription);
}

bool ENMLConverter::noteContentToHtml(const Note & note, QString & html, QString & errorDescription,
                                      DecryptedTextCachePtr decryptedTextCache,
                                      const NoteEditorPluginFactory * pluginFactory) const
{
    QNDEBUG("ENMLConverter::noteContentToHtml: note local guid = " << note.localGuid());

    Q_D(const ENMLConverter);
    return d->noteContentToHtml(note, html, errorDescription, decryptedTextCache, pluginFactory);
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

