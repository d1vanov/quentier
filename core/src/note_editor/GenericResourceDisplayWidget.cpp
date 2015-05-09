#include "GenericResourceDisplayWidget.h"
#include "ui_GenericResourceDisplayWidget.h"
#include "AttachmentStoragePathConfigDialog.h"
#include <tools/FileIOThreadWorker.h>
#include <tools/QuteNoteCheckPtr.h>
#include <tools/DesktopServices.h>
#include <client/types/IResource.h>
#include <logging/QuteNoteLogger.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QSettings>
#include <QDesktopServices>
#include <QMessageBox>

namespace qute_note {

GenericResourceDisplayWidget::GenericResourceDisplayWidget(const QIcon & icon, const QString & name,
                                                           const QString & size,
                                                           const QStringList & preferredFileSuffixes,
                                                           const QString & mimeTypeName,
                                                           const IResource & resource,
                                                           const FileIOThreadWorker & fileIOThreadWorker,
                                                           QWidget * parent) :
    QWidget(parent),
    m_pUI(new Ui::GenericResourceDisplayWidget),
    m_pResource(&resource),
    m_pFileIOThreadWorker(&fileIOThreadWorker),
    m_preferredFileSuffixes(preferredFileSuffixes),
    m_mimeTypeName(mimeTypeName),
    m_saveResourceToFileRequestId(),
    m_saveResourceToOwnFileRequestId(),
    m_saveResourceHashToHelperFileRequestId(),
    m_resourceHash(),
    m_ownFilePath(),
    m_savedResourceToOwnFile(false),
    m_pendingSaveResourceToOwnFile(false)
{
    QNDEBUG("GenericResourceDisplayWidget::GenericResourceDisplayWidget: name = " << name
            << ", size = " << size);
    QNTRACE("Resource: " << resource);

    m_pUI->setupUi(this);

    m_pUI->resourceDisplayNameLabel->setText("<html><head/><body><p><span style=\" font-size:14pt; font-weight:600;\">" +
                                             name + "</span></p></body></head></html>");
    m_pUI->resourceDisplayNameLabel->setTextFormat(Qt::RichText);

    m_pUI->resourceSizeLabel->setText("<html><head/><body><p><span style=\" font-size:12pt; font-weight:600;\">" + size +
                                      "</span></p></body></head></html>");
    m_pUI->resourceSizeLabel->setTextFormat(Qt::RichText);

    m_pUI->resourceIconLabel->setPixmap(icon.pixmap(QSize(32,32)));

    if (!QIcon::hasThemeIcon("document-open")) {
        m_pUI->openResourceButton->setIcon(QIcon(":/generic_resource_icons/png/open_with.png"));
    }

    if (!QIcon::hasThemeIcon("document-save-as")) {
        m_pUI->saveResourceButton->setIcon(QIcon(":/generic_resource_icons/png/save.png"));
    }

    QObject::connect(m_pUI->openResourceButton, SIGNAL(released()), this, SLOT(onOpenWithButtonPressed()));
    QObject::connect(m_pUI->saveResourceButton, SIGNAL(released()), this, SLOT(onSaveAsButtonPressed()));

    QObject::connect(m_pFileIOThreadWorker, SIGNAL(writeFileRequestProcessed(bool,QString,QUuid)),
                     this, SLOT(onWriteRequestProcessed(bool,QString,QUuid)));
    QObject::connect(this, SIGNAL(writeResourceToFile(QString,QByteArray,QUuid)),
                     m_pFileIOThreadWorker, SLOT(onWriteFileRequest(QString,QByteArray,QUuid)));

    QSettings settings;
    settings.beginGroup("AttachmentSaveLocations");
    QString resourceFileStorageLocation = settings.value("OwnFileStorageLocation").toString();
    if (resourceFileStorageLocation.isEmpty()) {
        resourceFileStorageLocation = applicationPersistentStoragePath() + "/" + "attachments";
        settings.setValue("OwnFileStorageLocation", QVariant(resourceFileStorageLocation));
    }

    QFileInfo resourceFileStorageLocationInfo(resourceFileStorageLocation);
    QDir resourceFileStorageLocationDir(resourceFileStorageLocation);
    QString manualPath;
    if (!resourceFileStorageLocationInfo.exists())
    {
        bool res = resourceFileStorageLocationDir.mkpath(resourceFileStorageLocation);
        if (!res)
        {
            QNWARNING("Can't create directory for attachment tmp storage location: "
                      << resourceFileStorageLocation);
            manualPath = getAttachmentStoragePath(this);
        }
    }
    else if (!resourceFileStorageLocationInfo.isDir())
    {
        QNWARNING("Can't figure out where to save the temporary copies of attachments: path "
                  << resourceFileStorageLocation << " is not a directory");
        manualPath = getAttachmentStoragePath(this);
        return;
    }
    else if (resourceFileStorageLocationInfo.isWritable())
    {
        QNWARNING("Can't save temporary copies of attachments: the suggested folder is not writable: "
                  << resourceFileStorageLocation);
        manualPath = getAttachmentStoragePath(this);
        return;
    }

    if (!manualPath.isEmpty())
    {
        resourceFileStorageLocationInfo.setFile(manualPath);
        if (!resourceFileStorageLocationInfo.exists() || !resourceFileStorageLocationInfo.isDir() ||
                !resourceFileStorageLocationInfo.isWritable())
        {
            QNWARNING("Can't use manually selected attachment storage path");
            return;
        }

        settings.setValue("OwnFileStorageLocation", QVariant(manualPath));
        resourceFileStorageLocation = manualPath;
        resourceFileStorageLocationDir.setPath(manualPath);
    }

    m_ownFilePath = resourceFileStorageLocation + "/" + m_pResource->localGuid();

    if (checkFileExistsAndUpToDate()) {
        QNTRACE("Resource file already exists and is up to date, don't need to rewrite it again");
        m_savedResourceToOwnFile = true;
        return;
    }

    if (!m_pResource->hasDataBody() && !m_pResource->hasAlternateDataBody()) {
        QNWARNING("Resource passed to GenericResourceDisplayWidget has no data: " << *m_pResource);
        return;
    }

    // Write resource's data to file asynchronously so that it can further be opened in some application
    const QByteArray & data = (m_pResource->hasDataBody()
                               ? m_pResource->dataBody()
                               : m_pResource->alternateDataBody());

    emit writeResourceToFile(m_ownFilePath, data, m_saveResourceToOwnFileRequestId);
    QNTRACE("Emitted request to save the attachment to own file storage location, request id = "
            << m_saveResourceToOwnFileRequestId << ", file path = " << m_ownFilePath);


    // Write resource's data hash to file asynchronously so that the up-to-dateness of the file
    // with resource data can be evaluated later and without re-calculating the hash (can be time consuming
    // if the file is large)
    if (!m_pResource->hasDataHash() && m_pResource->hasAlternateDataHash()) {
        QNWARNING("Resource has neither data hash nor alternate data hash");
        evaluateResourceHash();
    }

    const QByteArray & hash = (m_pResource->hasDataHash()
                               ? m_pResource->dataHash()
                               : (m_pResource->hasAlternateDataHash()
                                  ? m_pResource->alternateDataHash()
                                  : m_resourceHash));
    m_saveResourceHashToHelperFileRequestId = QUuid::createUuid();
    emit writeResourceToFile(m_ownFilePath + ".hash", hash, m_saveResourceHashToHelperFileRequestId);
    QNTRACE("Emitted request to save the attachment's hash to own file storage location, request id = "
            << m_saveResourceHashToHelperFileRequestId << ", file path = " << m_ownFilePath + ".hash"
            << ", hash = " << hash.constData());
}

void GenericResourceDisplayWidget::onOpenWithButtonPressed()
{
    if (m_savedResourceToOwnFile) {
        openResource();
        return;
    }

    setPendingMode(true);
}

void GenericResourceDisplayWidget::onSaveAsButtonPressed()
{
    QNDEBUG("GenericResourceDisplayWidget::onSaveAsButtonPressed");

    QUTE_NOTE_CHECK_PTR(m_pResource);

    if (!m_pResource->hasDataBody() && !m_pResource->hasAlternateDataBody()) {
        QNWARNING("Can't save resource: no data body or alternate data body within the resource");
        internalErrorMessageBox(this, "resource has neither data body nor alternate data body");
        return;
    }

    const QByteArray & data = (m_pResource->hasDataBody()
                               ? m_pResource->dataBody()
                               : m_pResource->alternateDataBody());

    QString preferredSuffix;
    QString preferredDirectory;

    if (!m_preferredFileSuffixes.isEmpty())
    {
        QSettings settings;
        QStringList childGroups = settings.childGroups();
        int attachmentsSaveLocGroupIndex = childGroups.indexOf("AttachmentSaveLocations");
        if (attachmentsSaveLocGroupIndex >= 0)
        {
            QNTRACE("Found cached attachment save location group within application settings");

            settings.beginGroup("AttachmentSaveLocations");
            QStringList cachedFileSuffixes = settings.childKeys();
            const int numPreferredSuffixes = m_preferredFileSuffixes.size();
            for(int i = 0; i < numPreferredSuffixes; ++i)
            {
                preferredSuffix = m_preferredFileSuffixes[i];
                int indexInCache = cachedFileSuffixes.indexOf(preferredSuffix);
                if (indexInCache < 0) {
                    QNTRACE("Haven't found cached attachment save directory for file suffix " << preferredSuffix);
                    continue;
                }

                QVariant dirValue = settings.value(preferredSuffix);
                if (dirValue.isNull() || !dirValue.isValid()) {
                    QNTRACE("Found inappropriate attachment save directory for file suffix " << preferredSuffix);
                    continue;
                }

                QFileInfo dirInfo(dirValue.toString());
                if (!dirInfo.exists()) {
                    QNTRACE("Cached attachment save directory for file suffix " << preferredSuffix
                            << " does not exist: " << dirInfo.absolutePath());
                    continue;
                }
                else if (!dirInfo.isDir()) {
                    QNTRACE("Cached attachment save directory for file suffix " << preferredSuffix
                            << " is not a directory: " << dirInfo.absolutePath());
                    continue;
                }
                else if (!dirInfo.isWritable()) {
                    QNTRACE("Cached attachment save directory for file suffix " << preferredSuffix
                            << " is not writable: " << dirInfo.absolutePath());
                    continue;
                }

                preferredDirectory = dirInfo.absolutePath();
                break;
            }

            settings.endGroup();
        }
    }

    QString filter = m_mimeTypeName + " (*.";
    filter += preferredSuffix;
    filter += " ";

    const int numPreferredSuffixes = m_preferredFileSuffixes.size();
    for(int i = 0; i < numPreferredSuffixes; ++i)
    {
        const QString & currentSuffix = m_preferredFileSuffixes[i];
        if (currentSuffix == preferredSuffix) {
            continue;
        }

        filter += "*.";
        filter += currentSuffix;
        filter += " ";
    }

    // Remove the trailing whitespace
    filter = filter.remove(filter.size()-1, 1);
    filter += ");;";
    filter += QObject::tr("All files");
    filter += " (*)";

    QString fileName = QFileDialog::getSaveFileName(this, QObject::tr("Save as..."),
                                                    preferredDirectory, filter);
    if (fileName.isEmpty()) {
        QNINFO("User cancelled writing to file");
        return;
    }

    bool foundSuffix = false;
    for(int i = 0; i < numPreferredSuffixes; ++i)
    {
        const QString & currentSuffix = m_preferredFileSuffixes[i];
        if (fileName.endsWith(currentSuffix, Qt::CaseInsensitive)) {
            foundSuffix = true;
            break;
        }
    }

    if (!foundSuffix) {
        fileName += "." + preferredSuffix;
    }

    m_saveResourceToFileRequestId = QUuid::createUuid();
    emit writeResourceToFile(fileName, data, m_saveResourceToFileRequestId);
    QNDEBUG("Sent request to save resource to file, request id = " << m_saveResourceToFileRequestId);
}

void GenericResourceDisplayWidget::onWriteRequestProcessed(bool success, QString errorDescription,
                                                           QUuid requestId)
{
    if (requestId == m_saveResourceToFileRequestId)
    {
        if (success) {
            QNDEBUG("Successfully saved resource to file, request id = " << requestId);
            emit savedResourceToFile();
        }
        else {
            QNWARNING("Could not save resource to file: " << errorDescription
                      << "; request id = " << requestId);
            warningMessageBox(this, QObject::tr("Error saving the resource to file"),
                              QObject::tr("Could not save the resource to file"),
                              errorDescription);
        }
    }
    else if (requestId == m_saveResourceToOwnFileRequestId)
    {
        if (success)
        {
            QNDEBUG("Successfully saved resource to own file, request id = " << requestId);
            m_savedResourceToOwnFile = true;
            if (m_pendingSaveResourceToOwnFile) {
                setPendingMode(false);
                openResource();
            }
        }
        else
        {
            QNWARNING("Could not save resource to own file: " << errorDescription
                      << "; request id = " << requestId);
            warningMessageBox(this, QObject::tr("Error saving the resource to hidden file"),
                              QObject::tr("Could not save the resource to hidden file "
                                          "(in order to make it possible to open it with some application)"),
                              errorDescription);
            if (m_pendingSaveResourceToOwnFile) {
                setPendingMode(false);
            }
        }
    }
    else if (requestId == m_saveResourceHashToHelperFileRequestId)
    {
        if (success) {
            QNDEBUG("Successfully saved resource's hash to helper file, request id = " << requestId);
        }
        else {
            QNWARNING("Could not save the resource's hash to helper file: " << errorDescription
                      << "; request id = " << requestId);
            warningMessageBox(this, QObject::tr("Error saving the resource hash to hidden helper file"),
                              QObject::tr("Could not save the resource's hash to hidden helper file "
                                          "(for internal needs of the application)"),
                              errorDescription);
        }
    }
    // else it's not ours request reply, skip it
}

void GenericResourceDisplayWidget::setPendingMode(const bool pendingMode)
{
    QNDEBUG("GenericResourceDisplayWidget::setPendingMode: pending mode = "
            << (pendingMode ? "true" : "false"));

    m_pendingSaveResourceToOwnFile = pendingMode;
    if (pendingMode) {
        QApplication::setOverrideCursor(Qt::BusyCursor);
    }
    else {
        QApplication::restoreOverrideCursor();
    }
}

void GenericResourceDisplayWidget::openResource()
{
    QNDEBUG("GenericResourceDisplayWidget::openResource: " << m_ownFilePath);
    QDesktopServices::openUrl(QUrl("file://" + m_ownFilePath));
}

void GenericResourceDisplayWidget::evaluateResourceHash()
{
    QNDEBUG("GenericResourceDisplayWidget::evaluateResourceHash");

    if (!m_pResource->hasDataBody() && !m_pResource->hasAlternateDataBody()) {
        QNWARNING("Can't evaluate resource hash: resource has neither data nor alternate data");
        return;
    }

    const QByteArray & data = (m_pResource->hasDataBody()
                               ? m_pResource->dataBody()
                               : m_pResource->alternateDataBody());
    m_resourceHash = QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
    QNTRACE("Evaluated resource's hash for " << (m_pResource->hasDataBody() ? "data" : "alternate data")
            << ": " << m_resourceHash.constData());
}

bool GenericResourceDisplayWidget::checkFileExistsAndUpToDate()
{
    QNDEBUG("GenericResourceDisplayWidget::checkFileExistsAndUpToDate");

    if (!m_pResource->hasDataHash() && !m_pResource->hasAlternateDataHash()) {
        QNWARNING("Resource does not have neither data hash nor alternate data hash");
        evaluateResourceHash();
    }

    const QByteArray & resourceHash = (m_pResource->hasDataHash()
                                       ? m_pResource->dataHash()
                                       : (m_pResource->hasAlternateDataHash()
                                          ? m_pResource->alternateDataHash()
                                          : m_resourceHash));

    QFileInfo ownFileInfo(m_ownFilePath);
    if (!ownFileInfo.exists()) {
        QNTRACE("Resource's own file does not exist yet: " << m_ownFilePath);
        return false;
    }

    QNTRACE("Resource's own file already exists, checking whether it is up to date");
    QFileInfo ownFileHashInfo(m_ownFilePath + ".hash");
    if (!ownFileHashInfo.exists()) {
        QNTRACE("Could not find helper file with precalculated resource's hash");
        return false;
    }

    QFile ownFileHash(m_ownFilePath + ".hash");
    bool open = ownFileHash.open(QIODevice::ReadOnly);
    if (!open) {
        QNWARNING("Can't open helper file with precalculated resource's hash");
        warningMessageBox(this, QObject::tr("Error reading the resource's hash from helper file"),
                          QObject::tr("Could not read the resource's hash from helper file"),
                          QObject::tr("Can't open the file for writing: ") + m_ownFilePath);
        return false;
    }

    QByteArray hash = ownFileHash.readAll();
    return (resourceHash == hash);
}

} // namespace qute_note
