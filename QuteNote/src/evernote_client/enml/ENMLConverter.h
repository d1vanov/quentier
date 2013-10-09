#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H

class QString;
class QuteNoteTextEdit;

namespace qute_note {

void RichTextToENML(const QuteNoteTextEdit & noteEditor, QString & ENML);

}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
