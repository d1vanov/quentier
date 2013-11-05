#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H

class QString;
class QuteNoteTextEdit;

namespace qute_note {

class Note;

void RichTextToENML(const Note & note, const QuteNoteTextEdit & noteEditor,
                    QString & ENML);

bool ENMLToRichText(const Note & note, const QString & ENML,
                    QuteNoteTextEdit & noteEditor, QString & errorMessage);

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
