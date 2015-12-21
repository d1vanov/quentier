#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__IMAGE_RESOURCE_ROTATION_DELEGATE_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__IMAGE_RESOURCE_ROTATION_DELEGATE_H

#include "../NoteEditor_p.h"
#include "JsResultCallbackFunctor.hpp"
#include <qute_note/types/ResourceWrapper.h>

namespace qute_note {

class ImageResourceRotationDelegate: public QObject
{
    Q_OBJECT
public:
    explicit ImageResourceRotationDelegate(const QString & resourceHashBefore, const INoteEditorBackend::Rotation::type rotationDirection,
                                           NoteEditorPrivate & noteEditor, ResourceInfo & resourceInfo,
                                           ResourceFileStorageManager & resourceFileStorageManager,
                                           QHash<QString, QString> & resourceFileStoragePathsByLocalGuid);
    void start();

Q_SIGNALS:
    void finished(QByteArray resourceDataBefore, QString resourceHashBefore, ResourceWrapper resourceAfter,
                  INoteEditorBackend::Rotation::type rotationDirection);
    void notifyError(QString error);

// private signals
    void saveResourceToStorage(QString localGuid, QByteArray data, QByteArray dataHash,
                               QString fileStoragePath, QUuid requestId);

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
    QHash<QString, QString> &       m_resourceFileStoragePathsByLocalGuid;

    INoteEditorBackend::Rotation::type  m_rotationDirection;

    Note *                          m_pNote;

    QByteArray                      m_resourceDataBefore;
    QString                         m_resourceHashBefore;

    QString                         m_resourceFileStoragePathBefore;
    QString                         m_resourceFileStoragePathAfter;

    ResourceWrapper                 m_rotatedResource;
    QUuid                           m_saveResourceRequestId;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__DELEGATES__IMAGE_RESOURCE_ROTATION_DELEGATE_H
