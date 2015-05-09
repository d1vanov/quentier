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
QT_FORWARD_DECLARE_CLASS(FileIOThreadWorker)

class GenericResourceDisplayWidget: public QWidget
{
    Q_OBJECT
public:
    GenericResourceDisplayWidget(const QIcon & icon, const QString & name,
                                 const QString & size, const QStringList & preferredFileSuffixes,
                                 const QString & mimeTypeName,
                                 const IResource & resource,
                                 const FileIOThreadWorker & fileIOThreadWorker,
                                 QWidget * parent = nullptr);

private:
    Q_DISABLE_COPY(GenericResourceDisplayWidget)

Q_SIGNALS:
    void savedResourceToFile();

// private signals
Q_SIGNALS:
    void writeResourceToFile(QString absoluteFilePath, QByteArray data, QUuid requestId);

private Q_SLOTS:
    void onOpenWithButtonPressed();
    void onSaveAsButtonPressed();

    void onWriteRequestProcessed(bool success, QString errorDescription, QUuid requestId);

private:
    void setPendingMode(const bool pendingMode);
    void openResource();
    void evaluateResourceHash();

    bool checkFileExistsAndUpToDate();

private:
    Ui::GenericResourceDisplayWidget *  m_pUI;

    const IResource *           m_pResource;
    const FileIOThreadWorker *  m_pFileIOThreadWorker;
    const QStringList           m_preferredFileSuffixes;
    const QString               m_mimeTypeName;

    QUuid                       m_saveResourceToFileRequestId;
    QUuid                       m_saveResourceToOwnFileRequestId;
    QUuid                       m_saveResourceHashToHelperFileRequestId;

    QByteArray                  m_resourceHash;
    QString                     m_ownFilePath;
    bool                        m_savedResourceToOwnFile;
    bool                        m_pendingSaveResourceToOwnFile;
};

} // namespace qute_note

#endif // __QUTE_NOTE__CORE__NOTE_EDITOR__GENERIC_RESOURCE_DISPLAY_WIDGET_H
