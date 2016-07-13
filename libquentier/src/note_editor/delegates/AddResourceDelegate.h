#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATES_ADD_RESOURCE_DELEGATE_H
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATES_ADD_RESOURCE_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <quentier/types/Note.h>
#include <quentier/types/ResourceWrapper.h>
#include <QObject>
#include <QByteArray>
#include <QUuid>
#include <QMimeType>
#include <QHash>
#include <QMimeType>

namespace quentier {

QT_FORWARD_DECLARE_CLASS(NoteEditorPrivate)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)
QT_FORWARD_DECLARE_CLASS(GenericResourceImageManager)

class AddResourceDelegate: public QObject
{
    Q_OBJECT
public:
    explicit AddResourceDelegate(const QString & filePath, NoteEditorPrivate & noteEditor,
                                 ResourceFileStorageManager * pResourceFileStorageManager,
                                 FileIOThreadWorker * pFileIOThreadWorker,
                                 GenericResourceImageManager * pGenericResourceImageManager,
                                 QHash<QByteArray, QString> & genericResourceImageFilePathsByResourceHash);

    void start();

Q_SIGNALS:
    void finished(ResourceWrapper addedResource, QString resourceFileStoragePath);
    void notifyError(QNLocalizedString error);

// private signals
    void readFileData(QString filePath, QUuid requestId);
    void saveResourceToStorage(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QByteArray dataHash,
                               QString preferredFileSuffix, QUuid requestId, bool isImage);
    void writeFile(QString filePath, QByteArray data, QUuid requestId);

    void saveGenericResourceImageToFile(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QString fileSuffix,
                                        QByteArray dataHash, QString fileStoragePath, QUuid requestId);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);

    void onResourceFileRead(bool success, QNLocalizedString errorDescription,
                            QByteArray data, QUuid requestId);
    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                  QString fileStoragePath, int errorCode,
                                  QNLocalizedString errorDescription);

    void onGenericResourceImageSaved(bool success, QByteArray resourceImageDataHash,
                                     QString filePath, QNLocalizedString errorDescription,
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
    GenericResourceImageManager *   m_pGenericResourceImageManager;
    QUuid                           m_saveResourceImageRequestId;

    const QString                   m_filePath;
    QMimeType                       m_resourceFileMimeType;
    ResourceWrapper                 m_resource;
    QString                         m_resourceFileStoragePath;

    QUuid                           m_readResourceFileRequestId;
    QUuid                           m_saveResourceToStorageRequestId;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATES_ADD_RESOURCE_DELEGATE_H
