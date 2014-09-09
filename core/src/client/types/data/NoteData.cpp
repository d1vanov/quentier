#include "NoteData.h"
#include <logging/QuteNoteLogger.h>
#include <QDomDocument>

namespace qute_note {

NoteData::NoteData() :
    m_isLocal(true),
    m_thumbnail(),
    m_lazyPlainText(),
    m_lazyPlainTextIsValid(false),
    m_lazyListOfWords(),
    m_lazyListOfWordsIsValid(false),
    m_lazyContainsCheckedToDo(-1),
    m_lazyContainsUncheckedToDo(-1),
    m_lazyContainsEncryption(-1)
{}

NoteData::NoteData(NoteData && other)
{
    m_lazyPlainText = other.m_lazyPlainText;
    other.m_lazyPlainText.clear();

    m_lazyPlainTextIsValid = other.m_lazyPlainTextIsValid;
    other.m_lazyPlainTextIsValid = false;

    m_lazyListOfWords = other.m_lazyListOfWords;
    other.m_lazyListOfWords.clear();

    m_lazyListOfWordsIsValid = other.m_lazyListOfWordsIsValid;
    other.m_lazyListOfWordsIsValid = false;
}

NoteData::~NoteData()
{}

NoteData::NoteData(const qevercloud::Note & other) :
    m_qecNote(other),
    m_isLocal(false),
    m_thumbnail(),
    m_lazyPlainText(),
    m_lazyPlainTextIsValid(false),
    m_lazyListOfWords(),
    m_lazyListOfWordsIsValid(false),
    m_lazyContainsCheckedToDo(-1),
    m_lazyContainsUncheckedToDo(-1),
    m_lazyContainsEncryption(-1)
{}

NoteData & NoteData::operator=(const qevercloud::Note & other)
{
    m_qecNote = other;
    m_lazyPlainTextIsValid = false;    // Mark any existing plain text as invalid but don't free memory
    m_lazyListOfWordsIsValid = false;
    m_lazyContainsCheckedToDo = -1;
    m_lazyContainsUncheckedToDo = -1;
    m_lazyContainsEncryption = -1;
    return *this;
}

NoteData & NoteData::operator=(NoteData && other)
{
    if (this != &other)
    {
        m_qecNote = std::move(other.m_qecNote);
        m_isLocal = std::move(other.m_isLocal);
        m_thumbnail = std::move(other.m_thumbnail);

        m_lazyPlainText = other.m_lazyPlainText;
        other.m_lazyPlainText.clear();

        m_lazyPlainTextIsValid = other.m_lazyPlainTextIsValid;
        other.m_lazyPlainTextIsValid = false;

        m_lazyListOfWords = other.m_lazyListOfWords;
        other.m_lazyListOfWords.clear();

        m_lazyListOfWordsIsValid = other.m_lazyListOfWordsIsValid;
        other.m_lazyListOfWordsIsValid = false;

        m_lazyContainsCheckedToDo   = other.m_lazyContainsCheckedToDo;
        m_lazyContainsUncheckedToDo = other.m_lazyContainsUncheckedToDo;
        m_lazyContainsEncryption    = other.m_lazyContainsEncryption;
    }

    return *this;
}

bool NoteData::ResourceAdditionalInfo::operator==(const NoteData::ResourceAdditionalInfo & other) const
{
    return (localGuid == other.localGuid) &&
            (noteLocalGuid == other.noteLocalGuid) &&
            (isDirty == other.isDirty);
}


bool NoteData::containsToDoImpl(const bool checked) const
{
    int & refLazyContainsToDo = (checked ? m_lazyContainsCheckedToDo : m_lazyContainsUncheckedToDo);
    if (refLazyContainsToDo > (-1)) {
        if (refLazyContainsToDo == 0) {
            return false;
        }
        else {
            return true;
        }
    }

    if (!m_qecNote.content.isSet()) {
        refLazyContainsToDo = 0;
        return false;
    }

    QDomDocument enXmlDomDoc;
    int errorLine = -1, errorColumn = -1;
    QString errorMessage;
    bool res = enXmlDomDoc.setContent(m_qecNote.content.ref(), &errorMessage, &errorLine, &errorColumn);
    if (!res) {
        // TRANSLATOR Explaining the error of XML parsing
        errorMessage += QT_TR_NOOP(". Error happened at line ") +
                        QString::number(errorLine) + QT_TR_NOOP(", at column ") +
                        QString::number(errorColumn);
        QNWARNING("Note content parsing error: " << errorMessage);
        refLazyContainsToDo = 0;
        return false;
    }

    QDomElement docElem = enXmlDomDoc.documentElement();
    QDomNode nextNode = docElem.firstChild();
    while (!nextNode.isNull())
    {
        QDomElement element = nextNode.toElement();
        if (!element.isNull())
        {
            QString tagName = element.tagName();
            if (tagName == "en-todo")
            {
                QString checkedStr = element.attribute("checked", "false");
                if (checked && (checkedStr == "true")) {
                    refLazyContainsToDo = 1;
                    return true;
                }
                else if (!checked && (checkedStr == "false")) {
                    refLazyContainsToDo = 1;
                    return true;
                }
            }
        }
        nextNode = nextNode.nextSibling();
    }

    refLazyContainsToDo = 0;
    return false;
}

void NoteData::clear()
{
    m_qecNote = qevercloud::Note();
    m_lazyPlainTextIsValid = false;    // Mark any existing plain text as invalid but don't free memory
    m_lazyListOfWordsIsValid = false;
    m_lazyContainsCheckedToDo = -1;
    m_lazyContainsUncheckedToDo = -1;
    m_lazyContainsEncryption = -1;
}

} // namespace qute_note
