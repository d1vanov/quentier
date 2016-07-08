#include "GenericResourceDisplayWidget.h"
#include "ui_GenericResourceDisplayWidget.h"
#include <quentier/note_editor/ResourceFileStorageManager.h>
#include <quentier/utility/FileIOThreadWorker.h>
#include <quentier/utility/QuentierCheckPtr.h>
#include <quentier/utility/DesktopServices.h>
#include <quentier/types/ResourceWrapper.h>
#include <quentier/logging/QuentierLogger.h>
#include <quentier/utility/ApplicationSettings.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QDesktopServices>
#include <QMessageBox>

namespace quentier {

GenericResourceDisplayWidget::GenericResourceDisplayWidget(QWidget * parent) :
    QWidget(parent),
    m_pUI(new Ui::GenericResourceDisplayWidget),
    m_pResource(Q_NULLPTR),
    m_pResourceFileStorageManager(Q_NULLPTR),
    m_pFileIOThreadWorker(Q_NULLPTR),
    m_preferredFileSuffixes(Q_NULLPTR),
    m_filterString(),
    m_saveResourceToFileRequestId(),
    m_saveResourceToStorageRequestId(),
    m_resourceHash(),
    m_savedResourceToStorage(false),
    m_pendingSaveResourceToStorage(false)
{
    m_pUI->setupUi(this);
}

GenericResourceDisplayWidget::~GenericResourceDisplayWidget()
{
    delete m_pUI;
    delete m_pResource;
}

void GenericResourceDisplayWidget::initialize(const QIcon & icon, const QString & name,
                                              const QString & size, const QStringList & preferredFileSuffixes,
                                              const QString & filterString,
                                              const IResource & resource,
                                              const ResourceFileStorageManager & resourceFileStorageManager,
                                              const FileIOThreadWorker & fileIOThreadWorker)
{
    QNDEBUG("GenericResourceDisplayWidget::initialize: name = " << name << ", size = " << size);

    m_pResource = new ResourceWrapper(resource);
    m_pResourceFileStorageManager = &resourceFileStorageManager;
    m_pFileIOThreadWorker = &fileIOThreadWorker;
    m_preferredFileSuffixes = preferredFileSuffixes;

    setupFilterString(filterString);

    updateResourceName(name);
    m_pUI->resourceDisplayNameLabel->setTextFormat(Qt::RichText);

    m_pUI->resourceSizeLabel->setText("<html><head/><body><p><span style=\" font-size:8pt;\">" + size +
                                      "</span></p></body></head></html>");
    m_pUI->resourceSizeLabel->setTextFormat(Qt::RichText);

    m_pUI->resourceIconLabel->setPixmap(icon.pixmap(QSize(16,16)));

    if (!QIcon::hasThemeIcon("document-open")) {
        m_pUI->openResourceButton->setIcon(QIcon(":/generic_resource_icons/png/open_with.png"));
    }

    if (!QIcon::hasThemeIcon("document-save-as")) {
        m_pUI->saveResourceButton->setIcon(QIcon(":/generic_resource_icons/png/save.png"));
    }

    QObject::connect(m_pUI->openResourceButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(GenericResourceDisplayWidget,onOpenWithButtonPressed));
    QObject::connect(m_pUI->saveResourceButton, QNSIGNAL(QPushButton,released),
                     this, QNSLOT(GenericResourceDisplayWidget,onSaveAsButtonPressed));

    QObject::connect(m_pResourceFileStorageManager, QNSIGNAL(ResourceFileStorageManager,writeResourceToFileCompleted,QUuid,QByteArray,QString,int,QString),
                     this, QNSLOT(GenericResourceDisplayWidget,onSaveResourceToStorageRequestProcessed,QUuid,QByteArray,QString,int,QString));
    QObject::connect(this, QNSIGNAL(GenericResourceDisplayWidget,saveResourceToStorage,QString,QString,QByteArray,QByteArray,QString,QUuid,bool),
                     m_pResourceFileStorageManager, QNSLOT(ResourceFileStorageManager,onWriteResourceToFileRequest,QString,QString,QByteArray,QByteArray,QString,QUuid,bool));

    QObject::connect(m_pFileIOThreadWorker, QNSIGNAL(FileIOThreadWorker,writeFileRequestProcessed,bool,QString,QUuid),
                     this, QNSLOT(GenericResourceDisplayWidget,onSaveResourceToFileRequestProcessed,bool,QString,QUuid));
    QObject::connect(this, QNSIGNAL(GenericResourceDisplayWidget,saveResourceToFile,QString,QByteArray,QUuid,bool),
                     m_pFileIOThreadWorker, QNSLOT(FileIOThreadWorker,onWriteFileRequest,QString,QByteArray,QUuid,bool));

    if (!m_pResource->hasDataBody() && !m_pResource->hasAlternateDataBody()) {
        QNWARNING("Resource passed to GenericResourceDisplayWidget has no data: " << *m_pResource);
        return;
    }

    const QByteArray & data = (m_pResource->hasDataBody()
                               ? m_pResource->dataBody()
                               : m_pResource->alternateDataBody());

    const QByteArray * dataHash = Q_NULLPTR;
    if (m_pResource->hasDataBody() && m_pResource->hasDataHash()) {
        dataHash = &(m_pResource->dataHash());
    }
    else if (m_pResource->hasAlternateDataBody() && m_pResource->hasAlternateDataHash()) {
        dataHash = &(m_pResource->alternateDataHash());
    }

    QByteArray localDataHash;
    if (!dataHash) {
        dataHash = &localDataHash;
    }

    // Write resource's data to file asynchronously so that it can further be opened in some application
    QString preferredFileSuffix = m_pResource->preferredFileSuffix();
    m_saveResourceToStorageRequestId = QUuid::createUuid();
    bool isImage = (m_pResource->hasMime()
                    ? m_pResource->mime().startsWith("image")
                    : false);

    QNTRACE("Emitting the request to save the attachment to own file storage location, request id = "
            << m_saveResourceToStorageRequestId << ", resource local uid = " << m_pResource->localUid());
    emit saveResourceToStorage(m_pResource->noteLocalUid(), m_pResource->localUid(), data, *dataHash,
                               preferredFileSuffix, m_saveResourceToStorageRequestId, isImage);
}

QString GenericResourceDisplayWidget::resourceLocalUid() const
{
    if (m_pResource) {
        return m_pResource->localUid();
    }

    return QString();
}

void GenericResourceDisplayWidget::updateResourceName(const QString & resourceName)
{
    m_pUI->resourceDisplayNameLabel->setText("<html><head/><body><p><span style=\" font-size:8pt;\">" +
                                             resourceName + "</span></p></body></head></html>");
}

void GenericResourceDisplayWidget::onOpenWithButtonPressed()
{
    if (m_savedResourceToStorage) {
        openResource();
        return;
    }

    setPendingMode(true);
}

void GenericResourceDisplayWidget::onSaveAsButtonPressed()
{
    QNDEBUG("GenericResourceDisplayWidget::onSaveAsButtonPressed");

    QUENTIER_CHECK_PTR(m_pResource);

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
        ApplicationSettings appSettings;
        QStringList childGroups = appSettings.childGroups();
        int attachmentsSaveLocGroupIndex = childGroups.indexOf("AttachmentSaveLocations");
        if (attachmentsSaveLocGroupIndex >= 0)
        {
            QNTRACE("Found cached attachment save location group within application settings");

            appSettings.beginGroup("AttachmentSaveLocations");
            QStringList cachedFileSuffixes = appSettings.childKeys();
            const int numPreferredSuffixes = m_preferredFileSuffixes.size();
            for(int i = 0; i < numPreferredSuffixes; ++i)
            {
                preferredSuffix = m_preferredFileSuffixes[i];
                int indexInCache = cachedFileSuffixes.indexOf(preferredSuffix);
                if (indexInCache < 0) {
                    QNTRACE("Haven't found cached attachment save directory for file suffix " << preferredSuffix);
                    continue;
                }

                QVariant dirValue = appSettings.value(preferredSuffix);
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

            appSettings.endGroup();
        }
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                                    preferredDirectory, m_filterString);
    if (fileName.isEmpty()) {
        QNINFO("User cancelled writing to file");
        return;
    }

    bool foundSuffix = false;
    const int numPreferredSuffixes = m_preferredFileSuffixes.size();
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
    emit saveResourceToFile(fileName, data, m_saveResourceToFileRequestId, /* append = */ false);
    QNDEBUG("Sent request to save resource to file, request id = " << m_saveResourceToFileRequestId);
}

void GenericResourceDisplayWidget::onSaveResourceToStorageRequestProcessed(QUuid requestId, QByteArray dataHash,
                                                                           QString fileStoragePath, int errorCode,
                                                                           QString errorDescription)
{
    if (requestId == m_saveResourceToStorageRequestId)
    {
        if (errorCode == 0)
        {
            QNDEBUG("Successfully saved resource to storage, request id = " << requestId
                    << ", file storage path = " << fileStoragePath);
            m_savedResourceToStorage = true;
            m_resourceHash = dataHash;
            if (m_pendingSaveResourceToStorage) {
                setPendingMode(false);
                openResource();
            }
        }
        else
        {
            QNWARNING("Could not save resource to storage: " << errorDescription
                      << "; request id = " << requestId);
            warningMessageBox(this, tr("Error saving the resource to hidden file"),
                              tr("Could not save the resource to hidden file "
                                 "(in order to make it possible to open it with some application)"),
                              tr("Error code = ") + QString::number(errorCode) + ": " + errorDescription);
            if (m_pendingSaveResourceToStorage) {
                setPendingMode(false);
            }
        }
    }
    // otherwise it's not ours request reply, skip it
}

void GenericResourceDisplayWidget::onSaveResourceToFileRequestProcessed(bool success,
                                                                        QString errorDescription,
                                                                        QUuid requestId)
{
    QNTRACE("GenericResourceDisplayWidget::onSaveResourceToFileRequestProcessed: success = "
            << (success ? "true" : "false") << ", request id = " << requestId);

    if (requestId == m_saveResourceToFileRequestId)
    {
        if (success) {
            QNDEBUG("Successfully saved resource to file, request id = " << requestId);
            emit savedResourceToFile();
        }
        else {
            QNWARNING("Could not save resource to file: " << errorDescription
                      << "; request id = " << requestId);
            warningMessageBox(this, tr("Error saving the resource to file"),
                              tr("Could not save the resource to file"),
                              errorDescription);
        }
    }
}

void GenericResourceDisplayWidget::setPendingMode(const bool pendingMode)
{
    QNDEBUG("GenericResourceDisplayWidget::setPendingMode: pending mode = "
            << (pendingMode ? "true" : "false"));

    m_pendingSaveResourceToStorage = pendingMode;
    if (pendingMode) {
        QApplication::setOverrideCursor(Qt::BusyCursor);
    }
    else {
        QApplication::restoreOverrideCursor();
    }
}

void GenericResourceDisplayWidget::openResource()
{
    QNDEBUG("GenericResourceDisplayWidget::openResource: hash = " << m_resourceHash.toHex());
    emit openResourceRequest(m_resourceHash);
}

void GenericResourceDisplayWidget::setupFilterString(const QString & defaultFilterString)
{
    QNDEBUG("GenericResourceDisplayWidget::setupFilterString: default = " << defaultFilterString);

    m_filterString = defaultFilterString;

    if (Q_UNLIKELY(!m_pResource)) {
        return;
    }

    QString resourcePreferredSuffix = m_pResource->preferredFileSuffix();
    QString resourcePreferredFilterString;
    if (!resourcePreferredSuffix.isEmpty()) {
        resourcePreferredFilterString = "(*." + resourcePreferredSuffix + ")";
    }

    QNTRACE("Resource preferred file suffix = " << resourcePreferredSuffix
            << ", resource preferred filter string = " << resourcePreferredFilterString);

    bool shouldSkipResourcePreferredSuffix = false;
    if (!resourcePreferredSuffix.isEmpty() && !m_preferredFileSuffixes.contains(resourcePreferredSuffix))
    {
        const int numSuffixes = m_preferredFileSuffixes.size();
        for(int i = 0; i < numSuffixes; ++i)
        {
            const QString & currentSuffix = m_preferredFileSuffixes[i];
            if (resourcePreferredSuffix.contains(currentSuffix)) {
                shouldSkipResourcePreferredSuffix = true;
                break;
            }
        }

        if (!shouldSkipResourcePreferredSuffix) {
            m_preferredFileSuffixes.prepend(resourcePreferredSuffix);
        }
    }

    if (!shouldSkipResourcePreferredSuffix && !resourcePreferredFilterString.isEmpty()) {
        m_filterString = resourcePreferredFilterString + ";;" + m_filterString;
        QNTRACE("m_filterString = " << m_filterString);
    }
}

} // namespace quentier
