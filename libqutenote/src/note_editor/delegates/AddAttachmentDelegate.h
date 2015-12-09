#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_ATTACHMENT_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_ATTACHMENT_DELEGATE_H

#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/ResourceWrapper.h>
#include <QObject>
#include <QByteArray>
#include <QUuid>
#include <QMimeType>
#include <QHash>
#include <QMimeType>

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

#ifdef USE_QT_WEB_ENGINE
QT_FORWARD_DECLARE_CLASS(GenericResourceImageWriter)
#endif

class AddAttachmentDelegate: public QObject
{
    Q_OBJECT
public:
    explicit AddAttachmentDelegate(const QString & filePath, NoteEditorPrivate & noteEditor,
                                   ResourceFileStorageManager * pResourceFileStorageManager,
                                   FileIOThreadWorker * pFileIOThreadWorker
#ifdef USE_QT_WEB_ENGINE
                                   , GenericResourceImageWriter * pGenericResourceImageWriter
#endif
                                   );

    void start();

Q_SIGNALS:

#ifdef USE_QT_WEB_ENGINE
    void finished(ResourceWrapper addedResource, QString resourceFileStoragePath,
                  QString resourceImageFilePath);
#else
    void finished(ResourceWrapper addedResource, QString resourceFileStoragePath);
#endif

    void notifyError(QString error);

// private signals
    void readFileData(QString filePath, QUuid requestId);
    void saveResourceToStorage(QString localGuid, QByteArray data, QByteArray dataHash,
                               QString fileStoragePath, QUuid requestId);

#ifdef USE_QT_WEB_ENGINE
    void saveGenericResourceImageToFile(QString localGuid, QByteArray data, QString fileSuffix,
                                        QByteArray dataHash, QString fileStoragePath, QUuid requestId);
#endif

private Q_SLOTS:
    void onResourceFileRead(bool success, QString errorDescription,
                            QByteArray data, QUuid requestId);
    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                  QString fileStoragePath, int errorCode,
                                  QString errorDescription);

#ifdef USE_QT_WEB_ENGINE
    void onGenericResourceImageSaved(bool success, QByteArray resourceImageDataHash,
                                     QString filePath, QString errorDescription,
                                     QUuid requestId);
#endif

private:
    NoteEditorPrivate &             m_noteEditor;
    ResourceFileStorageManager *    m_pResourceFileStorageManager;
    FileIOThreadWorker *            m_pFileIOThreadWorker;

#ifdef USE_QT_WEB_ENGINE
    GenericResourceImageWriter *    m_pGenericResourceImageWriter;
    QUuid                           m_saveResourceImageRequestId;
#endif

    const QString                   m_filePath;
    QMimeType                       m_resourceFileMimeType;
    ResourceWrapper                 m_resource;
    QString                         m_resourceFileStoragePath;

    QUuid                           m_readResourceFileRequestId;
    QUuid                           m_saveResourceToStorageRequestId;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_ATTACHMENT_DELEGATE_H
