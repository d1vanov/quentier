#ifndef __LIB_QUTE_NOTE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
#define __LIB_QUTE_NOTE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H

#include <qute_note/utility/Qt4Helper.h>
#include <QWidget>
#include <QUuid>

QT_FORWARD_DECLARE_CLASS(QMimeDatabase)

namespace Ui {
QT_FORWARD_DECLARE_CLASS(GenericResourceDisplayWidget)
}

namespace qute_note {

QT_FORWARD_DECLARE_CLASS(ResourceWrapper)
QT_FORWARD_DECLARE_CLASS(ResourceFileStorageManager)
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)
QT_FORWARD_DECLARE_CLASS(IResource)

class GenericResourceDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    GenericResourceDisplayWidget(QWidget * parent = Q_NULLPTR);
    virtual ~GenericResourceDisplayWidget();

    void initialize(const QIcon & icon, const QString & name,
                    const QString & size, const QStringList & preferredFileSuffixes,
                    const QString & filterString, const IResource & resource,
                    const ResourceFileStorageManager & resourceFileStorageManager,
                    const FileIOThreadWorker & fileIOThreadWorker);

    QString resourceLocalUid() const;

    void updateResourceName(const QString & resourceName);

Q_SIGNALS:
    void savedResourceToFile();

// private signals
Q_SIGNALS:
    void saveResourceToStorage(QString noteLocalUid, QString resourceLocalUid, QByteArray data, QByteArray dataHash,
                               QString preferredFileSuffix, QUuid requestId, bool isImage);
    void saveResourceToFile(QString filePath, QByteArray data, QUuid requestId, bool append);

private Q_SLOTS:
    void onOpenWithButtonPressed();
    void onSaveAsButtonPressed();

    void onSaveResourceToStorageRequestProcessed(QUuid requestId, QByteArray dataHash, QString fileStoragePath, int errorCode, QString errorDescription);
    void onSaveResourceToFileRequestProcessed(bool success, QString errorDescription, QUuid requestId);

private:
    void setPendingMode(const bool pendingMode);
    void openResource();

    void setupFilterString(const QString & defaultFilterString);

private:
    Q_DISABLE_COPY(GenericResourceDisplayWidget)

private:
    Ui::GenericResourceDisplayWidget *  m_pUI;

    const ResourceWrapper *             m_pResource;
    const ResourceFileStorageManager *  m_pResourceFileStorageManager;
    const FileIOThreadWorker *          m_pFileIOThreadWorker;
    QStringList                         m_preferredFileSuffixes;
    QString                             m_filterString;

    QUuid                               m_saveResourceToFileRequestId;
    QUuid                               m_saveResourceToStorageRequestId;

    QByteArray                          m_resourceHash;
    QString                             m_ownFilePath;
    bool                                m_savedResourceToStorage;
    bool                                m_pendingSaveResourceToStorage;
};

} // namespace qute_note

#endif // __LIB_QUTE_NOTE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
