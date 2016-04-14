#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/utility/Linkage.h>
#include <QObject>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QWidget)

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(Note)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManagerPrivate)

/**
 * @brief The ResourceFileStorageManager class is intended to provide the service of
 * reading and writing  the resource data from/to local files. The purpose of having
 * a separate class for that is to encapsulate the logics around the checks for resource
 * files actuality and also to make it possible to move all the resource file IO into a separate thread.
 */
class QUTE_NOTE_EXPORT ResourceFileStorageManager: public QObject
{
    Q_OBJECT
public:
    explicit ResourceFileStorageManager(const QString & imageResourceFileStorageFolderPath,
                                        QObject * parent = Q_NULLPTR);

    /**
     * @brief resourceFileStorageLocation - the method tries to return the most appropriate
     * generic (i.e. non-image) resource file storage location; first it tries to find it within application settings,
     * then tries to suggest on its own, then, as a last resort, opens a dialog for that.
     * The method also performs the check that the path it returns is writable (unless it's empty
     * due to the failure to find the appropriate path)
     * @param context - the widget in the context of which the modal dialog asking
     * user to specify the resource file storage location should be shown
     * @return resource file storage location path in the case of success or empty string in the case of failure
     */
    static QString resourceFileStorageLocation(QWidget * context);

    enum Errors
    {
        EmptyLocalUid = -1,
        EmptyRequestId = -2,
        EmptyData = -3,
        NoResourceFileStorageLocation = -4
    };

Q_SIGNALS:
    void writeResourceToFileCompleted(QUuid requestId, QByteArray dataHash,
                                      QString fileStoragePath, int errorCode, QString errorDescription);
    void readResourceFromFileCompleted(QUuid requestId, QByteArray data, QByteArray dataHash,
                                       int errorCode, QString errorDescription);

    void resourceFileChanged(QString localUid, QString fileStoragePath);

    void diagnosticsCollected(QUuid requestId, QString diagnostics);

public Q_SLOTS:
    /**
     * @brief onWriteResourceToFileRequest - slot being called when the resource data needs to be written
     * to local file; the method would also check that the already existing file (if any) is actual.
     * If so, it would return successfully without doing any IO.
     * @param noteLocalUid - the local uid of the note to which the resource belongs
     * @param resourceLocalUid - the local uid of the resource for which the data is written to file
     * @param data - the resource data to be written to file
     * @param dataHash - the hash of the resource data; if it's empty, it would be calculated by the method itself
     * @param preferredFileSuffix - the preferred file suffix for the resource; if empty, the resource file is written withoug suffix
     * @param requestId - request identifier for writing the data to file
     * @param isImage - indicates whether the resource is the image which can be displayed inline in the note editor page
     */
    void onWriteResourceToFileRequest(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QByteArray dataHash,
                                      QString preferredFileSuffix, QUuid requestId, bool isImage);

    /**
     * @brief onReadResourceFromFileRequest - slot being called when the resource data and hash need to be read
     * from local file
     * @param fileStoragePath - the path at which the resource is stored
     * @param resourceLocalUid - the local uid of the resource for which the data and hash should be read from file
     * @param requestId - request identifier for reading the resource data and hash from file
     */
    void onReadResourceFromFileRequest(QString fileStoragePath, QString resourceLocalUid, QUuid requestId);

    /**
     * @brief onOpenResourceRequest - slot being called when the resource file is requested to be opened
     * in any external program for viewing and/or editing; the resource file storage manager would watch
     * for the changes of this resource until the current note in the note editor is changed or until the file
     * is replaced with its own next version, for example
     */
    void onOpenResourceRequest(QString fileStoragePath);

    /**
     * @brief onCurrentNoteChanged - slot which should be called when the current note in the note editor is changed;
     * when the note is changed, this object stops watching for the changes of resource files belonging
     * to the previously edited note in the note editor; it is due to the performance cost and OS limitations
     * for the number of files which can be monitored for changes simultaneously by one process
     */
    void onCurrentNoteChanged(Note note);

    /**
     * @brief onRequestDiagnostics - slot which initiates the collection of diagnostics regarding the internal state of
     * ResourceFileStorageManager; intended primarily for troubleshooting purposes
     */
    void onRequestDiagnostics(QUuid requestId);

private:
    ResourceFileStorageManagerPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(ResourceFileStorageManager)
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__RESOURCE_FILES_STORAGE_MANAGER_H
