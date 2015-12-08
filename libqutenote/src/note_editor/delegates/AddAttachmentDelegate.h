#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_ATTACHMENT_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_ATTACHMENT_DELEGATE_H

#include <qute_note/utility/Qt4Helper.h>
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
    void finished();
    void notifyError(QString error);

// private signals
    void readFileData(QString filePath, QUuid requestId);
    void saveResourceToStorage(QString localGuid, QByteArray data, QByteArray dataHash,
                               QString fileStoragePath, QUuid requestId);

private Q_SLOTS:
    void onResourceFileRead(bool success, QString errorDescription,
                            QByteArray data, QUuid requestId);
    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                  QString fileStoragePath, int errorCode,
                                  QString errorDescription);

#ifdef USE_QT_WEB_ENGINE
    void onGenericResourceImageSaved(const bool success, const QByteArray resourceActualHash,
                                     const QString filePath, const QString errorDescription,
                                     const QUuid requestId);
#endif

private:
    NoteEditorPrivate &             m_noteEditor;
    ResourceFileStorageManager *    m_pResourceFileStorageManager;
    FileIOThreadWorker *            m_pFileIOThreadWorker;

#ifdef USE_QT_WEB_ENGINE
    GenericResourceImageWriter *    m_pGenericResourceImageWriter;
#endif

    const QString                   m_filePath;
    QMimeType                       m_resourceFileMimeType;
    QUuid                           m_readResourceFileRequestId;

    QString                         m_resourceLocalGuid;
    QUuid                           m_saveResourceToStorageRequestId;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_ATTACHMENT_DELEGATE_H
