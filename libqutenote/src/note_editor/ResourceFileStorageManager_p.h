#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_P_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_P_H

#include <QObject>
#include <QUuid>
#include <QStringList>
#include <QFileSystemWatcher>
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

    void resourceFileChanged(QString localGuid, QString fileStoragePath);

public Q_SLOTS:
    void onWriteResourceToFileRequest(QString localGuid, QByteArray data, QByteArray dataHash,
                                      QString fileStoragePath, QUuid requestId);
    void onReadResourceFromFileRequest(QString localGuid, QUuid requestId);

    void onOpenResourceRequest(QString fileStoragePath);

    void onCurrentNoteChanged(Note * pNote);

private Q_SLOTS:
    void onFileChanged(const QString & path);

private:
    void createConnections();
    QByteArray calculateHash(const QByteArray & data) const;
    bool checkIfResourceFileExistsAndIsActual(const QString & localGuid, const QString & fileStoragePath,
                                              const QByteArray & dataHash) const;

    bool updateResourceHash(const QString & resourceLocalGuid, const QByteArray & dataHash, int & errorCode, QString & errorDescription);
    void watchResourceFileForChanges(const QString & resourceLocalGuid, const QString & fileStoragePath);

private:
    QString     m_resourceFileStorageLocation;

    QHash<QString, QString>             m_resourceLocalGuidByFilePath;
    QFileSystemWatcher                  m_fileSystemWatcher;

    ResourceFileStorageManager * const q_ptr;
    Q_DECLARE_PUBLIC(ResourceFileStorageManager)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_P_H
