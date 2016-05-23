#ifndef LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_IMAGE_RESOURCE_ROTATION_DELEGATE_H
#define LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_IMAGE_RESOURCE_ROTATION_DELEGATE_H

#include "../NoteEditor_p.h"
#include "JsResultCallbackFunctor.hpp"
#include <qute_note/types/ResourceWrapper.h>

namespace qute_note {

class ImageResourceRotationDelegate: public QObject
{
    Q_OBJECT
public:
    explicit ImageResourceRotationDelegate(const QByteArray & resourceHashBefore, const INoteEditorBackend::Rotation::type rotationDirection,
                                           NoteEditorPrivate & noteEditor, ResourceInfo & resourceInfo,
                                           ResourceFileStorageManager & resourceFileStorageManager,
                                           QHash<QString, QString> & resourceFileStoragePathsByLocalUid);
    void start();

Q_SIGNALS:
    void finished(QByteArray resourceDataBefore, QByteArray resourceHashBefore,
                  QByteArray resourceRecognitionDataBefore, QByteArray resourceRecognitionDataHashBefore,
                  ResourceWrapper resourceAfter, INoteEditorBackend::Rotation::type rotationDirection);
    void notifyError(QString error);

// private signals
    void saveResourceToStorage(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QByteArray dataHash,
                               QString preferredFileSuffix, QUuid requestId, bool isImage);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);
    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash, QString fileStoragePath,
                                  int errorCode, QString errorDescription);
    void onResourceTagHashUpdated(const QVariant & data);
    void onResourceTagSrcUpdated(const QVariant & data);

private:
    void rotateImageResource();

private:
    typedef JsResultCallbackFunctor<ImageResourceRotationDelegate> JsCallback;

private:
    NoteEditorPrivate &             m_noteEditor;
    ResourceInfo &                  m_resourceInfo;
    ResourceFileStorageManager &    m_resourceFileStorageManager;
    QHash<QString, QString> &       m_resourceFileStoragePathsByLocalUid;

    INoteEditorBackend::Rotation::type  m_rotationDirection;

    Note *                          m_pNote;

    QByteArray                      m_resourceDataBefore;
    QByteArray                      m_resourceHashBefore;

    QByteArray                      m_resourceRecognitionDataBefore;
    QByteArray                      m_resourceRecognitionDataHashBefore;

    QString                         m_resourceFileStoragePathBefore;
    QString                         m_resourceFileStoragePathAfter;

    ResourceWrapper                 m_rotatedResource;
    QUuid                           m_saveResourceRequestId;
};

} // namespace qute_note

#endif // LIB_QUTE_NOTE_NOTE_EDITOR_DELEGATES_IMAGE_RESOURCE_ROTATION_DELEGATE_H
