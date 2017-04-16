#include "EnexImporter.h"
#include "models/TagModel.h"
#include <quentier/local_storage/LocalStorageManagerThreadWorker.h>
#include <quentier/enml/ENMLConverter.h>
#include <quentier/logging/QuentierLogger.h>
#include <QFile>

namespace quentier {

EnexImporter::EnexImporter(const QString & enexFilePath,
                           LocalStorageManagerThreadWorker & localStorageWorker,
                           TagModel & tagModel, QObject * parent) :
    QObject(parent),
    m_localStorageWorker(localStorageWorker),
    m_tagModel(tagModel),
    m_enexFilePath(enexFilePath),
    m_addTagRequestIds(),
    m_tagsByName(),
    m_connectedToLocalStorage(false)
{
    if (!m_tagModel.allTagsListed()) {
        QObject::connect(&m_tagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                         this, QNSLOT(EnexImporter,onAllTagsListed));
    }
}

bool EnexImporter::isInProgress() const
{
    QNDEBUG(QStringLiteral("EnexImporter::isInProgress"));

    if (!m_addTagRequestIds.isEmpty()) {
        QNDEBUG(QStringLiteral("No pending requests to add tag"));
        return false;
    }

    return true;
}

void EnexImporter::start()
{
    QNDEBUG(QStringLiteral("EnexImporter::start"));

    QFile enexFile(m_enexFilePath);
    bool res = enexFile.open(QIODevice::ReadOnly);
    if (Q_UNLIKELY(!res)) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Can't import ENEX: can't open enex file for writing"));
        errorDescription.details() = m_enexFilePath;
        QNWARNING(errorDescription);
        emit enexImportFailed(errorDescription);
        return;
    }

    QString enex = QString::fromLocal8Bit(enexFile.readAll());
    enexFile.close();

    QVector<Note> importedNotes;
    QHash<QString, QStringList> tagNamesByNoteLocalUid;
    ErrorString errorDescription;
    ENMLConverter converter;
    res = converter.importEnex(enex, importedNotes, tagNamesByNoteLocalUid, errorDescription);
    if (!res) {
        emit enexImportFailed(errorDescription);
        return;
    }

    // TODO: implement further
}

void EnexImporter::clear()
{
    QNDEBUG(QStringLiteral("EnexImporter::clear"));

    m_addTagRequestIds.clear();
    m_tagsByName.clear();
    disconnectFromLocalStorage();
}

void EnexImporter::onAddTagComplete(Tag tag, QUuid requestId)
{
    auto it = m_addTagRequestIds.find(requestId);
    if (it == m_addTagRequestIds.end()) {
        return;
    }

    QNDEBUG(QStringLiteral("EnexImporter::onAddTagComplete: request id = ")
            << requestId << QStringLiteral(", tag: ") << tag);

    Q_UNUSED(m_addTagRequestIds.erase(it))

    if (Q_UNLIKELY(!tag.hasName())) {
        ErrorString errorDescription(QT_TRANSLATE_NOOP("", "Can't import ENEX: internal error, "
                                                       "could not create a new tag in the local storage: "
                                                       "the added tag has no name"));
        QNWARNING(errorDescription);
        emit enexImportFailed(errorDescription);
        return;
    }

    m_tagsByName[tag.name()] = tag;

    // TODO: implement further
}

void EnexImporter::onAddTagFailed(Tag tag, ErrorString errorDescription, QUuid requestId)
{
    QNDEBUG(QStringLiteral("EnexImporter::onAddTagFailed: request id = ")
            << requestId << QStringLiteral(", error description = ")
            << errorDescription << QStringLiteral(", tag: ") << tag);

    // TODO: implement
}

void EnexImporter::onAllTagsListed()
{
    QNDEBUG(QStringLiteral("EnexImporter::onAllTagsListed"));

    QObject::disconnect(&m_tagModel, QNSIGNAL(TagModel,notifyAllTagsListed),
                        this, QNSLOT(EnexImporter,onAllTagsListed));

    // TODO: implement further
}

void EnexImporter::connectToLocalStorage()
{
    QNDEBUG(QStringLiteral("EnexImporter::connectToLocalStorage"));

    if (m_connectedToLocalStorage) {
        QNDEBUG(QStringLiteral("Already connected to the local storage"));
        return;
    }

    QObject::connect(this, QNSIGNAL(EnexImporter,addTag,Tag,QUuid),
                     &m_localStorageWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddTagRequest,Tag,QUuid));
    QObject::connect(&m_localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagComplete,Tag,QUuid),
                     this, QNSLOT(EnexImporter,onAddTagComplete,Tag,QUuid));
    QObject::connect(&m_localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagFailed,Tag,ErrorString,QUuid),
                     this, QNSLOT(EnexImporter,onAddTagFailed,Tag,ErrorString,QUuid));

    m_connectedToLocalStorage = true;
}

void EnexImporter::disconnectFromLocalStorage()
{
    QNDEBUG(QStringLiteral("EnexImporter::disconnectFromLocalStorage"));

    if (!m_connectedToLocalStorage) {
        QNTRACE(QStringLiteral("Not connected to local storage at the moment"));
        return;
    }

    QObject::disconnect(this, QNSIGNAL(EnexImporter,addTag,Tag,QUuid),
                        &m_localStorageWorker, QNSLOT(LocalStorageManagerThreadWorker,onAddTagRequest,Tag,QUuid));
    QObject::disconnect(&m_localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagComplete,Tag,QUuid),
                        this, QNSLOT(EnexImporter,onAddTagComplete,Tag,QUuid));
    QObject::disconnect(&m_localStorageWorker, QNSIGNAL(LocalStorageManagerThreadWorker,addTagFailed,Tag,ErrorString,QUuid),
                        this, QNSLOT(EnexImporter,onAddTagFailed,Tag,ErrorString,QUuid));
}

} // namespace quentier
