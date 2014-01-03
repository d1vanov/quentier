#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H

#include <set>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QuteNoteTextEdit)
QT_FORWARD_DECLARE_CLASS(QTextFragment)
QT_FORWARD_DECLARE_CLASS(QDomElement)

namespace evernote {
namespace edam {

QT_FORWARD_DECLARE_CLASS(Note)

}
}

namespace qute_note {

class ENMLConverter
{
public:
    ENMLConverter();
    ENMLConverter(const ENMLConverter & other);
    ENMLConverter & operator=(const ENMLConverter & other);

    bool richTextToNote(const QuteNoteTextEdit & noteEditor, evernote::edam::Note & note,
                        QString & errorDescription) const;

    bool NoteToRichText(const evernote::edam::Note & note, QuteNoteTextEdit & noteEditor,
                        QString & errorMessage) const;

private:
    void fillTagsLists();
    bool encodeFragment(const QTextFragment & fragment, QString & encodedFragment,
                        QString & errorMessage) const;
    const QString domElementToRawXML(const QDomElement & elem) const;

    bool isForbiddenXhtmlTag(const QString & tagName) const;
    bool isForbiddenXhtmlAttribute(const QString & attributeName) const;
    bool isEvernoteSpecificXhtmlTag(const QString & tagName) const;
    bool isAllowedXhtmlTag(const QString & tagName) const;

    std::set<QString> m_forbiddenXhtmlTags;
    std::set<QString> m_forbiddenXhtmlAttributes;
    std::set<QString> m_evernoteSpecificXhtmlTags;
    std::set<QString> m_allowedXhtmlTags;
};


}

#endif // __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
