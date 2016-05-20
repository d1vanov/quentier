#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_GENERIC_RESOURCE_IMAGE_MANAGER_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_GENERIC_RESOURCE_IMAGE_MANAGER_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
#include <QObject>
#include <QUuid>
#include <QScopedPointer>

namespace qute_note {

/**
 * @brief The GenericResourceImageManager class is a worker for the I/O thread which
 * would write two files in a folder accessible for note editor's page: the composed image
 * for a generic resource and the hash of that resource. It would also listen to the current note
 * changes and remove stale generic resource images as appropriate.
 */
class GenericResourceImageManager: public QObject
{
    Q_OBJECT
public:
    explicit GenericResourceImageManager(QObject * parent = Q_NULLPTR);

    void setStorageFolderPath(const QString & storageFolderPath);

Q_SIGNALS:
    void genericResourceImageWriteReply(bool success, QByteArray resourceHash, QString filePath,
                                        QString errorDescription, QUuid requestId);

public Q_SLOTS:
    void onGenericResourceImageWriteRequest(QString noteLocalUid, QString resourceLocalUid, QByteArray resourceImageData,
                                            QString resourceFileSuffix, QByteArray resourceActualHash,
                                            QString resourceDisplayName, QUuid requestId);
    void onCurrentNoteChanged(Note note);

private:
    void removeStaleGenericResourceImageFilesFromCurrentNote();

private:
    QString                 m_storageFolderPath;
    QScopedPointer<Note>    m_pCurrentNote;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_GENERIC_RESOURCE_IMAGE_MANAGER_H
