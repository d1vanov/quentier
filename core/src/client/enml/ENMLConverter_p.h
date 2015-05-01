#ifndef __QUTE_NOTE__CORE__CLIENT__ENML__ENML_CONVERTER_P_H
#define __QUTE_NOTE__CORE__CLIENT__ENML__ENML_CONVERTER_P_H

#include <QtGlobal>

namespace qute_note {
QT_FORWARD_DECLARE_CLASS(Note)
}

namespace qute_note_private {

class ENMLConverterPrivate
{
public:
    ENMLConverterPrivate();

    // TODO: move the implementation of these methods here as well
    /*
    bool htmlToNoteContent(const QString & html, Note & note, QString & errorDescription) const;
    bool noteContentToHtml(const Note & note, QString & html, QString & errorDescription) const;
    */

    bool validateEnml(const QString & enml, QString & errorDescription) const;

private:
    Q_DISABLE_COPY(ENMLConverterPrivate)
};

} // namespace qute_note_private

#endif // __QUTE_NOTE__CORE__CLIENT__ENML__ENML_CONVERTER_P_H
