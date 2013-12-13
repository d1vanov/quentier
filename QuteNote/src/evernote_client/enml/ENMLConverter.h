#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H

#include <set>
#include <QString>

class QuteNoteTextEdit;
class QTextFragment;
class QDomElement;

namespace qute_note {

class Note;

class ENMLConverter
{
public:
    ENMLConverter();
    ENMLConverter(const ENMLConverter & other);
    ENMLConverter & operator=(const ENMLConverter & other);

    void richTextToENML(const QuteNoteTextEdit & noteEditor, QString & ENML) const;

    bool ENMLToRichText(const QString & ENML, QuteNoteTextEdit & noteEditor,
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
