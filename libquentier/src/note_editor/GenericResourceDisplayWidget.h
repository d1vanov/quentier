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

#ifndef LIB_QUENTIER_NOTE_EDITOR_GENERIC_RESOURCE_DISPLAY_WIDGET_H
#define LIB_QUENTIER_NOTE_EDITOR_GENERIC_RESOURCE_DISPLAY_WIDGET_H

#include <quentier/utility/Qt4Helper.h>
#include <quentier/utility/QNLocalizedString.h>
#include <QWidget>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QMimeDatabase)

namespace Ui {
QT_FORWARD_DECLARE_CLASS(GenericResourceDisplayWidget)
}

namespace quentier {

QT_FORWARD_DECLARE_CLASS(Resource)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)
QT_FORWARD_DECLARE_CLASS(Resource)

class GenericResourceDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    GenericResourceDisplayWidget(QWidget * parent = Q_NULLPTR);
    virtual ~GenericResourceDisplayWidget();

    void initialize(const QIcon & icon, const QString & name,
                    const QString & size, const QStringList & preferredFileSuffixes,
                    const QString & filterString, const Resource & resource,
                    const ResourceFileStorageManager & resourceFileStorageManager,
                    const FileIOThreadWorker & fileIOThreadWorker);

    QString resourceLocalUid() const;

    void updateResourceName(const QString & resourceName);

Q_SIGNALS:
    void savedResourceToFile();
    void openResourceRequest(const QByteArray & resourceHash);

// private signals
Q_SIGNALS:
    void saveResourceToStorage(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QByteArray dataHash,
                               QString preferredFileSuffix, QUuid requestId, bool isImage);
    void saveResourceToFile(QString filePath, QByteArray data, QUuid requestId, bool append);

private Q_SLOTS:
    void onOpenWithButtonPressed();
    void onSaveAsButtonPressed();

    void onSaveResourceToStorageRequestProcessed(QUuid requestId, QByteArray dataHash, QString fileStoragePath, int errorCode,
                                                 QNLocalizedString errorDescription);
    void onSaveResourceToFileRequestProcessed(bool success, QNLocalizedString errorDescription, QUuid requestId);

private:
    void setPendingMode(const bool pendingMode);
    void openResource();

    void setupFilterString(const QString & defaultFilterString);

private:
    Q_DISABLE_COPY(GenericResourceDisplayWidget)

private:
    Ui::GenericResourceDisplayWidget *  m_pUI;

    const Resource *                    m_pResource;
    const ResourceFileStorageManager *  m_pResourceFileStorageManager;
    const FileIOThreadWorker *          m_pFileIOThreadWorker;
    QStringList                         m_preferredFileSuffixes;
    QString                             m_filterString;

    QUuid                               m_saveResourceToFileRequestId;
    QUuid                               m_saveResourceToStorageRequestId;

    QByteArray                          m_resourceHash;
    bool                                m_savedResourceToStorage;
    bool                                m_pendingSaveResourceToStorage;
};

} // namespace quentier

#endif // LIB_QUENTIER_NOTE_EDITOR_GENERIC_RESOURCE_DISPLAY_WIDGET_H
