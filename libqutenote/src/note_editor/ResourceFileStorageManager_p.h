#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_P_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_P_H

#include <qute_note/utility/FileSystemWatcher.h>
#include <QObject>
#include <QUuid>
#include <QStringList>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)

class ResourceFileStorageManagerPrivate: public QObject
{
    Q_OBJECT
public:
    explicit ResourceFileStorageManagerPrivate(ResourceFileStorageManager & manager);

    static QString resourceFileStorageLocation(QWidget * context);

Q_SIGNALS:
    void writeResourceToFileCompleted(QUuid requestId, QByteArray dataHash, QString fileStoragePath,
                                      int errorCode, QString errorDescription);
    void readResourceFromFileCompleted(QUuid requestId, QByteArray data, QByteArray dataHash,
                                       int errorCode, QString errorDescription);

    void resourceFileChanged(QString localUid, QString fileStoragePath);

    void diagnosticsCollected(QUuid requestId, QString diagnostics);

public Q_SLOTS:
    void onWriteResourceToFileRequest(QString localUid, QByteArray data, QByteArray dataHash,
                                      QString fileStoragePath, QUuid requestId);
    void onReadResourceFromFileRequest(QString fileStoragePath, QString localUid, QUuid requestId);

    void onOpenResourceRequest(QString fileStoragePath);

    void onCurrentNoteChanged(Note note);

    void onRequestDiagnostics(QUuid requestId);

private Q_SLOTS:
    void onFileChanged(const QString & path);
    void onFileRemoved(const QString & path);

private:
    void createConnections();
    QByteArray calculateHash(const QByteArray & data) const;
    bool checkIfResourceFileExistsAndIsActual(const QString & localUid, const QString & fileStoragePath,
                                              const QByteArray & dataHash) const;

    bool updateResourceHash(const QString & resourceLocalUid, const QByteArray & dataHash, int & errorCode, QString & errorDescription);
    void watchResourceFileForChanges(const QString & resourceLocalUid, const QString & fileStoragePath);

private:
    QString     m_resourceFileStorageLocation;

    QHash<QString, QString>             m_resourceLocalUidByFilePath;
    FileSystemWatcher                   m_fileSystemWatcher;

    ResourceFileStorageManager * const q_ptr;
    Q_DECLARE_PUBLIC(ResourceFileStorageManager)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_P_H
