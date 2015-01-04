#include "NoteData.h"
#include "../../Utility.h"
#include <client/enml/ENMLConverter.h>
#include <logging/QuteNoteLogger.h>
#include <QDomDocument>
#include <QMutexLocker>

namespace qute_note {

NoteData::NoteData() :
    DataElementWithShortcutData(),
    m_qecNote(),
    m_resourcesAdditionalInfo(),
    m_thumbnail(),
    m_lazyPlainText(),
    m_lazyPlainTextIsValid(false),
    m_lazyListOfWords(),
    m_lazyListOfWordsIsValid(false),
    m_lazyContainsCheckedToDo(-1),
    m_lazyContainsUncheckedToDo(-1),
    m_lazyContainsEncryption(-1)
{}

NoteData::NoteData(const NoteData & other) :
    DataElementWithShortcutData(other),
    m_qecNote(other.m_qecNote),
    m_resourcesAdditionalInfo(other.m_resourcesAdditionalInfo),
    m_thumbnail(other.m_thumbnail),
    m_lazyPlainText(other.m_lazyPlainText),
    m_lazyPlainTextIsValid(other.m_lazyPlainTextIsValid),
    m_lazyListOfWords(other.m_lazyListOfWords),
    m_lazyListOfWordsIsValid(other.m_lazyListOfWordsIsValid),
    m_lazyContainsCheckedToDo(other.m_lazyContainsCheckedToDo),
    m_lazyContainsUncheckedToDo(other.m_lazyContainsUncheckedToDo),
    m_lazyContainsEncryption(other.m_lazyContainsEncryption)
{}

NoteData::NoteData(NoteData && other) :
    DataElementWithShortcutData(std::move(other)),
    m_qecNote(std::move(other.m_qecNote)),
    m_resourcesAdditionalInfo(std::move(other.m_resourcesAdditionalInfo)),
    m_thumbnail(std::move(other.m_thumbnail)),
    m_lazyPlainText(std::move(other.m_lazyPlainText)),
    m_lazyPlainTextIsValid(std::move(other.m_lazyPlainTextIsValid)),
    m_lazyListOfWords(std::move(other.m_lazyListOfWords)),
    m_lazyListOfWordsIsValid(std::move(other.m_lazyListOfWordsIsValid)),
    m_lazyContainsCheckedToDo(std::move(other.m_lazyContainsCheckedToDo)),
    m_lazyContainsUncheckedToDo(std::move(other.m_lazyContainsUncheckedToDo)),
    m_lazyContainsEncryption(std::move(other.m_lazyContainsEncryption))
{}

NoteData::~NoteData()
{}

NoteData::NoteData(const qevercloud::Note & other) :
    DataElementWithShortcutData(),
    m_qecNote(other),
    m_resourcesAdditionalInfo(),
    m_thumbnail(),
    m_lazyPlainText(),
    m_lazyPlainTextIsValid(false),
    m_lazyListOfWords(),
    m_lazyListOfWordsIsValid(false),
    m_lazyContainsCheckedToDo(-1),
    m_lazyContainsUncheckedToDo(-1),
    m_lazyContainsEncryption(-1)
{}

bool NoteData::ResourceAdditionalInfo::operator==(const NoteData::ResourceAdditionalInfo & other) const
{
    return (localGuid == other.localGuid) &&
            (noteLocalGuid == other.noteLocalGuid) &&
            (isDirty == other.isDirty);
}

bool NoteData::containsToDoImpl(const bool checked) const
{
    QMutex mutex;
    QMutexLocker mutexLocker(&mutex);

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

bool NoteData::containsEncryption() const
{
    QMutex mutex;
    QMutexLocker mutexLocker(&mutex);

    if (m_lazyContainsEncryption > (-1)) {
        if (m_lazyContainsEncryption == 0) {
            return false;
        }
        else {
            return true;
        }
    }

    if (!m_qecNote.content.isSet()) {
        m_lazyContainsEncryption = 0;
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
        m_lazyContainsEncryption = 0;
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
            if (tagName == "en-crypt") {
                m_lazyContainsEncryption = 1;
                return true;
            }
        }
        nextNode = nextNode.nextSibling();
    }

    m_lazyContainsEncryption = 0;
    return false;
}

void NoteData::setContent(const QString & content)
{
    if (!content.isEmpty()) {
        m_qecNote.content = content;
    }
    else {
        m_qecNote.content.clear();
    }

    m_lazyPlainTextIsValid = false;    // Mark any existing plain text as invalid but don't free memory
    m_lazyListOfWordsIsValid = false;
    m_lazyContainsCheckedToDo = -1;
    m_lazyContainsUncheckedToDo = -1;
    m_lazyContainsEncryption = -1;
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

bool NoteData::checkParameters(QString & errorDescription) const
{
    if (m_qecNote.guid.isSet() && !CheckGuid(m_qecNote.guid.ref())) {
        errorDescription = QT_TR_NOOP("Note's guid is invalid");
        return false;
    }

    if (m_qecNote.updateSequenceNum.isSet() && !CheckUpdateSequenceNumber(m_qecNote.updateSequenceNum)) {
        errorDescription = QT_TR_NOOP("Note's update sequence number is invalid");
        return false;
    }

    if (m_qecNote.title.isSet())
    {
        int titleSize = m_qecNote.title->size();

        if ( (titleSize < qevercloud::EDAM_NOTE_TITLE_LEN_MIN) ||
             (titleSize > qevercloud::EDAM_NOTE_TITLE_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Note's title length is invalid");
            return false;
        }
    }

    if (m_qecNote.content.isSet())
    {
        int contentSize = m_qecNote.content->size();

        if ( (contentSize < qevercloud::EDAM_NOTE_CONTENT_LEN_MIN) ||
             (contentSize > qevercloud::EDAM_NOTE_CONTENT_LEN_MAX) )
        {
            errorDescription = QT_TR_NOOP("Note's content length is invalid");
            return false;
        }
    }

    if (m_qecNote.contentHash.isSet())
    {
        size_t contentHashSize = m_qecNote.contentHash->size();

        if (contentHashSize != qevercloud::EDAM_HASH_LEN) {
            errorDescription = QT_TR_NOOP("Note's content hash size is invalid");
            return false;
        }
    }

    if (m_qecNote.notebookGuid.isSet() && !CheckGuid(m_qecNote.notebookGuid.ref())) {
        errorDescription = QT_TR_NOOP("Note's notebook guid is invalid");
        return false;
    }

    if (m_qecNote.tagGuids.isSet())
    {
        int numTagGuids = m_qecNote.tagGuids->size();

        if (numTagGuids > qevercloud::EDAM_NOTE_TAGS_MAX) {
            errorDescription = QT_TR_NOOP("Note has too many tags, max allowed ");
            errorDescription.append(QString::number(qevercloud::EDAM_NOTE_TAGS_MAX));
            return false;
        }
    }

    if (m_qecNote.resources.isSet())
    {
        int numResources = m_qecNote.resources->size();

        if (numResources > qevercloud::EDAM_NOTE_RESOURCES_MAX) {
            errorDescription = QT_TR_NOOP("Note has too many resources, max allowed ");
            errorDescription.append(QString::number(qevercloud::EDAM_NOTE_RESOURCES_MAX));
            return false;
        }
    }


    if (m_qecNote.attributes.isSet())
    {
        const qevercloud::NoteAttributes & attributes = m_qecNote.attributes;

#define CHECK_NOTE_ATTRIBUTE(name) \
    if (attributes.name.isSet()) { \
        int name##Size = attributes.name->size(); \
        \
        if ( (name##Size < qevercloud::EDAM_ATTRIBUTE_LEN_MIN) || \
             (name##Size > qevercloud::EDAM_ATTRIBUTE_LEN_MAX) ) \
        { \
            errorDescription = QT_TR_NOOP("Note attributes' " #name " field has invalid size"); \
            return false; \
        } \
    }

        CHECK_NOTE_ATTRIBUTE(author);
        CHECK_NOTE_ATTRIBUTE(source);
        CHECK_NOTE_ATTRIBUTE(sourceURL);
        CHECK_NOTE_ATTRIBUTE(sourceApplication);

#undef CHECK_NOTE_ATTRIBUTE

        if (attributes.contentClass.isSet())
        {
            int contentClassSize = attributes.contentClass->size();
            if ( (contentClassSize < qevercloud::EDAM_NOTE_CONTENT_CLASS_LEN_MIN) ||
                 (contentClassSize > qevercloud::EDAM_NOTE_CONTENT_CLASS_LEN_MAX) )
            {
                errorDescription = QT_TR_NOOP("Note attributes' content class has invalid size");
                return false;
            }
        }

        if (attributes.applicationData.isSet())
        {
            const qevercloud::LazyMap & applicationData = attributes.applicationData;

            if (applicationData.keysOnly.isSet())
            {
                const QSet<QString> & keysOnly = applicationData.keysOnly;
                foreach(const QString & key, keysOnly) {
                    int keySize = key.size();
                    if ( (keySize < qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                         (keySize > qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
                    {
                        errorDescription = QT_TR_NOOP("Note's attributes application data has invalid key (in keysOnly part)");
                        return false;
                    }
                }
            }

            if (applicationData.fullMap.isSet())
            {
                const QMap<QString, QString> & fullMap = applicationData.fullMap;
                for(QMap<QString, QString>::const_iterator it = fullMap.constBegin(); it != fullMap.constEnd(); ++it)
                {
                    int keySize = it.key().size();
                    if ( (keySize < qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MIN) ||
                         (keySize > qevercloud::EDAM_APPLICATIONDATA_NAME_LEN_MAX) )
                    {
                        errorDescription = QT_TR_NOOP("Note's attributes application data has invalid key (in fullMap part)");
                        return false;
                    }

                    int valueSize = it.value().size();
                    if ( (valueSize < qevercloud::EDAM_APPLICATIONDATA_VALUE_LEN_MIN) ||
                         (valueSize > qevercloud::EDAM_APPLICATIONDATA_VALUE_LEN_MAX) )
                    {
                        errorDescription = QT_TR_NOOP("Note's attributes application data has invalid value");
                        return false;
                    }

                    int sumSize = keySize + valueSize;
                    if (sumSize > qevercloud::EDAM_APPLICATIONDATA_ENTRY_LEN_MAX) {
                        errorDescription = QT_TR_NOOP("Note's attributes application data has invalid entry size");
                        return false;
                    }
                }
            }
        }

        if (attributes.classifications.isSet())
        {
            const QMap<QString, QString> & classifications = attributes.classifications;
            for(QMap<QString, QString>::const_iterator it = classifications.constBegin();
                it != classifications.constEnd(); ++it)
            {
                const QString & value = it.value();
                if (!value.startsWith("CLASSIFICATION_")) {
                    errorDescription = QT_TR_NOOP("Note's attributes classifications has invalid classification value");
                    return false;
                }
            }
        }
    }

    return true;
}

QString NoteData::plainText(QString * errorMessage) const
{
    QMutex mutex;
    QMutexLocker mutexLocker(&mutex);

    if (m_lazyPlainTextIsValid) {
        return m_lazyPlainText;
    }

    if (!m_qecNote.content.isSet()) {
        if (errorMessage) {
            *errorMessage = "Note content is not set";
        }
        return QString();
    }

    ENMLConverter converter;
    QString error;
    bool res = converter.noteContentToPlainText(m_qecNote.content.ref(),
                                                m_lazyPlainText, error);
    if (!res) {
        QNWARNING(error);
        if (errorMessage) {
            *errorMessage = error;
        }
        return QString();
    }

    m_lazyPlainTextIsValid = true;

    return m_lazyPlainText;
}

QStringList NoteData::listOfWords(QString * errorMessage) const
{
    QMutex mutex;
    QMutexLocker mutexLocker(&mutex);

    if (m_lazyListOfWordsIsValid) {
        return m_lazyListOfWords;
    }

    if (m_lazyPlainTextIsValid) {
        m_lazyListOfWords = ENMLConverter::plainTextToListOfWords(m_lazyPlainText);
        m_lazyListOfWordsIsValid = true;
        return m_lazyListOfWords;
    }

    // If still not returned, there's no plain text available so will get the list of words
    // from Note's content instead

    ENMLConverter converter;
    QString error;
    bool res = converter.noteContentToListOfWords(m_qecNote.content.ref(),
                                                  m_lazyListOfWords,
                                                  error, &m_lazyPlainText);
    if (!res) {
        QNWARNING(error);
        if (errorMessage) {
            *errorMessage = error;
        }
        return QStringList();
    }

    m_lazyPlainTextIsValid = true;
    m_lazyListOfWordsIsValid = true;

    return m_lazyListOfWords;
}

} // namespace qute_note
