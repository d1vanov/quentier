#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_P_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_P_H

#include <QObject>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace qute_note {

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

public Q_SLOTS:
    void onWriteResourceToFileRequest(QString localGuid, QByteArray data, QByteArray dataHash,
                                      QString fileStoragePath, QUuid requestId);
    void onReadResourceFromFileRequest(QString localGuid, QUuid requestId);

private:
    void createConnections();
    QByteArray calculateHash(const QByteArray & data) const;
    bool checkIfResourceFileExistsAndIsActual(const QString & localGuid, const QString & fileStoragePath,
                                              const QByteArray & dataHash) const;

private:
    QString     m_resourceFileStorageLocation;

    ResourceFileStorageManager * const q_ptr;
    Q_DECLARE_PUBLIC(ResourceFileStorageManager)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_P_H
