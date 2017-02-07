/*
 * Copyright 2016 Dmitry Ivanov
 *
 * This file is part of libquentier
 *
 * libquentier is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * libquentier is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libquentier. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIB_QUENTIER_NOTE_EDITOR_DELEGATES_ADD_RESOURCE_DELEGATE_H
#define LIB_QUENTIER_NOTE_EDITOR_DELEGATES_ADD_RESOURCE_DELEGATE_H

#include "JsResultCallbackFunctor.hpp"
#include <quentier/utility/Macros.h>
#include <quentier/types/ErrorString.h>
#include <quentier/types/Note.h>
#include <quentier/types/Resource.h>
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
    void finished(Resource addedResource, QString resourceFileStoragePath);
    void notifyError(ErrorString error);

// private signals
    void readFileData(QString filePath, QUuid requestId);
    void saveResourceToStorage(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QByteArray dataHash,
                               QString preferredFileSuffix, QUuid requestId, bool isImage);
    void writeFile(QString filePath, QByteArray data, QUuid requestId);

    void saveGenericResourceImageToFile(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QString fileSuffix,
                                        QByteArray dataHash, QString fileStoragePath, QUuid requestId);

private Q_SLOTS:
    void onOriginalPageConvertedToNote(Note note);

    void onResourceFileRead(bool success, ErrorString errorDescription,
                            QByteArray data, QUuid requestId);
    void onResourceSavedToStorage(QUuid requestId, QByteArray dataHash,
                                  QString fileStoragePath, int errorCode,
                                  ErrorString errorDescription);

    void onGenericResourceImageSaved(bool success, QByteArray resourceImageDataHash,
                                     QString filePath, ErrorString errorDescription,
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
    Resource                        m_resource;
    QString                         m_resourceFileStoragePath;

    QUuid                           m_readResourceFileRequestId;
    QUuid                           m_saveResourceToStorageRequestId;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_DELEGATES_ADD_RESOURCE_DELEGATE_H
