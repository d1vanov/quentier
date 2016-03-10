#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_RESOURCE_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_RESOURCE_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <qute_note/utility/Qt4Helper.h>
#include <qute_note/types/Note.h>
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
QT_FORWARD_DECLARE_CLASS(GenericResourceImageWriter)

class AddResourceDelegate: public QObject
{
    Q_OBJECT
public:
    explicit AddResourceDelegate(const QString & filePath, NoteEditorPrivate & noteEditor,
                                 ResourceFileStorageManager * pResourceFileStorageManager,
                                 FileIOThreadWorker * pFileIOThreadWorker,
                                 GenericResourceImageWriter * pGenericResourceImageWriter,
                                 QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash);

    void start();

Q_SIGNALS:
    void finished(ResourceWrapper addedResource, QString resourceFileStoragePath);
    void notifyError(QString error);

// private signals
    void readFileData(QString filePath, QUuid requestId);
    void saveResourceToStorage(QString localUid, QByteArray data, QByteArray dataHash,
                               QString fileStoragePath, QUuid requestId);
    void writeFile(QString filePath, QByteArray data, QUuid requestId);

    void saveGenericResourceImageToFile(QString localUid, QByteArray data, QString fileSuffix,
                                        QByteArray dataHash, QString fileStoragePath, QUuid requestId);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);

    void onResourceFileRead(bool success, QString errorDescription,
                            QByteArray data, QUuid requestId);
    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                  QString fileStoragePath, int errorCode,
                                  QString errorDescription);

    void onGenericResourceImageSaved(bool success, QByteArray resourceImageDataHash,
                                     QString filePath, QString errorDescription,
                                     QUuid requestId);

    void onNewResourceHtmlInserted(const QVariant & data);

private:
    void doStart();
    void insertNewResourceHtml();

private:
    typedef JsResultCallbackFunctor<AddResourceDelegate> JsCallback;

private:
    NoteEditorPrivate &             m_noteEditor;
    ResourceFileStorageManager *    m_pResourceFileStorageManager;
    FileIOThreadWorker *            m_pFileIOThreadWorker;

    QHash<QByteArray, QString> &    m_genericResourceImageFilePathsByResourceHash;
    GenericResourceImageWriter *    m_pGenericResourceImageWriter;
    QUuid                           m_saveResourceImageRequestId;

    const QString                   m_filePath;
    QMimeType                       m_resourceFileMimeType;
    ResourceWrapper                 m_resource;
    QString                         m_resourceFileStoragePath;

    QUuid                           m_readResourceFileRequestId;
    QUuid                           m_saveResourceToStorageRequestId;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__ADD_RESOURCE_DELEGATE_H
