#ifndef __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
#define __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H

#include <QWidget>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QMimeDatabase)

namespace Ui {
QT_FORWARD_DECLARE_CLASS(GenericResourceDisplayWidget)
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(IResource)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

class GenericResourceDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    GenericResourceDisplayWidget(const QIcon & icon, const QString & name,
                                 const QString & size, const QStringList & preferredFileSuffixes,
                                 const QString & filterString,
                                 const IResource & resource,
                                 const ResourceFileStorageManager & resourceFileStorageManager,
                                 const FileIOThreadWorker & fileIOThreadWorker,
                                 QWidget * parent = nullptr);

private:
    Q_DISABLE_COPY(GenericResourceDisplayWidget)

Q_SIGNALS:
    void savedResourceToFile();

// private signals
Q_SIGNALS:
    void saveResourceToStorage(QString localGuid, QByteArray data, QByteArray dataHash, QUuid requestId);
    void saveResourceToFile(QString filePath, QByteArray data, QUuid requestId);

private Q_SLOTS:
    void onOpenWithButtonPressed();
    void onSaveAsButtonPressed();

    void onSaveResourceToStorageRequestProcessed(QUuid requestId, QByteArray dataHash, int errorCode, QString errorDescription);
    void onSaveResourceToFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);

private:
    void setPendingMode(const bool pendingMode);
    void openResource();

private:
    Ui::GenericResourceDisplayWidget *  m_pUI;

    const IResource *                   m_pResource;
    const ResourceFileStorageManager *  m_pResourceFileStorageManager;
    const FileIOThreadWorker *          m_pFileIOThreadWorker;
    const QStringList                   m_preferredFileSuffixes;
    const QString                       m_filterString;

    QUuid                               m_saveResourceToFileRequestId;
    QUuid                               m_saveResourceToStorageRequestId;

    QByteArray                          m_resourceHash;
    QString                             m_ownFilePath;
    bool                                m_savedResourceToStorage;
    bool                                m_pendingSaveResourceToStorage;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
