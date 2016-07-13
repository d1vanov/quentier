#ifndef LIB_QUENTIER_NOTE_EDITOR_RESOURCE_FILES_STORAGE_MANAGER_P_H
#define LIB_QUENTIER_NOTE_EDITOR_RESOURCE_FILES_STORAGE_MANAGER_P_H

#include <quentier/utility/FileSystemWatcher.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QObject>
#include <QUuid>
#include <QStringList>
#include <QHash>
#include <QScopedPointer>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)

class ResourceFileStorageManagerPrivate: public QObject
{
    Q_OBJECT
public:
    explicit ResourceFileStorageManagerPrivate(const QString & imageResourceFileStorageFolderPath,
                                               ResourceFileStorageManager & manager);

    static QString resourceFileStorageLocation(QWidget * context);

Q_SIGNALS:
    void writeResourceToFileCompleted(QUuid requestId, QByteArray dataHash, QString fileStoragePath,
                                      int errorCode, QNLocalizedString errorDescription);
    void readResourceFromFileCompleted(QUuid requestId, QByteArray data, QByteArray dataHash,
                                       int errorCode, QNLocalizedString errorDescription);

    void resourceFileChanged(QString localUid, QString fileStoragePath);

    void diagnosticsCollected(QUuid requestId, QString diagnostics);

public Q_SLOTS:
    void onWriteResourceToFileRequest(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QByteArray dataHash,
                                      QString preferredFileSuffix, QUuid requestId, bool isImage);
    void onReadResourceFromFileRequest(QString fileStoragePath, QString resourceLocalUid, QUuid requestId);

    void onOpenResourceRequest(QString fileStoragePath);

    void onCurrentNoteChanged(Note note);

    void onRequestDiagnostics(QUuid requestId);

private Q_SLOTS:
    void onFileChanged(const QString & path);
    void onFileRemoved(const QString & path);

private:
    void createConnections();
    QByteArray calculateHash(const QByteArray & data) const;
    bool checkIfResourceFileExistsAndIsActual(const QString & noteLocalUid, const QString & resourceLocalUid,
                                              const QString & fileStoragePath, const QByteArray & dataHash) const;

    bool updateResourceHash(const QString & resourceLocalUid, const QByteArray & dataHash,
                            const QString & storageFolderPath, int & errorCode, QNLocalizedString & errorDescription);
    void watchResourceFileForChanges(const QString & resourceLocalUid, const QString & fileStoragePath);
    void stopWatchingResourceFile(const QString & filePath);

    void removeStaleResourceFilesFromCurrentNote();

private:
    QString     m_imageResourceFileStorageLocation;
    QString     m_resourceFileStorageLocation;

    QScopedPointer<Note>                m_pCurrentNote;

    QHash<QString, QString>             m_resourceLocalUidByFilePath;
    FileSystemWatcher                   m_fileSystemWatcher;

    ResourceFileStorageManager * const q_ptr;
    Q_DECLARE_PUBLIC(ResourceFileStorageManager)
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_RESOURCE_FILES_STORAGE_MANAGER_P_H
