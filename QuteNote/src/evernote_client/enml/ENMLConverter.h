#ifndef __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H
#define __QUTE_NOTE__EVERNOTE_CLIENT__ENML_CONVERTER_H

#include <set>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QuteNoteTextEdit)
QT_FORWARD_DECLARE_CLASS(QTextFragment)
QT_FORWARD_DECLARE_CLASS(QDomElement)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(DatabaseManager)
QT_FORWARD_DECLARE_CLASS(Note)

class ENMLConverter
{
public:
    ENMLConverter();
    ENMLConverter(const ENMLConverter & other);
    ENMLConverter & operator=(const ENMLConverter & other);

    bool richTextToENML(const QuteNoteTextEdit & noteEditor, QString & ENML,
                        QString & errorDescription) const;

    bool ENMLToRichText(const QString & ENML, const DatabaseManager & databaseManager,
                        QuteNoteTextEdit & noteEditor, QString & errorMessage) const;

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
